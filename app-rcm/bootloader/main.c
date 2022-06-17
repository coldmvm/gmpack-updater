/*
 * Copyright (c) 2018 naehrwert
 *
 * Copyright (c) 2018-2020 CTCaer
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdlib.h>

#include <memory_map.h>

#include "config.h"
#include <display/di.h>
#include <gfx_utils.h>
#include "gfx/logos.h"
#include <ianos/ianos.h>
#include <libs/compr/blz.h>
#include <libs/fatfs/ff.h>
#include <mem/heap.h>
#include <mem/minerva.h>
#include <mem/sdram.h>
#include <power/bq24193.h>
#include <power/max17050.h>
#include <power/max77620.h>
#include <power/max7762x.h>
#include <rtc/max77620-rtc.h>
#include <soc/bpmp.h>
#include <soc/fuse.h>
#include <soc/hw_init.h>
#include <soc/i2c.h>
#include <soc/pmc.h>
#include <soc/t210.h>
#include <soc/uart.h>
#include <storage/nx_sd.h>
#include <storage/sdmmc.h>
#include <utils/btn.h>
#include <utils/dirlist.h>
#include <utils/list.h>
#include <utils/util.h>
#include <stdio.h>
#include <ctype.h>

hekate_config h_cfg;
boot_cfg_t __attribute__((section ("._boot_cfg"))) b_cfg;

volatile nyx_storage_t *nyx_str = (nyx_storage_t *)NYX_STORAGE_ADDR;

// This is a safe and unused DRAM region for our payloads.
#define RELOC_META_OFF      0x7C
#define PATCHED_RELOC_SZ    0x94
#define PATCHED_RELOC_STACK 0x40007000
#define PATCHED_RELOC_ENTRY 0x40010000
#define EXT_PAYLOAD_ADDR    0xC0000000
#define RCM_PAYLOAD_ADDR    (EXT_PAYLOAD_ADDR + ALIGN(PATCHED_RELOC_SZ, 0x10))
#define COREBOOT_END_ADDR   0xD0000000
#define CBFS_DRAM_EN_ADDR   0x4003e000
#define  CBFS_DRAM_MAGIC    0x4452414D // "DRAM"

char appName[100];
bool printCRLF = false;

static void *coreboot_addr;

void reloc_patcher(u32 payload_dst, u32 payload_src, u32 payload_size)
{
    memcpy((u8 *)payload_src, (u8 *)IPL_LOAD_ADDR, PATCHED_RELOC_SZ);

    volatile reloc_meta_t *relocator = (reloc_meta_t *)(payload_src + RELOC_META_OFF);

    relocator->start = payload_dst - ALIGN(PATCHED_RELOC_SZ, 0x10);
    relocator->stack = PATCHED_RELOC_STACK;
    relocator->end   = payload_dst + payload_size;
    relocator->ep    = payload_dst;

    if (payload_size == 0x7000)
    {
        memcpy((u8 *)(payload_src + ALIGN(PATCHED_RELOC_SZ, 0x10)), coreboot_addr, 0x7000); //Bootblock
        *(vu32 *)CBFS_DRAM_EN_ADDR = CBFS_DRAM_MAGIC;
    }
}

int launch_payload(char *path, bool update)
{
    if (sd_mount())
    {
        FIL fp;
        if (f_open(&fp, path, FA_READ))
        {
            gfx_con.mute = 0;
            EPRINTFARGS("Payload file is missing!\n(%s)", path);

            goto out;
        }

        // Read and copy the payload to our chosen address
        void *buf;
        u32 size = f_size(&fp);

        if (size < 0x30000)
            buf = (void *)RCM_PAYLOAD_ADDR;
        else
        {
            coreboot_addr = (void *)(COREBOOT_END_ADDR - size);
            buf = coreboot_addr;
            if (h_cfg.t210b01)
            {
                f_close(&fp);

                gfx_con.mute = 0;
                EPRINTF("Coreboot not allowed on Mariko!");

                goto out;
            }
        }

        if (f_read(&fp, buf, size, NULL))
        {
            f_close(&fp);

            goto out;
        }

        f_close(&fp);

        sd_end();

        if (size < 0x30000)
        {
            if (update)
                memcpy((u8 *)(RCM_PAYLOAD_ADDR + PATCHED_RELOC_SZ), &b_cfg, sizeof(boot_cfg_t)); // Transfer boot cfg.
            else
                reloc_patcher(PATCHED_RELOC_ENTRY, EXT_PAYLOAD_ADDR, ALIGN(size, 0x10));

            hw_reinit_workaround(false, byte_swap_32(*(u32 *)(buf + size - sizeof(u32))));
        }
        else
        {
            reloc_patcher(PATCHED_RELOC_ENTRY, EXT_PAYLOAD_ADDR, 0x7000);
            hw_reinit_workaround(true, 0);
        }

        // Some cards (Sandisk U1), do not like a fast power cycle. Wait min 100ms.
        sdmmc_storage_init_wait_sd();

        void (*ext_payload_ptr)() = (void *)EXT_PAYLOAD_ADDR;
        void (*update_ptr)() = (void *)RCM_PAYLOAD_ADDR;

        // Launch our payload.
        if (!update)
            (*ext_payload_ptr)();
        else
        {
            EMC(EMC_SCRATCH0) |= EMC_HEKA_UPD;
            (*update_ptr)();
        }
    }

out:
    if (!update)
        sd_end();

    return 1;
}

#define EXCP_EN_ADDR   0x4003FFFC
#define  EXCP_MAGIC 0x30505645      // EVP0
#define EXCP_TYPE_ADDR 0x4003FFF8
#define  EXCP_TYPE_RESET 0x545352   // RST
#define  EXCP_TYPE_UNDEF 0x464455   // UDF
#define  EXCP_TYPE_PABRT 0x54424150 // PABT
#define  EXCP_TYPE_DABRT 0x54424144 // DABT
#define EXCP_LR_ADDR   0x4003FFF4

static void _show_errors()
{
    u32 *excp_enabled = (u32 *)EXCP_EN_ADDR;
    u32 *excp_type = (u32 *)EXCP_TYPE_ADDR;
    u32 *excp_lr = (u32 *)EXCP_LR_ADDR;

    if (*excp_enabled == EXCP_MAGIC)
        h_cfg.errors |= ERR_EXCEPTION;

    //! FIXME: Find a better way to identify if that scratch has proper data.
    if (0 && PMC(APBDEV_PMC_SCRATCH37) & PMC_SCRATCH37_KERNEL_PANIC_FLAG)
    {
        // Set error and clear flag.
        h_cfg.errors |= ERR_L4T_KERNEL;
        PMC(APBDEV_PMC_SCRATCH37) &= ~PMC_SCRATCH37_KERNEL_PANIC_FLAG;
    }

    if (h_cfg.errors)
    {
        gfx_clear_grey(0x1B);
        gfx_con_setpos(0, 0);
        display_backlight_brightness(150, 1000);

        if (h_cfg.errors & ERR_SD_BOOT_EN)
            WPRINTF("Failed to mount SD!\n");

        if (h_cfg.errors & ERR_LIBSYS_LP0)
            WPRINTF("Missing LP0 (sleep mode) lib!\n");
        if (h_cfg.errors & ERR_LIBSYS_MTC)
            WPRINTF("Missing or old Minerva lib!\n");

        if (h_cfg.errors & (ERR_LIBSYS_LP0 | ERR_LIBSYS_MTC))
            WPRINTF("\nUpdate bootloader folder!\n\n");

        if (h_cfg.errors & ERR_EXCEPTION)
        {
            WPRINTFARGS("An exception occurred (LR %08X):\n", *excp_lr);
            switch (*excp_type)
            {
            case EXCP_TYPE_RESET:
                WPRINTF("RESET");
                break;
            case EXCP_TYPE_UNDEF:
                WPRINTF("UNDEF");
                break;
            case EXCP_TYPE_PABRT:
                WPRINTF("PABRT");
                break;
            case EXCP_TYPE_DABRT:
                WPRINTF("DABRT");
                break;
            }
            WPRINTF("\n");

            // Clear the exception.
            *excp_enabled = 0;
        }

        if (0 && h_cfg.errors & ERR_L4T_KERNEL)
        {
            WPRINTF("Panic occurred while running L4T.\n");
            if (!sd_save_to_file((void *)PSTORE_ADDR, PSTORE_SZ, "L4T_panic.bin"))
                WPRINTF("PSTORE saved to L4T_panic.bin\n");
        }

        WPRINTF("Press any key...");

        msleep(1000);
        btn_wait();
    }
}

static void _check_low_battery()
{
    if (fuse_read_hw_state() == FUSE_NX_HW_STATE_DEV)
        goto out;

    int enough_battery;
    int batt_volt = 0;
    int charge_status = 0;

    bq24193_get_property(BQ24193_ChargeStatus, &charge_status);
    max17050_get_property(MAX17050_AvgVCELL, &batt_volt);

    enough_battery = charge_status ? 3250 : 3000;

    if (batt_volt > enough_battery || !batt_volt)
        goto out;

    // Prepare battery icon resources.
    u8 *battery_res = malloc(ALIGN(SZ_BATTERY_EMPTY, 0x1000));
    blz_uncompress_srcdest(BATTERY_EMPTY_BLZ, SZ_BATTERY_EMPTY_BLZ, battery_res, SZ_BATTERY_EMPTY);

    u8 *battery_icon = malloc(0x95A);  // 21x38x3
    u8 *charging_icon = malloc(0x2F4); // 21x12x3
    u8 *no_charging_icon = calloc(0x2F4, 1);

    memcpy(charging_icon, battery_res, 0x2F4);
    memcpy(battery_icon, battery_res + 0x2F4, 0x95A);

    u32 battery_icon_y_pos = 1280 - 16 - Y_BATTERY_EMPTY_BATT;
    u32 charging_icon_y_pos = 1280 - 16 - Y_BATTERY_EMPTY_BATT - 12 - Y_BATTERY_EMPTY_CHRG;
    free(battery_res);

    charge_status = !charge_status;

    u32 timer = 0;
    bool screen_on = false;
    while (true)
    {
        bpmp_msleep(250);

        // Refresh battery stats.
        int current_charge_status = 0;
        bq24193_get_property(BQ24193_ChargeStatus, &current_charge_status);
        max17050_get_property(MAX17050_AvgVCELL, &batt_volt);
        enough_battery = current_charge_status ? 3250 : 3000;

        if (batt_volt > enough_battery)
            break;

        // Refresh charging icon.
        if (screen_on && (charge_status != current_charge_status))
        {
            if (current_charge_status)
                gfx_set_rect_rgb(charging_icon, X_BATTERY_EMPTY, Y_BATTERY_EMPTY_CHRG, 16, charging_icon_y_pos);
            else
                gfx_set_rect_rgb(no_charging_icon, X_BATTERY_EMPTY, Y_BATTERY_EMPTY_CHRG, 16, charging_icon_y_pos);
        }

        // Check if it's time to turn off display.
        if (screen_on && timer < get_tmr_ms())
        {
            if (!current_charge_status)
            {
                max77620_low_battery_monitor_config(true);
                power_set_state(POWER_OFF_RESET);
            }

            display_end();
            screen_on = false;
        }

        // Check if charging status changed or Power button was pressed.
        if ((charge_status != current_charge_status) || (btn_wait_timeout_single(0, BTN_POWER) & BTN_POWER))
        {
            if (!screen_on)
            {
                display_init();
                u32 *fb = display_init_framebuffer_pitch();
                gfx_init_ctxt(fb, 720, 1280, 720);

                gfx_set_rect_rgb(battery_icon, X_BATTERY_EMPTY, Y_BATTERY_EMPTY_BATT, 16, battery_icon_y_pos);
                if (current_charge_status)
                    gfx_set_rect_rgb(charging_icon, X_BATTERY_EMPTY, Y_BATTERY_EMPTY_CHRG, 16, charging_icon_y_pos);
                else
                    gfx_set_rect_rgb(no_charging_icon, X_BATTERY_EMPTY, Y_BATTERY_EMPTY_CHRG, 16, charging_icon_y_pos);

                display_backlight_pwm_init();
                display_backlight_brightness(100, 1000);

                screen_on = true;
            }

            timer = get_tmr_ms() + 15000;
        }

        // Check if forcefully continuing.
        if (btn_read_vol() == (BTN_VOL_UP | BTN_VOL_DOWN))
            break;

        charge_status = current_charge_status;
    }

    display_end();

    free(battery_icon);
    free(charging_icon);
    free(no_charging_icon);

out:
    // Re enable Low Battery Monitor shutdown.
    max77620_low_battery_monitor_config(true);
}

FRESULT easy_rename(const char* old, const char* new)
{
    FRESULT res = FR_OK;
    if (f_stat(old, NULL) == FR_OK) {
        if (f_stat(new, NULL) == FR_OK) {
            res = f_unlink(new);
        }
        if (res == FR_OK) {
            res = f_rename(old, new);
        }
    }
    return res;
}

struct Node
{
    char data;
    struct Node *nextPtr;
};

char* toLower(char* s)
{
    for(char *p = s; *p; p++) *p = tolower(*p);
    return s;
}

bool canDelete(TCHAR* cPath)
{
    char * x [] = {
        "/apgtmppackfolder",
        "/nintendo",
        "/emummc",
        "/emutendo",
        "/jksv",
        "/checkpoint",
        "/switch/tinfoil/credentials.json",
        "/switch/tinfoil/gdrive.token",
        "/switch/tinfoil/locations.conf"
    };

    int len = sizeof(x)/sizeof(x[0]);
    int i;

    bool res = true;

    for (i = 0; i < len; ++i)
        if (!strcmp(x[i], cPath))
        {
            res = false;
            break;
        }
    return res;
}

FRESULT delete_node (
    TCHAR* path,    /* Path name buffer with the sub-directory to delete */
    UINT sz_buff,   /* Size of path name buffer (items) */
    FILINFO* fno    /* Name read buffer */
)
{
    UINT i, j;
    FRESULT res;
    DIR dir;

    res = f_opendir(&dir, path); /* Open the sub-directory to make it empty */
    if (res != FR_OK) return res;

    for (i = 0; path[i]; i++) ; /* Get current path length */
    path[i++] = _T('/');

    for (;;) {
        res = f_readdir(&dir, fno);  /* Get a directory item */
        if (res != FR_OK || !fno->fname[0]) break;   /* End of directory? */
        j = 0;
        do {    /* Make a path name */
            if (i + j >= sz_buff) { /* Buffer over flow? */
                res = 100; break;    /* Fails with 100 when buffer overflow */
            }
            path[i + j] = fno->fname[j];
        } while (fno->fname[j++]);
        if (fno->fattrib & AM_DIR) {    /* Item is a sub-directory */
            res = delete_node(path, sz_buff, fno);
        } else {                        /* Item is a file */
            if (canDelete(toLower(path)))
                res = f_unlink(path);
            else
            {
                if (printCRLF)
                {
                    printCRLF = false;
                    gfx_printf("%k\n%k", 0xFF00E100, 0xFFCCCCCC);
                }
                gfx_printf("%kPulando %s!\n%k", 0xFF00E100, path, 0xFFCCCCCC);
            }
        }
        if (res != FR_OK) break;
    }

    path[--i] = 0;  /* Restore the path name */
    f_closedir(&dir);

    if (res == FR_OK)
    {
        if (canDelete(toLower(path)))
            res = f_unlink(path);  /* Delete the empty sub-directory */
        else
            gfx_printf("%kPulando %s!\n%k", 0xFF00E100, path, 0xFFCCCCCC);
    }

    return res;
}

FRESULT performCleanup()
{
    WPRINTF("\nRemovendo tudo do antigo pacote...");

    FRESULT res;
    DIR dir;
    static FILINFO fno;

    char path[2];
    strcpy(path, "/");

    res = f_opendir(&dir, path);
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);
            if (res != FR_OK || fno.fname[0] == 0) break;

            char *tmp1, *tmp2, *file;
            tmp1 = malloc(2);
            tmp2 = malloc(sizeof(fno.fname));
            file = malloc(sizeof(tmp1) + sizeof(tmp2));
            strcpy(tmp1, "/");
            strcpy(tmp2, fno.fname);
            strcpy(file, tmp1);
            strcat(file, tmp2);

            printCRLF = false;

            if (fno.fattrib & AM_DIR)
            {
                if (canDelete(toLower(file)))
                {
                    printCRLF = true;
                    gfx_printf("%kApagando %s... %k", 0xFFFF0000, fno.fname, 0xFFCCCCCC);
                    TCHAR buff[256];
                    strcpy(buff, file);
                    delete_node(buff, sizeof buff / sizeof buff[0], &fno);
                    gfx_printf("%kOK!\n%k", 0xFFFF0000, 0xFFCCCCCC);
                }
                else
                    gfx_printf("%kPulando %s!\n%k", 0xFF00E100, fno.fname, 0xFFCCCCCC);
            }
            else
            {
                if (canDelete(toLower(file)))
                {
                    gfx_printf("%kApagando %s... %k", 0xFFFF0000, fno.fname, 0xFFCCCCCC);
                    f_unlink(file);
                    gfx_printf("%kOK!\n%k", 0xFFFF0000, 0xFFCCCCCC);
                }
                else
                    gfx_printf("%kPulando %s!\n%k", 0xFF00E100, fno.fname, 0xFFCCCCCC);
            }
        }
        f_closedir(&dir);
    }
    return res;
}

FRESULT move_node (
    TCHAR* path,    /* Path name buffer with the sub-directory to delete */
    UINT sz_buff,   /* Size of path name buffer (items) */
    FILINFO* fno    /* Name read buffer */
)
{
    UINT i, j;
    FRESULT res;
    DIR dir;

    res = f_opendir(&dir, path); /* Open the sub-directory to make it empty */
    if (res != FR_OK) return res;

    for (i = 0; path[i]; i++) ; /* Get current path length */
    path[i++] = _T('/');

    for (;;) {
        res = f_readdir(&dir, fno);  /* Get a directory item */
        if (res != FR_OK || !fno->fname[0]) break;   /* End of directory? */
        j = 0;
        do {    /* Make a path name */
            if (i + j >= sz_buff) { /* Buffer over flow? */
                res = 100; break;    /* Fails with 100 when buffer overflow */
            }
            path[i + j] = fno->fname[j];
        } while (fno->fname[j++]);

        if (fno->fattrib & AM_DIR)
		{    /* Item is a sub-directory */
            gfx_printf("%kPasta %s!\n%k", 0xFF03adfc, path, 0xFFCCCCCC);
            res = move_node(path, sz_buff, fno);
        }
		else {                        /* Item is a file */
            TCHAR dest[256] = {0};
            memcpy(dest, path + 17, sizeof(TCHAR) * 256);

            if (f_stat(dest, NULL) == FR_OK) //file already exists... Delete origin!
            {
                gfx_printf("%kPulando %s!\n%k", 0xFF00E100, dest, 0xFFCCCCCC);
//                res = f_unlink(path);
            }
            else //file does not exist... Move it to destination!
			{
                    gfx_printf("%kMovendo %s!\n%k", 0xFFf7d53b, dest, 0xFFCCCCCC);
//                res = f_rename(path, dest);
			}
        }
        if (res != FR_OK) break;
    }

    path[--i] = 0;  /* Restore the path name */
    f_closedir(&dir);

    return res;
}

FRESULT finishUpdate()
{
    WPRINTF("\nMovendo arquivos do novo pacote...\n");

    FRESULT res;
    DIR dir;
    static FILINFO fno;

    char path[18];
    strcpy(path, "/apgtmppackfolder");

    char *tmp1;
    tmp1 = malloc(19);
    strcpy(tmp1, "/apgtmppackfolder/");

    res = f_opendir(&dir, path);
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);
            if (res != FR_OK || fno.fname[0] == 0) break;

            char *tmp2, *tmpPath;
            tmp2 = malloc(sizeof(fno.fname));
            tmpPath = malloc(sizeof(tmp1) + sizeof(tmp2));
            strcpy(tmp2, fno.fname);
            strcpy(tmpPath, tmp1);
            strcat(tmpPath, tmp2);

            if (fno.fattrib & AM_DIR)
            {
                TCHAR updPath[256];
                strcpy(updPath, tmpPath);

                gfx_printf("%kPasta %s!\n%k", 0xFF03adfc, updPath, 0xFFCCCCCC);
                res = move_node(updPath, sizeof updPath / sizeof updPath[0], &fno);
            }
            else
            {
                char *tmp2, *tmp3, *orig, *dest;

                tmp2 = malloc(2);
                strcpy(tmp2, "/");

                tmp3 = malloc(sizeof(fno.fname));
                strcpy(tmp3, fno.fname);

                orig = malloc(sizeof(tmp1) + sizeof(tmp3));
                strcpy(orig, tmp1);
                strcat(orig, tmp3);

                dest = malloc(sizeof(tmp2) + sizeof(tmp3));
                strcpy(dest, tmp2);
                strcat(dest, tmp3);

                if (f_stat(dest, NULL) == FR_OK)
                {
                    gfx_printf("%kPulando %s!\n%k", 0xFF00E100, dest, 0xFFCCCCCC);
//                    res = f_unlink(orig);
                }
                else
				{
                    gfx_printf("%kMovendo %s!\n%k", 0xFFf803fc, dest, 0xFFCCCCCC);
//                    res = f_rename(orig, dest);
				}
            }
        }
        f_closedir(&dir);
    }
	else
		gfx_printf("%k\nERRO AO ABRIR DIR%s!\n%k", 0xFFf803fc, 0xFFCCCCCC);

//    res = delete_node(tmp1, sizeof tmp1 / sizeof tmp1[0], &fno);

    return res;
}

extern void pivot_stack(u32 stack_top);

void ipl_main()
{
    // Do initial HW configuration. This is compatible with consecutive reruns without a reset.
    hw_init();

    // Pivot the stack so we have enough space.
    pivot_stack(IPL_STACK_TOP);

    // Tegra/Horizon configuration goes to 0x80000000+, package2 goes to 0xA9800000, we place our heap in between.
    heap_init(IPL_HEAP_START);

#ifdef DEBUG_UART_PORT
    uart_send(DEBUG_UART_PORT, (u8 *)"hekate: Hello!\r\n", 16);
    uart_wait_idle(DEBUG_UART_PORT, UART_TX_IDLE);
#endif

    // Check if battery is enough.
    _check_low_battery();

    // Set bootloader's default configuration.
    set_default_configuration();

    // Mount SD Card.
    h_cfg.errors |= !sd_mount() ? ERR_SD_BOOT_EN : 0;

    // Save sdram lp0 config.
    void *sdram_params =
        hw_get_chip_id() == GP_HIDREV_MAJOR_T210 ? sdram_get_params_patched() : sdram_get_params_t210b01();
    if (!ianos_loader("bootloader/sys/libsys_lp0.bso", DRAM_LIB, sdram_params))
        h_cfg.errors |= ERR_LIBSYS_LP0;

    // Train DRAM and switch to max frequency.
    if (minerva_init()) //!TODO: Add Tegra210B01 support to minerva.
        h_cfg.errors |= ERR_LIBSYS_MTC;

    display_init();

    u32 *fb = display_init_framebuffer_pitch();
    gfx_init_ctxt(fb, 720, 1280, 720);

    gfx_con_init();

    display_backlight_pwm_init();
    //display_backlight_brightness(h_cfg.backlight, 1000);

    // Overclock BPMP.
    bpmp_clk_rate_set(BPMP_CLK_DEFAULT_BOOST);

    // Clear Minverva missing errors
    h_cfg.errors &= ~ERR_LIBSYS_LP0;
    h_cfg.errors &= ~ERR_LIBSYS_MTC;
    
    // Show exception, library errors and L4T kernel panics.
    _show_errors();

    // Failed to launch Nyx, unmount SD Card.
    sd_end();

    minerva_change_freq(FREQ_800);

    gfx_clear_grey(0x1B);
    display_backlight_brightness(150, 1000);
    WPRINTF("Executando payload atualizador RCM...\n");

    FRESULT res = FR_OK;

    if (sd_mount()) {
/*
msleep(1000);
btn_wait();
easy_rename("payload.bin.apg", "payload.bin");
power_set_state(POWER_OFF);
*/

        // clean install? delete everything but emuMMC and Emutendo folders
        const char* filename = "/cleaninstall.flag";
        if (f_stat(filename, NULL) == FR_OK)
        {
            //reading cleaninstall file to get the app name
            FIL fil;
            FRESULT fr;
            fr = f_open(&fil, filename, FA_READ);
            if (!fr)
                while (f_gets(appName, sizeof appName, &fil))
                    break;
            f_close(&fil);

/*
            //reading cleaninstall file to get the app name
            char file[256] = "/config/";
            strcat(file, appName);
            strcat(file, "/lista_exclusao.txt");
            const char* filename = file;

            if (f_stat(filename, NULL) == FR_OK)
            {
                char line[100];
                fr = f_open(&fil, filename, FA_READ);
                if (!fr)
                    while (f_gets(line, sizeof line, &fil))
                    {
                        line[strcspn(line, "\n")] = 0;
                        strcat(exclusion_list, line);
                        strcat(exclusion_list, "#");
                    }
                f_close(&fil);
            }
*/


FRESULT res = finishUpdate();
if (res == FR_OK)
    WPRINTF("\nTudo pronto... continuando!\n");
else
{
    gfx_printf("%k\nDeu erro! RES = %d\n%k", 0xFF00E100, res, 0xFFCCCCCC);

    gfx_printf("%kRES FR_DISK_ERR = %d\n%k", 0xFF00E100, FR_DISK_ERR, 0xFFCCCCCC);
    gfx_printf("%kRES FR_INT_ERR = %d\n%k", 0xFF00E100, FR_INT_ERR, 0xFFCCCCCC);
    gfx_printf("%kRES FR_NOT_READY = %d\n%k", 0xFF00E100, FR_NOT_READY, 0xFFCCCCCC);
    gfx_printf("%kRES FR_NO_FILE = %d\n%k", 0xFF00E100, FR_NO_FILE, 0xFFCCCCCC);
    gfx_printf("%kRES FR_NO_PATH = %d\n%k", 0xFF00E100, FR_NO_PATH, 0xFFCCCCCC);
    gfx_printf("%kRES FR_INVALID_NAME = %d\n%k", 0xFF00E100, FR_INVALID_NAME, 0xFFCCCCCC);  // <---------------------
    gfx_printf("%kRES FR_DENIED = %d\n%k", 0xFF00E100, FR_DENIED, 0xFFCCCCCC);
    gfx_printf("%kRES FR_EXIST = %d\n%k", 0xFF00E100, FR_EXIST, 0xFFCCCCCC);
    gfx_printf("%kRES FR_INVALID_OBJECT = %d\n%k", 0xFF00E100, FR_INVALID_OBJECT, 0xFFCCCCCC);
    gfx_printf("%kRES FR_WRITE_PROTECTED = %d\n%k", 0xFF00E100, FR_WRITE_PROTECTED, 0xFFCCCCCC);
    gfx_printf("%kRES FR_INVALID_DRIVE = %d\n%k", 0xFF00E100, FR_INVALID_DRIVE, 0xFFCCCCCC);
    gfx_printf("%kRES FR_NOT_ENABLED = %d\n%k", 0xFF00E100, FR_NOT_ENABLED, 0xFFCCCCCC);
    gfx_printf("%kRES FR_NO_FILESYSTEM = %d\n%k", 0xFF00E100, FR_NO_FILESYSTEM, 0xFFCCCCCC);
    gfx_printf("%kRES FR_MKFS_ABORTED = %d\n%k", 0xFF00E100, FR_MKFS_ABORTED, 0xFFCCCCCC);
    gfx_printf("%kRES FR_TIMEOUT = %d\n%k", 0xFF00E100, FR_TIMEOUT, 0xFFCCCCCC);
    gfx_printf("%kRES FR_LOCKED = %d\n%k", 0xFF00E100, FR_LOCKED, 0xFFCCCCCC);
    gfx_printf("%kRES FR_NOT_ENOUGH_CORE = %d\n%k", 0xFF00E100, FR_NOT_ENOUGH_CORE, 0xFFCCCCCC);
    gfx_printf("%kRES FR_TOO_MANY_OPEN_FILES = %d\n%k", 0xFF00E100, FR_TOO_MANY_OPEN_FILES, 0xFFCCCCCC);
    gfx_printf("%kRES FR_INVALID_PARAMETER = %d\n%k", 0xFF00E100, FR_INVALID_PARAMETER, 0xFFCCCCCC);
}
msleep(1000);
btn_wait();
//easy_rename("payload.bin.apg", "payload.bin");
power_set_state(POWER_OFF);







            //removing flag file
            f_unlink(filename);

            //cleaning up the SD
            res = performCleanup();
        }

        //checking wheter the cleanup was successfull
        if (res == FR_OK)
        {
            static FILINFO fno;
            TCHAR buff[18];
            strcpy(buff, "/apgtmppackfolder");
            if (finishUpdate(buff, sizeof buff / sizeof buff[0], &fno) == FR_OK)
            {
                WPRINTF("\nTudo pronto... continuando!\n");
                msleep(500);


msleep(1000);
btn_wait();


                if (f_stat("atmosphere/fusee-secondary.bin.apg", NULL) == FR_OK)
                    easy_rename("atmosphere/fusee-secondary.bin.apg", "atmosphere/fusee-secondary.bin");
                if (f_stat("sept/payload.bin.apg", NULL) == FR_OK)
                    easy_rename("sept/payload.bin.apg", "sept/payload.bin");
                if (f_stat("atmosphere/stratosphere.romfs.apg", NULL) == FR_OK)
                    easy_rename("atmosphere/stratosphere.romfs.apg", "atmosphere/stratosphere.romfs");
                if (f_stat("atmosphere/package3.apg", NULL) == FR_OK)
                    easy_rename("atmosphere/package3.apg", "atmosphere/package3");

                // If the console is a patched or Mariko unit
                if (h_cfg.t210b01 || h_cfg.rcm_patched) {
                    easy_rename("payload.bin.apg", "payload.bin");
                    power_set_state(POWER_OFF_REBOOT);
                }
                else {
                    if (f_stat("bootloader/update.bin", NULL) == FR_OK)
                        launch_payload("bootloader/update.bin", false);

                    if (f_stat("atmosphere/reboot_payload.bin", NULL) == FR_OK)    
                        launch_payload("atmosphere/reboot_payload.bin", false);

                    EPRINTF("Falha ao executar o payload!");
                }
            }
            else
            {
                EPRINTF("\nFalha ao mover arquivos do novo pacote!\n\n");
                WPRINTF("Procure assistencia tecnica ou reconfigure");
                WPRINTF("seu microSD manualmente.");
                WPRINTF("Processo interrompido!");
            }
        }
        else
        {
            EPRINTF("\nFalha ao executar limpeza!\n\n");
            WPRINTF("Verifique se o console inicia corretamente.");
            WPRINTF("Se ele não iniciar procure assistencia ou");
            WPRINTF("configure seu microSD manualmente.");
            WPRINTF("Processo interrompido!");
        }
    }

    // Halt BPMP if we managed to get out of execution.
    while (true)
        bpmp_halt();
}
