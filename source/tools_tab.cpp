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
                stagedFrame->addStage(new WorkerPage(stagedFrame, "menus/common/downloading"_i18n, []() { util::downloadArchive(fmt::format(APP_URL, GITHUB_USER, BASE_FOLDER_NAME, BASE_FOLDER_NAME), contentType::app); }));
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

/*
ICONS TEMP

    brls::ListItem* icons1 = new brls::ListItem("Icones 1");
    icons1->getClickEvent()->subscribe([](brls::View* view) {
		util::showDialogBoxInfo(
			"\uE000, \uE001, \uE002, \uE003, \uE004, \uE005, \uE006, \uE007, \uE008, \uE009, \uE00A, \uE00B, \uE00C, \uE00D, \uE00E, \uE00F"
			"\uE010, \uE011, \uE012, \uE013, \uE014, \uE015, \uE016, \uE017, \uE018, \uE019, \uE01A, \uE01B, \uE01C, \uE01D, \uE01E, \uE01F"
			"\uE020, \uE021, \uE022, \uE023, \uE024, \uE025, \uE026, \uE027, \uE028, \uE029, \uE02A, \uE02B, \uE02C, \uE02D, \uE02E, \uE02F"
			"\uE030, \uE031, \uE032, \uE033, \uE034, \uE035, \uE036, \uE037, \uE038, \uE039, \uE03A, \uE03B, \uE03C, \uE03D, \uE03E, \uE03F"
			"\uE040, \uE041, \uE042, \uE043, \uE044, \uE045, \uE046, \uE047, \uE048, \uE049, \uE04A, \uE04B, \uE04C, \uE04D, \uE04E, \uE04F"
			"\uE050, \uE051, \uE052, \uE053, \uE054, \uE055, \uE056, \uE057, \uE058, \uE059, \uE05A, \uE05B, \uE05C, \uE05D, \uE05E, \uE05F"
			"\uE060, \uE061, \uE062, \uE063, \uE064, \uE065, \uE066, \uE067, \uE068, \uE069, \uE06A, \uE06B, \uE06C, \uE06D, \uE06E, \uE06F"
			"\uE070, \uE071, \uE072, \uE073, \uE074, \uE075, \uE076, \uE077, \uE078, \uE079, \uE07A, \uE07B, \uE07C, \uE07D, \uE07E, \uE07F"
		);
    });
    icons1->setHeight(LISTITEM_HEIGHT);
    this->addView(icons1);

    brls::ListItem* icons2 = new brls::ListItem("Icones 2");
    icons2->getClickEvent()->subscribe([](brls::View* view) {
		util::showDialogBoxInfo(
			"\uE080, \uE081, \uE082, \uE083, \uE084, \uE085, \uE086, \uE087, \uE088, \uE089, \uE08A, \uE08B, \uE08C, \uE08D, \uE08E, \uE08F"
			"\uE090, \uE091, \uE092, \uE093, \uE094, \uE095, \uE096, \uE097, \uE098, \uE099, \uE09A, \uE09B, \uE09C, \uE09D, \uE09E, \uE09F"
			"\uE0A0, \uE0A1, \uE0A2, \uE0A3, \uE0A4, \uE0A5, \uE0A6, \uE0A7, \uE0A8, \uE0A9, \uE0AA, \uE0AB, \uE0AC, \uE0AD, \uE0AE, \uE0AF"
			"\uE0B0, \uE0B1, \uE0B2, \uE0B3, \uE0B4, \uE0B5, \uE0B6, \uE0B7, \uE0B8, \uE0B9, \uE0BA, \uE0BB, \uE0BC, \uE0BD, \uE0BE, \uE0BF"
			"\uE0C0, \uE0C1, \uE0C2, \uE0C3, \uE0C4, \uE0C5, \uE0C6, \uE0C7, \uE0C8, \uE0C9, \uE0CA, \uE0CB, \uE0CC, \uE0CD, \uE0CE, \uE0CF"
			"\uE0D0, \uE0D1, \uE0D2, \uE0D3, \uE0D4, \uE0D5, \uE0D6, \uE0D7, \uE0D8, \uE0D9, \uE0DA, \uE0DB, \uE0DC, \uE0DD, \uE0DE, \uE0DF"
			"\uE0E0, \uE0E1, \uE0E2, \uE0E3, \uE0E4, \uE0E5, \uE0E6, \uE0E7, \uE0E8, \uE0E9, \uE0EA, \uE0EB, \uE0EC, \uE0ED, \uE0EE, \uE0EF"
			"\uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF, \uE0FF"
		);
    });
    icons2->setHeight(LISTITEM_HEIGHT);
    this->addView(icons2);

    brls::ListItem* icons3 = new brls::ListItem("Icones 3");
    icons3->getClickEvent()->subscribe([](brls::View* view) {
		util::showDialogBoxInfo(
			"\uE100, \uE101, \uE102, \uE103, \uE104, \uE105, \uE106, \uE107, \uE108, \uE109, \uE10A, \uE10B, \uE10C, \uE10D, \uE10E, \uE10F"
			"\uE110, \uE111, \uE112, \uE113, \uE114, \uE115, \uE116, \uE117, \uE118, \uE119, \uE11A, \uE11B, \uE11C, \uE11D, \uE11E, \uE11F"
			"\uE120, \uE121, \uE122, \uE123, \uE124, \uE125, \uE126, \uE127, \uE128, \uE129, \uE12A, \uE12B, \uE12C, \uE12D, \uE12E, \uE12F"
			"\uE130, \uE131, \uE132, \uE133, \uE134, \uE135, \uE136, \uE137, \uE138, \uE139, \uE13A, \uE13B, \uE13C, \uE13D, \uE13E, \uE13F"
			"\uE140, \uE141, \uE142, \uE143, \uE144, \uE145, \uE146, \uE147, \uE148, \uE149, \uE14A, \uE14B, \uE14C, \uE14D, \uE14E, \uE14F"
			"\uE150, \uE151, \uE152, \uE153, \uE154, \uE155, \uE156, \uE157, \uE158, \uE159, \uE15A, \uE15B, \uE15C, \uE15D, \uE15E, \uE15F"
			"\uE160, \uE161, \uE162, \uE163, \uE164, \uE165, \uE166, \uE167, \uE168, \uE169, \uE16A, \uE16B, \uE16C, \uE16D, \uE16E, \uE16F"
			"\uE170, \uE171, \uE172, \uE173, \uE174, \uE175, \uE176, \uE177, \uE178, \uE179, \uE17A, \uE17B, \uE17C, \uE17D, \uE17E, \uE17F"
		);
    });
    icons3->setHeight(LISTITEM_HEIGHT);
    this->addView(icons3);

    brls::ListItem* icons4 = new brls::ListItem("Icones 4");
    icons4->getClickEvent()->subscribe([](brls::View* view) {
		util::showDialogBoxInfo(
			"\uE180, \uE181, \uE182, \uE183, \uE184, \uE185, \uE186, \uE187, \uE188, \uE189, \uE18A, \uE18B, \uE18C, \uE18D, \uE18E, \uE18F"
			"\uE190, \uE191, \uE192, \uE193, \uE194, \uE195, \uE196, \uE197, \uE198, \uE199, \uE19A, \uE19B, \uE19C, \uE19D, \uE19E, \uE19F"
			"\uE1A0, \uE1A1, \uE1A2, \uE1A3, \uE1A4, \uE1A5, \uE1A6, \uE1A7, \uE1A8, \uE1A9, \uE1AA, \uE1AB, \uE1AC, \uE1AD, \uE1AE, \uE1AF"
			"\uE1B0, \uE1B1, \uE1B2, \uE1B3, \uE1B4, \uE1B5, \uE1B6, \uE1B7, \uE1B8, \uE1B9, \uE1BA, \uE1BB, \uE1BC, \uE1BD, \uE1BE, \uE1BF"
			"\uE1C0, \uE1C1, \uE1C2, \uE1C3, \uE1C4, \uE1C5, \uE1C6, \uE1C7, \uE1C8, \uE1C9, \uE1CA, \uE1CB, \uE1CC, \uE1CD, \uE1CE, \uE1CF"
			"\uE1D0, \uE1D1, \uE1D2, \uE1D3, \uE1D4, \uE1D5, \uE1D6, \uE1D7, \uE1D8, \uE1D9, \uE1DA, \uE1DB, \uE1DC, \uE1DD, \uE1DE, \uE1DF"
			"\uE1E0, \uE1E1, \uE1E2, \uE1E3, \uE1E4, \uE1E5, \uE1E6, \uE1E7, \uE1E8, \uE1E9, \uE1EA, \uE1EB, \uE1EC, \uE1ED, \uE1EE, \uE1EF"
		);
    });
    icons4->setHeight(LISTITEM_HEIGHT);
    this->addView(icons4);
*/
}
