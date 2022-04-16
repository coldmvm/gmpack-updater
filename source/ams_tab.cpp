#include "ams_tab.hpp"

#include <filesystem>
#include <string>

#include "confirm_page.hpp"
#include "list_download_tab_confirmation.hpp"
#include "current_cfw.hpp"
#include "dialogue_page.hpp"
#include "download.hpp"
#include "extract.hpp"
#include "fs.hpp"
#include "utils.hpp"
#include "worker_page.hpp"

namespace i18n = brls::i18n;
using namespace i18n::literals;

AmsTab::AmsTab(const nlohmann::json& nxlinks, const bool erista, const bool hideStandardEntries) : brls::List()
{
    this->erista = erista;
    //this->hekate = util::getValueFromKey(nxlinks, "hekate");
    auto cfws = util::getValueFromKey(nxlinks, "cfws");

    if (!hideStandardEntries) {
        std::string packVersion = util::getGMPackVersion();

        this->description = new brls::Label(brls::LabelStyle::DESCRIPTION,
            "menus/ams_update/pack_label"_i18n + "\n" +
            (CurrentCfw::running_cfw == CFW::ams ? "menus/ams_update/current_ams"_i18n + CurrentCfw::getAmsInfo() : "") +
			(util::getGMPackVersion()) +
			(erista ? "\n" + "menus/ams_update/erista_rev"_i18n : "\n" + "menus/ams_update/mariko_rev"_i18n), true);
        this->addView(description);

        CreateDownloadItems(util::getValueFromKey(cfws, "GMPack"), false);

        description = new brls::Label(brls::LabelStyle::DESCRIPTION, "menus/ams_update/goma_label"_i18n, true);
        this->addView(description);

		CreateDownloadItems(util::getValueFromKey(cfws, "GNX"), false);
    }
}

void AmsTab::CreateDownloadItems(const nlohmann::ordered_json& cfw_links, bool hekate)
{
    std::string operation("menus/ams_update/getting_ams"_i18n);
    std::vector<std::pair<std::string, std::string>> links;
    links = download::getLinksFromJson(cfw_links);
    if (links.size()) {
        //auto hekate_link = download::getLinksFromJson(this->hekate);
        //std::string hekate_url = hekate_link[0].second;
        //std::string text_hekate = "menus/common/download"_i18n + hekate_link[0].first;
		
		std::string hekate_url = "";
        std::string text_hekate = "";

        for (const auto& link : links) {
            std::string url = link.second;
            //std::string text("menus/common/download"_i18n + link.first + "menus/common/from"_i18n + url);
            std::string text("menus/common/download"_i18n + link.first);
            listItem = new brls::ListItem(link.first);
            listItem->setHeight(LISTITEM_HEIGHT);
            listItem->getClickEvent()->subscribe([this, text, text_hekate, url, hekate_url, operation, hekate](brls::View* view) {
                if (!erista && !std::filesystem::exists(MARIKO_PAYLOAD_PATH)) {
                    brls::Application::crash("menus/errors/mariko_payload_missing"_i18n);
                }
                else {
                    CreateStagedFrames(text, url, operation, erista, hekate, text_hekate, hekate_url);
                }
            });
            this->addView(listItem);
        }
    }
    else {
        description = new brls::Label(
            brls::LabelStyle::SMALL,
            "menus/main/links_not_found"_i18n,
            true);
        description->setHorizontalAlign(NVG_ALIGN_CENTER);
        this->addView(description);
    }

}

void AmsTab::CreateStagedFrames(const std::string& text, const std::string& url, const std::string& operation, bool erista, bool hekate, const std::string& text_hekate, const std::string& hekate_url)
{
    brls::StagedAppletFrame* stagedFrame = new brls::StagedAppletFrame();
    stagedFrame->setTitle(operation);

    stagedFrame->addStage(
        new ListDownloadConfirmationPage(stagedFrame, "menus/main/download_time_warning"_i18n));

    stagedFrame->addStage(
        new ConfirmPage(stagedFrame, text));
    stagedFrame->addStage(
        new WorkerPage(stagedFrame, "menus/common/downloading"_i18n, [url]() { util::downloadArchive(url, contentType::ams_cfw); }));
    stagedFrame->addStage(
        new WorkerPage(stagedFrame, "menus/common/extracting"_i18n, []() { util::extractArchive(contentType::ams_cfw); }));
    if (hekate) {
        stagedFrame->addStage(
            new DialoguePage_ams(stagedFrame, text_hekate, erista));
    }
    stagedFrame->addStage(
        new ConfirmPage(stagedFrame, "menus/ams_update/reboot_rcm"_i18n, false, true, erista));
    brls::Application::pushView(stagedFrame);
}

std::string AmsTab::GetRepoName(const std::string& repo)
{
    return repo.substr(repo.find("/") + 1, repo.length());
}

std::set<std::string> AmsTab::GetLastDownloadedModules(const std::string& json_path)
{
    nlohmann::json package = fs::parseJsonFile(json_path);
    std::set<std::string> res;
    if (package.find("modules") != package.end()) {
        for (const auto& module : package.at("modules")) {
            res.insert(module.get<std::string>());
        }
    }
    return res;
}

nlohmann::ordered_json AmsTab::SortDeepseaModules(const nlohmann::ordered_json& modules)
{
    nlohmann::ordered_json sorted_modules = nlohmann::ordered_json::object();
    if (modules.find("modules") != modules.end()) {
        for (const auto& module : modules.at("modules").items()) {
            sorted_modules[std::string(module.value().at("category"))][module.key()] = module.value();
        }
    }
    return sorted_modules;
}

bool UnTogglableListItem::onClick()
{
    return true;
}
