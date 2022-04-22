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

AmsTab::AmsTab(const nlohmann::json& nxlinks, const bool erista) : brls::List()
{
    this->erista = erista;
    auto cfws = util::getValueFromKey(nxlinks, "cfws");

    std::string packVersion = util::getGMPackVersion();

    this->description = new brls::Label(brls::LabelStyle::DESCRIPTION,
        "menus/ams_update/pack_label"_i18n + "\n" + "menus/ams_update/current_ams"_i18n +
        (util::getGMPackVersion()) +
        (CurrentCfw::running_cfw == CFW::ams ? CurrentCfw::getAmsInfo() : "") +
        (erista ? "\n" + "menus/ams_update/erista_rev"_i18n : "\n" + "menus/ams_update/mariko_rev"_i18n), true);
    this->addView(description);

    CreateDownloadItems(util::getValueFromKey(cfws, "GMPack"), "GMPACK");

    description = new brls::Label(brls::LabelStyle::DESCRIPTION, "menus/ams_update/goma_label"_i18n, true);
    this->addView(description);

    CreateDownloadItems(util::getValueFromKey(cfws, "GNX"), "GNX");
}

void AmsTab::CreateDownloadItems(const nlohmann::ordered_json& cfw_links, const std::string& pack)
{
    std::string operation(fmt::format("menus/main/getting"_i18n, pack));
    std::vector<std::pair<std::string, std::string>> links;
    links = download::getLinksFromJson(cfw_links);
    if (links.size()) {
        for (const auto& link : links) {
            std::string url = link.second;
            //std::string text("menus/common/download"_i18n + link.first + "menus/common/from"_i18n + url);
            std::string text("menus/common/download"_i18n + link.first);
            listItem = new brls::ListItem(link.first);
            listItem->setHeight(LISTITEM_HEIGHT);
            listItem->getClickEvent()->subscribe([this, text, url, operation](brls::View* view) {
                if (!erista && !std::filesystem::exists(MARIKO_PAYLOAD_PATH)) {
                    brls::Application::crash("menus/errors/mariko_payload_missing"_i18n);
                }
                else {
                    CreateStagedFrames(text, url, operation, erista);
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

void AmsTab::CreateStagedFrames(const std::string& text, const std::string& url, const std::string& operation, bool erista)
{
    brls::StagedAppletFrame* stagedFrame = new brls::StagedAppletFrame();
    stagedFrame->setTitle(operation);

    stagedFrame->addStage(
        new ListDownloadConfirmationPage(stagedFrame, "menus/main/download_time_warning"_i18n));

    stagedFrame->addStage(new ConfirmPage(stagedFrame, text));
    stagedFrame->addStage(new WorkerPage(stagedFrame, "menus/common/downloading"_i18n, [url]() { util::downloadArchive(url, contentType::ams_cfw); }));
    stagedFrame->addStage(new WorkerPage(stagedFrame, "menus/common/extracting"_i18n, []() { util::extractArchive(contentType::ams_cfw); }));
    stagedFrame->addStage(new ConfirmPage(stagedFrame, "menus/ams_update/reboot_rcm"_i18n, false, true, erista));
    brls::Application::pushView(stagedFrame);
}
