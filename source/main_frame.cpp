#include "main_frame.hpp"

#include <fstream>
#include <json.hpp>

#include "about_tab.hpp"
#include "ams_tab.hpp"
#include "download.hpp"
#include "fs.hpp"
#include "list_download_tab.hpp"
#include "list_translations_tab.hpp"
#include "tools_tab.hpp"
#include "utils.hpp"

namespace i18n = brls::i18n;
using namespace i18n::literals;
using json = nlohmann::json;

namespace {
    constexpr const char AppTitle[] = APP_TITLE;
    constexpr const char AppVersion[] = APP_VERSION;
}  // namespace

MainFrame::MainFrame() : TabFrame()
{
    //util::writeLog("APG started");
	this->setIcon("romfs:/gui_icon.png");
    this->setTitle(AppTitle);

    s64 freeStorage;
    std::string tag = util::getLatestTag(TAGS_INFO);

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

        this->setFooterText(fmt::format("menus/main/footer_text"_i18n,
            (!tag.empty() && iTag > iAppVersion) ? "v" + std::string(AppVersion) + "menus/main/new_update"_i18n : AppVersion,
            R_SUCCEEDED(fs::getFreeStorageSD(freeStorage)) ? (float)freeStorage / 0x40000000 : -1));
    }
    else {
        this->setFooterText(fmt::format("menus/main/footer_text"_i18n, std::string(AppVersion) + "menus/main/footer_text_not_connected"_i18n, R_SUCCEEDED(fs::getFreeStorageSD(freeStorage)) ? (float)freeStorage / 0x40000000 : -1));
    }

    json hideStatus = fs::parseJsonFile(HIDE_TABS_JSON);
    nlohmann::ordered_json nxlinks;
    download::getRequest(NXLINKS_URL, nxlinks);

    bool erista = util::isErista();

    if (!util::getBoolValue(hideStatus, "about"))
        this->addTab("menus/main/about"_i18n, new AboutTab());

    if (!util::getBoolValue(hideStatus, "atmosphere"))
        this->addTab("menus/main/update_ams"_i18n, new AmsTab(nxlinks, erista, util::getBoolValue(hideStatus, "atmosphereentries")));

    if (!util::getBoolValue(hideStatus, "translations"))
        this->addTab("menus/main/download_translations"_i18n, new ListTranslationsTab(contentType::translations, nxlinks));

    if (!util::getBoolValue(hideStatus, "firmwares"))
        this->addTab("menus/main/download_firmware"_i18n, new ListDownloadTab(contentType::fw, nxlinks));

    if (!util::getBoolValue(hideStatus, "tools"))
        this->addTab("menus/main/tools"_i18n, new ToolsTab(tag, erista, hideStatus));

    this->registerAction("", brls::Key::B, [this] { return true; });
}
