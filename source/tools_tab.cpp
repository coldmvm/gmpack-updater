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
            updateApp = new brls::ListItem(fmt::format("menus/tools/update_app"_i18n, tag));
            std::string text(fmt::format("menus/tools/dl_app"_i18n, APP_FULL_NAME, tag));
            updateApp->getClickEvent()->subscribe([text, tag](brls::View* view) {
                brls::StagedAppletFrame* stagedFrame = new brls::StagedAppletFrame();
                stagedFrame->setTitle("menus/common/updating"_i18n);
                stagedFrame->addStage(new ConfirmPage(stagedFrame, text));
                stagedFrame->addStage(new WorkerPage(stagedFrame, "menus/common/downloading"_i18n, []() { util::downloadArchive(fmt::format(APP_URL, BASE_WWW_NAME, BASE_FOLDER_NAME, BASE_FOLDER_NAME), contentType::app); }));
                stagedFrame->addStage(new WorkerPage(stagedFrame, "menus/common/extracting"_i18n, []() { util::extractArchive(contentType::app); }));
                stagedFrame->addStage(new ConfirmPage(stagedFrame, "menus/common/all_done"_i18n, true));
                brls::Application::pushView(stagedFrame);
            });
            updateApp->setHeight(LISTITEM_HEIGHT);
            //this->addView(updateApp);
        }
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

    cleanUp = new brls::ListItem("menus/tools/clean_up"_i18n);
    cleanUp->getClickEvent()->subscribe([](brls::View* view) {
        util::cleanFiles();
        util::showDialogBoxInfo("menus/common/all_done"_i18n);
    });
    cleanUp->setHeight(LISTITEM_HEIGHT);

    motd = new brls::ListItem("menus/tools/motd_label"_i18n);
	std::string sMOTD = util::getMOTD();
    motd->getClickEvent()->subscribe([sMOTD](brls::View* view) {
        util::showDialogBoxInfo(sMOTD);
/*
		util::showDialogBoxInfo(
			"\uE000, \uE001, \uE002, \uE003, \uE004, \uE005, \uE006, \uE007, \uE008, \uE009, \uE00A, \uE00B, \uE00C, \uE00D, \uE00E, \uE00F" 
			"\uE010, \uE011, \uE012, \uE013, \uE014, \uE015, \uE016, \uE017, \uE018, \uE019, \uE01A, \uE01B, \uE01C, \uE01D, \uE01E, \uE01F" 
			"\uE020, \uE021, \uE022, \uE023, \uE024, \uE025, \uE026, \uE027, \uE028, \uE029, \uE02A, \uE02B, \uE02C, \uE02D, \uE02E, \uE02F" 
			"\uE030, \uE031, \uE032, \uE033, \uE034, \uE035, \uE036, \uE037, \uE038, \uE039, \uE03A, \uE03B, \uE03C, \uE03D, \uE03E, \uE03F" 
			"\uE040, \uE041, \uE042, \uE043, \uE044, \uE045, \uE046, \uE047, \uE048, \uE049, \uE04A, \uE04B, \uE04C, \uE04D, \uE04E, \uE04F" 
			"\uE050, \uE051, \uE052, \uE053, \uE054, \uE055, \uE056, \uE057, \uE058, \uE059, \uE05A, \uE05B, \uE05C, \uE05D, \uE05E, \uE05F" 
			"\uE060, \uE061, \uE062, \uE063, \uE064, \uE065, \uE066, \uE067, \uE068, \uE069, \uE06A, \uE06B, \uE06C, \uE06D, \uE06E, \uE06F" 
			"\uE070, \uE071, \uE072, \uE073, \uE074, \uE075, \uE076, \uE077, \uE078, \uE079, \uE07A, \uE07B, \uE07C, \uE07D, \uE07E, \uE07F" 
			"\uE080, \uE081, \uE082, \uE083, \uE084, \uE085, \uE086, \uE087, \uE088, \uE089, \uE08A, \uE08B, \uE08C, \uE08D, \uE08E, \uE08F" 
			"\uE090, \uE091, \uE092, \uE093, \uE094, \uE095, \uE096, \uE097, \uE098, \uE099, \uE09A, \uE09B, \uE09C, \uE09D, \uE09E, \uE09F" 
			"\uE0A0, \uE0A1, \uE0A2, \uE0A3, \uE0A4, \uE0A5, \uE0A6, \uE0A7, \uE0A8, \uE0A9, \uE0AA, \uE0AB, \uE0AC, \uE0AD, \uE0AE, \uE0AF" 
			"\uE0B0, \uE0B1, \uE0B2, \uE0B3, \uE0B4, \uE0B5, \uE0B6, \uE0B7, \uE0B8, \uE0B9, \uE0BA, \uE0BB, \uE0BC, \uE0BD, \uE0BE, \uE0BF" 
			"\uE0C0, \uE0C1, \uE0C2, \uE0C3, \uE0C4, \uE0C5, \uE0C6, \uE0C7, \uE0C8, \uE0C9, \uE0CA, \uE0CB, \uE0CC, \uE0CD, \uE0CE, \uE0CF" 
			"\uE0D0, \uE0D1, \uE0D2, \uE0D3, \uE0D4, \uE0D5, \uE0D6, \uE0D7, \uE0D8, \uE0D9, \uE0DA, \uE0DB, \uE0DC, \uE0DD, \uE0DE, \uE0DF" 
			"\uE0E0, \uE0E1, \uE0E2, \uE0E3, \uE0E4, \uE0E5, \uE0E6, \uE0E7, \uE0E8, \uE0E9, \uE0EA, \uE0EB, \uE0EC, \uE0ED, \uE0EE, \uE0EF"
			"\uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF"
		);
*/
    });
    motd->setHeight(LISTITEM_HEIGHT);

    changelog = new brls::ListItem(fmt::format("{} no homebrew", "menus/changelog/changelog"_i18n));
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
}
