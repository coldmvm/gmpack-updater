#include "main_frame.hpp"

#include <fstream>
#include <json.hpp>

#include "about_tab.hpp"
#include "ams_tab.hpp"
#include "update_tab.hpp"
#include "download.hpp"
#include "fs.hpp"
#include "list_download_tab.hpp"
#include "list_extra_tab.hpp"
#include "credits_tab.hpp"
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
    util::cleanFiles();

    bool newversion = false;

	this->setIcon("romfs:/gui_icon.png");
    this->setTitle(AppTitle);

    s64 freeStorage;
    std::string tag = util::getLatestTag();

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

        newversion = (iTag > iAppVersion);

        this->setFooterText(fmt::format("menus/main/footer_text"_i18n, BRAND_FULL_NAME,
            (!tag.empty() && newversion) ? "v" + std::string(AppVersion) + "menus/main/new_update_footer"_i18n : AppVersion,
            R_SUCCEEDED(fs::getFreeStorageSD(freeStorage)) ? floor(((float)freeStorage / 0x40000000) * 100.0) / 100.0 : -1));
    }
    else {
        this->setFooterText(fmt::format("menus/main/footer_text"_i18n, BRAND_FULL_NAME, std::string(AppVersion) + "menus/main/footer_text_not_connected"_i18n, R_SUCCEEDED(fs::getFreeStorageSD(freeStorage)) ? (float)freeStorage / 0x40000000 : -1));
    }

    nlohmann::ordered_json nxlinks;
    download::getRequest(NXLINKS_URL, nxlinks);

    bool erista = util::isErista();

    if (!newversion) {
		this->addTab("menus/main/about"_i18n, new AboutTab());
		this->addTab("menus/main/update_ams"_i18n, new AmsTab(nxlinks, erista));
		this->addTab("menus/main/download_firmware"_i18n, new ListDownloadTab(contentType::fw, nxlinks));
		this->addTab("menus/main/download_translations"_i18n, new ListExtraTab(contentType::translations, nxlinks));
		this->addTab("menus/main/download_mods"_i18n, new ListExtraTab(contentType::modifications, nxlinks));
		this->addTab("menus/main/tools"_i18n, new ToolsTab(tag, erista));
		this->addSeparator();
		this->addTab("menus/main/credits"_i18n, new CreditsTab());

		this->registerAction("", brls::Key::B, [this] { return true; });
    }
    else
    {
        this->addTab("menus/main/new_update"_i18n, new UpdateTab(tag));
        this->registerAction("", brls::Key::B, [this] { return true; });
    }
}
