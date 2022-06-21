#include "tools_tab.hpp"

#include <filesystem>
#include <fstream>

#include "changelog_page.hpp"
#include "icons_page.hpp"
#include "confirm_page.hpp"
#include "extract.hpp"
#include "fs.hpp"
#include "net_page.hpp"
#include "utils.hpp"
#include "worker_page.hpp"
#include "download.hpp"

#include <chrono>

namespace i18n = brls::i18n;
using namespace i18n::literals;
using json = nlohmann::json;

namespace {
    constexpr const char AppVersion[] = APP_VERSION;
}

ToolsTab::ToolsTab(const std::string& tag, bool erista) : brls::List()
{
/*
    if (!tag.empty()) {
        //fetching the version as a number
        std::string temp = "";
        int iTag = 0;
        int iAppVersion = 0;

        temp.reserve(tag.size()); // optional, avoids buffer reallocations in the loop
        for(size_t i = 0; i < tag.size(); ++i)
            if(tag[i] != '.') temp += tag[i]; // removing the . from the version
        iTag = std::stoi(temp); // casting from string to integer

        temp = "";

        temp.reserve(strlen(AppVersion)); // optional, avoids buffer reallocations in the loop
        for(size_t i = 0; i < strlen(AppVersion); ++i)
            if(AppVersion[i] != '.') temp += AppVersion[i]; // removing the . from the version
        iAppVersion = std::stoi(temp); // casting from string to integer

        if (iTag > iAppVersion) {
            brls::ListItem* updateApp = new brls::ListItem(fmt::format("menus/tools/update_app"_i18n, tag));
            std::string text(fmt::format("menus/tools/dl_app"_i18n, APP_FULL_NAME, tag));
            updateApp->getClickEvent()->subscribe([text, tag](brls::View* view) {
                brls::StagedAppletFrame* stagedFrame = new brls::StagedAppletFrame();
                stagedFrame->setTitle("menus/common/updating"_i18n);
                stagedFrame->addStage(new ConfirmPage(stagedFrame, text));
                stagedFrame->addStage(new WorkerPage(stagedFrame, "menus/common/downloading"_i18n, []() { util::downloadArchive(fmt::format(APP_URL, GITHUB_USER, BASE_FOLDER_NAME, BASE_FOLDER_NAME), contentType::app); }));
                stagedFrame->addStage(new WorkerPage(stagedFrame, "menus/common/extracting"_i18n, []() { util::extractArchive(contentType::app); }));
                stagedFrame->addStage(new ConfirmPage(stagedFrame, "menus/common/all_done"_i18n, true));
                brls::Application::pushView(stagedFrame);
            });
            updateApp->setHeight(LISTITEM_HEIGHT);
            this->addView(updateApp);
        }
    }
*/
    brls::ListItem* netSettings = new brls::ListItem("menus/tools/internet_settings"_i18n);
    netSettings->getClickEvent()->subscribe([](brls::View* view) {
        brls::PopupFrame::open("menus/tools/internet_settings"_i18n, new NetPage(), "", "");
    });
    netSettings->setHeight(LISTITEM_HEIGHT);

    brls::ListItem* browser = new brls::ListItem("menus/tools/browser"_i18n);
    browser->getClickEvent()->subscribe([](brls::View* view) {
        std::string url;
        if (brls::Swkbd::openForText([&url](std::string text) { url = text; }, "cheatslips.com e-mail", "", 64, "https://duckduckgo.com", 0, "Submit", "https://website.tld")) {
            std::string error = "";
            int at = appletGetAppletType();
            if (at == AppletType_Application) {    // Running as a title
                WebCommonConfig conf;
                WebCommonReply out;
                Result rc = webPageCreate(&conf, url.c_str());
                if (R_FAILED(rc))
                    error += "\uE016 Error starting Browser\n\uE016 Lookup error code for more info " + rc;
                webConfigSetJsExtension(&conf, true);
                webConfigSetPageCache(&conf, true);
                webConfigSetBootLoadingIcon(&conf, true);
                webConfigSetWhitelist(&conf, ".*");
                rc = webConfigShow(&conf, &out);
                if (R_FAILED(rc))
                    error += "\uE016 Error starting Browser\n\uE016 Lookup error code for more info " + rc;
            }
            else {  // Running under applet
                error += "\uE016 Running in applet mode/through a forwarder.\n\uE016 Please launch hbmenu by holding [R] on a game";
            }
            if (!error.empty()) {
                util::showDialogBoxInfo(error);
            }
        }
    });
    browser->setHeight(LISTITEM_HEIGHT);

    brls::ListItem* cleanUp = new brls::ListItem("menus/tools/clean_up"_i18n);
    cleanUp->getClickEvent()->subscribe([](brls::View* view) {
        util::cleanFiles();
        util::showDialogBoxInfo("menus/common/all_done"_i18n);
    });
    cleanUp->setHeight(LISTITEM_HEIGHT);

    brls::ListItem* motd = new brls::ListItem("menus/tools/motd_label"_i18n);

    bool bAlwaysShow;
	std::string sMOTD = util::getMOTD(bAlwaysShow);
    motd->getClickEvent()->subscribe([sMOTD](brls::View* view) {
        util::showDialogBoxInfo(sMOTD);
    });
    motd->setHeight(LISTITEM_HEIGHT);

    brls::ListItem* changelog = new brls::ListItem(fmt::format("{} no homebrew", "menus/changelog/changelog"_i18n));
    changelog->getClickEvent()->subscribe([](brls::View* view) {
        brls::PopupFrame::open(fmt::format("{} no homebrew", "menus/changelog/changelog"_i18n), new ChangelogPage(), "", "");
    });
    changelog->setHeight(LISTITEM_HEIGHT);

    this->addView(netSettings);
    this->addView(browser);
    this->addView(cleanUp);
    if (sMOTD != "")
	    this->addView(motd);
    this->addView(changelog);

    if (DEBUG)
    {
        brls::ListItem* icons = new brls::ListItem("Icones");
        icons->getClickEvent()->subscribe([](brls::View* view) {
            brls::PopupFrame::open("Icones", new IconsPage(), "", "");
        });
        icons->setHeight(LISTITEM_HEIGHT);
        this->addView(icons);

        brls::ListItem* forceCleanInstall = new brls::ListItem("Forçar Instalação Limpa");
        forceCleanInstall->getClickEvent()->subscribe([](brls::View* view) {
            util::createCleanInstallFile();
			util::showDialogBoxInfo("Agora use a opção 'Reiniciar no Payload RCM' para testar a 'Instalação Limpa'.");
        });
        forceCleanInstall->setHeight(LISTITEM_HEIGHT);
        this->addView(forceCleanInstall);

        brls::ListItem* payloadRCM = new brls::ListItem("Reiniciar no Payload RCM");
        payloadRCM->getClickEvent()->subscribe([](brls::View* view) {
            if (util::isErista()) {
                util::rebootToPayload(RCM_PAYLOAD_PATH);
            }
            else {
                if (std::filesystem::exists(UPDATE_BIN_PATH)) {
                    fs::copyFile(UPDATE_BIN_PATH, MARIKO_PAYLOAD_PATH_TEMP);
                }
                else {
                    fs::copyFile(REBOOT_PAYLOAD_PATH, MARIKO_PAYLOAD_PATH_TEMP);
                }
                fs::copyFile(RCM_PAYLOAD_PATH, MARIKO_PAYLOAD_PATH);
                util::shutDown(true);
            }
        });
        payloadRCM->setHeight(LISTITEM_HEIGHT);
        this->addView(payloadRCM);
    }
}
