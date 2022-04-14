#include "app_page.hpp"

#include <switch.h>

#include <filesystem>
#include <fstream>
#include <regex>

#include "confirm_page.hpp"
#include "current_cfw.hpp"
#include "extract.hpp"
#include "fs.hpp"
#include "utils.hpp"
#include "worker_page.hpp"

namespace i18n = brls::i18n;
using namespace i18n::literals;

AppPage::AppPage() : AppletFrame(true, true)
{
    list = new brls::List();
}

void AppPage::PopulatePage()
{
    this->CreateLabel();

    NsApplicationRecord* records = new NsApplicationRecord[MaxTitleCount];
    NsApplicationControlData* controlData = NULL;
    std::string name;

    s32 recordCount = 0;
    u64 controlSize = 0;
    u64 tid;

    if (!util::isApplet()) {
        if (R_SUCCEEDED(nsListApplicationRecord(records, MaxTitleCount, 0, &recordCount))) {
            for (s32 i = 0; i < recordCount; i++) {
                controlSize = 0;

                if (R_FAILED(InitControlData(&controlData))) break;

                tid = records[i].application_id;

                if R_FAILED (GetControlData(tid, controlData, controlSize, name)) continue;

                listItem = new brls::ListItem(name, "", util::formatApplicationId(tid));
                this->DeclareGameListItem(name, tid, &controlData);
            }
            free(controlData);
        }
    }
    else {
        tid = GetCurrentApplicationId();
        if (R_SUCCEEDED(InitControlData(&controlData)) && R_SUCCEEDED(GetControlData(tid & 0xFFFFFFFFFFFFF000, controlData, controlSize, name))) {
            listItem = new brls::ListItem(name, "", util::formatApplicationId(tid));
            this->DeclareGameListItem(name, tid, &controlData);
        }
        label = new brls::Label(brls::LabelStyle::SMALL, "menus/common/applet_mode_not_supported"_i18n, true);
        list->addView(label);
    }
    delete[] records;

    this->setContentView(list);
}

u32 AppPage::InitControlData(NsApplicationControlData** controlData)
{
    free(*controlData);
    *controlData = (NsApplicationControlData*)malloc(sizeof(NsApplicationControlData));
    if (*controlData == NULL) {
        return 300;
    }
    else {
        memset(*controlData, 0, sizeof(NsApplicationControlData));
        return 0;
    }
}

u32 AppPage::GetControlData(u64 tid, NsApplicationControlData* controlData, u64& controlSize, std::string& name)
{
    Result rc;
    NacpLanguageEntry* langEntry = NULL;
    rc = nsGetApplicationControlData(NsApplicationControlSource_Storage, tid, controlData, sizeof(NsApplicationControlData), &controlSize);
    if (R_FAILED(rc)) return rc;

    if (controlSize < sizeof(controlData->nacp)) return 100;

    rc = nacpGetLanguageEntry(&controlData->nacp, &langEntry);
    if (R_FAILED(rc)) return rc;

    if (!langEntry->name) return 200;

    name = langEntry->name;

    return 0;
}

void AppPage::DeclareGameListItem(const std::string& name, u64 tid, NsApplicationControlData** controlData)
{
    listItem->setThumbnail((*controlData)->icon, sizeof((*controlData)->icon));
    list->addView(listItem);
}

uint64_t AppPage::GetCurrentApplicationId()
{
    Result rc = 0;
    uint64_t pid = 0;
    uint64_t tid = 0;

    rc = pmdmntGetApplicationProcessId(&pid);
    if (rc == 0x20f || R_FAILED(rc)) return 0;

    rc = pminfoGetProgramId(&tid, pid);
    if (rc == 0x20f || R_FAILED(rc)) return 0;

    return tid;
}

AppPage_Exclude::AppPage_Exclude() : AppPage()
{
    this->PopulatePage();
}

void AppPage_Exclude::CreateLabel()
{
    this->setTitle("menus/cheats/exclude_titles"_i18n);
    label = new brls::Label(brls::LabelStyle::DESCRIPTION, "menus/cheats/exclude_titles_desc"_i18n, true);
    list->addView(label);
}

void AppPage_Exclude::PopulatePage()
{
    this->CreateLabel();

    NsApplicationRecord* records = new NsApplicationRecord[MaxTitleCount];
    NsApplicationControlData* controlData = NULL;
    std::string name;

    s32 recordCount = 0;
    u64 controlSize = 0;
    u64 tid;

    auto titles = fs::readLineByLine(CHEATS_EXCLUDE);

    if (!util::isApplet()) {
        if (R_SUCCEEDED(nsListApplicationRecord(records, MaxTitleCount, 0, &recordCount))) {
            for (s32 i = 0; i < recordCount; i++) {
                controlSize = 0;

                if (R_FAILED(InitControlData(&controlData))) break;

                tid = records[i].application_id;

                if R_FAILED (GetControlData(tid, controlData, controlSize, name)) continue;

                brls::ToggleListItem* listItem;
                listItem = new brls::ToggleListItem(std::string(name), titles.find(util::formatApplicationId(tid)) != titles.end() ? 0 : 1);

                listItem->setThumbnail(controlData->icon, sizeof(controlData->icon));
                items.insert(std::make_pair(listItem, util::formatApplicationId(tid)));
                list->addView(listItem);
            }
            free(controlData);
        }
    }
    else {
        label = new brls::Label(brls::LabelStyle::SMALL, "menus/common/applet_mode_not_supported"_i18n, true);
        list->addView(label);
    }
    delete[] records;

    list->registerAction("menus/cheats/exclude_titles_save"_i18n, brls::Key::B, [this] {
        std::set<std::string> exclude;
        for (const auto& item : items) {
            if (!item.first->getToggleState()) {
                exclude.insert(item.second);
            }
        }
        extract::writeTitlesToFile(exclude, CHEATS_EXCLUDE);
        brls::Application::popView();
        return true;
    });

    this->setContentView(list);
}
