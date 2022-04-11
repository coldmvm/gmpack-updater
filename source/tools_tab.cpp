#include "tools_tab.hpp"

#include <filesystem>
#include <fstream>

#include "changelog_page.hpp"
#include "confirm_page.hpp"
#include "extract.hpp"
#include "fs.hpp"
#include "net_page.hpp"
#include "utils.hpp"
#include "worker_page.hpp"

namespace i18n = brls::i18n;
using namespace i18n::literals;
using json = nlohmann::json;

namespace {
    constexpr const char AppVersion[] = APP_VERSION;
}

ToolsTab::ToolsTab(const std::string& tag, const nlohmann::ordered_json& payloads, bool erista, const nlohmann::json& hideStatus) : brls::List()
{
    if (!tag.empty() && tag != AppVersion) {
        updateApp = new brls::ListItem(fmt::format("menus/tools/update_app"_i18n, tag));
        //std::string text("menus/tools/dl_app"_i18n + std::string(APP_URL));
		std::string text(fmt::format("menus/tools/dl_app"_i18n, tag));
        updateApp->getClickEvent()->subscribe([text, tag](brls::View* view) {
            brls::StagedAppletFrame* stagedFrame = new brls::StagedAppletFrame();
            stagedFrame->setTitle("menus/common/updating"_i18n);
            stagedFrame->addStage(
                new ConfirmPage(stagedFrame, text));
            stagedFrame->addStage(
                new WorkerPage(stagedFrame, "menus/common/downloading"_i18n, []() { util::downloadArchive(APP_URL, contentType::app); }));
            stagedFrame->addStage(
                new WorkerPage(stagedFrame, "menus/common/extracting"_i18n, []() { util::extractArchive(contentType::app); }));
            stagedFrame->addStage(
                new ConfirmPage(stagedFrame, "menus/common/all_done"_i18n, true));
            brls::Application::pushView(stagedFrame);
        });
        updateApp->setHeight(LISTITEM_HEIGHT);
        this->addView(updateApp);
    }

    netSettings = new brls::ListItem("menus/tools/internet_settings"_i18n);
    netSettings->getClickEvent()->subscribe([](brls::View* view) {
        brls::PopupFrame::open("menus/tools/internet_settings"_i18n, new NetPage(), "", "");
    });
    netSettings->setHeight(LISTITEM_HEIGHT);

    browser = new brls::ListItem("menus/tools/browser"_i18n);
    browser->getClickEvent()->subscribe([](brls::View* view) {
        std::string url;
        if (brls::Swkbd::openForText([&url](std::string text) { url = text; }, "cheatslips.com e-mail", "", 64, "https://duckduckgo.com", 0, "Submit", "https://website.tld")) {
            std::string error = "";
            int at = appletGetAppletType();
            if (at == AppletType_Application) {// Running as a title
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

    cleanUp = new brls::ListItem("menus/tools/clean_up"_i18n);
    cleanUp->getClickEvent()->subscribe([](brls::View* view) {
        std::filesystem::remove(AMS_ZIP_PATH);
        std::filesystem::remove(APP_ZIP_PATH);
        std::filesystem::remove(CFW_ZIP_PATH);
        std::filesystem::remove(FW_ZIP_PATH);
        std::filesystem::remove(CHEATS_ZIP_PATH);
        std::filesystem::remove(CHEATS_VERSION);
        std::filesystem::remove(SIGPATCHES_ZIP_PATH);
        fs::removeDir(AMS_DIRECTORY_PATH);
        fs::removeDir(SEPT_DIRECTORY_PATH);
        fs::removeDir(FW_DIRECTORY_PATH);
        util::showDialogBoxInfo("menus/common/all_done"_i18n);
    });
    cleanUp->setHeight(LISTITEM_HEIGHT);

    changelog = new brls::ListItem("menus/tools/changelog"_i18n);
    changelog->getClickEvent()->subscribe([](brls::View* view) {
        brls::PopupFrame::open("menus/tools/changelog"_i18n, new ChangelogPage(), "", "");
    });
    changelog->setHeight(LISTITEM_HEIGHT);

    if (!util::getBoolValue(hideStatus, "netsettings")) this->addView(netSettings);
    if (!util::getBoolValue(hideStatus, "browser")) this->addView(browser);
    if (!util::getBoolValue(hideStatus, "cleanup")) this->addView(cleanUp);
    this->addView(changelog);
}
