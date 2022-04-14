#pragma once

constexpr const char ROOT_PATH[] = "/";
constexpr const char DOWNLOAD_PATH[] = "/config/gmpack-updater/";
constexpr const char CONFIG_PATH[] = "/config/gmpack-updater/";

constexpr const char RCM_PAYLOAD_PATH[] = "romfs:/aio_rcm.bin";
constexpr const char MARIKO_PAYLOAD_PATH[] = "/payload.bin";
constexpr const char MARIKO_PAYLOAD_PATH_TEMP[] = "/payload.bin.aio";

constexpr const char APP_URL[] = "https://github.com/coldmvm/gmpack-updater/releases/latest/download/gmpack-updater.zip";
constexpr const char TAGS_INFO[] = "https://api.github.com/repos/coldmvm/gmpack-updater/releases/latest";
constexpr const char APP_FILENAME[] = "/config/gmpack-updater/app.zip";

constexpr const char NXLINKS_URL[] = "https://raw.githubusercontent.com/coldmvm/nx-links/master/nx-links-new.json";

constexpr const char FIRMWARE_FILENAME[] = "/config/gmpack-updater/firmware.zip";
constexpr const char FIRMWARE_PATH[] = "/firmware/";

constexpr const char AMS_FILENAME[] = "/config/gmpack-updater/ams.zip";

constexpr const char DEEPSEA_META_JSON[] = "https://builder.teamneptune.net/meta.json";
constexpr const char DEEPSEA_BUILD_URL[] = "https://builder.teamneptune.net/build/";
constexpr const char DEEPSEA_PACKAGE_PATH[] = "/config/deepsea/customPackage.json";

constexpr const char CUSTOM_PACKS_PATH[] = "/config/gmpack-updater/customPacks.json";

constexpr const char CHEATS_URL_CONTENTS[] = "https://github.com/coldmvm/switch-cheats-db/releases/latest/download/contents.zip";
constexpr const char CHEATS_URL_VERSION[] = "https://github.com/coldmvm/switch-cheats-db/releases/latest/download/VERSION";
constexpr const char CHEATS_EXCLUDE[] = "/config/gmpack-updater/exclude.txt";
constexpr const char FILES_IGNORE[] = "/config/gmpack-updater/preserve.txt";
constexpr const char INTERNET_JSON[] = "/config/gmpack-updater/internet.json";
constexpr const char CHEATS_VERSION[] = "/config/gmpack-updater/cheats_version.dat";
constexpr const char AMS_CONTENTS[] = "/atmosphere/contents/";
constexpr const char REINX_CONTENTS[] = "/ReiNX/contents/";
constexpr const char SXOS_TITLES[] = "/sxos/titles/";
constexpr const char AMS_PATH[] = "/atmosphere/";
constexpr const char SXOS_PATH[] = "/sxos/";
constexpr const char REINX_PATH[] = "/ReiNX/";
constexpr const char CONTENTS_PATH[] = "contents/";
constexpr const char TITLES_PATH[] = "titles/";

constexpr const char PAYLOAD_PATH[] = "/payloads/";
constexpr const char BOOTLOADER_PATH[] = "/bootloader/";
constexpr const char BOOTLOADER_PL_PATH[] = "/bootloader/payloads/";
constexpr const char UPDATE_BIN_PATH[] = "/bootloader/update.bin";
//
constexpr const char REBOOT_PAYLOAD_PATH[] = "/atmosphere/reboot_payload.bin";
constexpr const char FUSEE_SECONDARY[] = "/atmosphere/fusee-secondary.bin";
constexpr const char FUSEE_MTC[] = "/atmosphere/fusee-mtc.bin";

constexpr const char AMS_ZIP_PATH[] = "/config/gmpack-updater/ams.zip";
constexpr const char APP_ZIP_PATH[] = "/config/gmpack-updater/app.zip";
constexpr const char CFW_ZIP_PATH[] = "/config/gmpack-updater/cfw.zip";
constexpr const char FW_ZIP_PATH[] = "/config/gmpack-updater/firmware.zip";
constexpr const char CHEATS_ZIP_PATH[] = "/config/gmpack-updater/cheats.zip";
constexpr const char SIGPATCHES_ZIP_PATH[] = "/config/gmpack-updater/sigpatches.zip";
constexpr const char AMS_DIRECTORY_PATH[] = "/config/gmpack-updater/atmosphere/";
constexpr const char SEPT_DIRECTORY_PATH[] = "/config/gmpack-updater/sept/";
constexpr const char FW_DIRECTORY_PATH[] = "/firmware/";

constexpr const char HIDE_TABS_JSON[] = "/config/gmpack-updater/hide_tabs.json";
constexpr const char LANGUAGE_JSON[] = "/config/gmpack-updater/language.json";

constexpr const char ROMFS_PATH[] = "romfs:/";
constexpr const char ROMFS_FORWARDER[] = "romfs:/gmpack-forwarder.nro";
constexpr const char FORWARDER_PATH[] = "/config/gmpack-updater/gmpack-forwarder.nro";

constexpr const char DAYBREAK_PATH[] = "/switch/daybreak.nro";

constexpr const char HIDDEN_AIO_FILE[] = "/config/gmpack-updater/.gmpack-updater";

constexpr const int LISTITEM_HEIGHT = 50;

enum class contentType
{
    fw,
    app,
    ams_cfw,
};

constexpr std::string_view contentTypeNames[3]{"firmwares", "app", "cfws"};

enum class CFW
{
    rnx,
    sxos,
    ams,
};