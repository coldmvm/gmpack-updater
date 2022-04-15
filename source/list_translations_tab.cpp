#include "list_translations_tab.hpp"

#include <filesystem>
#include <fstream>
#include <string>

#include "confirm_page.hpp"
#include "download.hpp"
#include "utils.hpp"
#include "worker_page.hpp"

namespace i18n = brls::i18n;
using namespace i18n::literals;

ListTranslationsTab::ListTranslationsTab(const contentType type, const nlohmann::ordered_json& nxlinks) : brls::List(), type(type), nxlinks(nxlinks)
{
    this->setDescription();

    this->createList();
}

void ListTranslationsTab::createList()
{
    ListTranslationsTab::createList(this->type);
}

void ListTranslationsTab::createList(contentType type)
{
    std::vector<std::pair<std::string, std::string>> links;
    links = download::getLinksFromJson(util::getValueFromKey(this->nxlinks, contentTypeNames[(int)type].data()));

    if (links.size()) {
        for (const auto& link : links) {
            const std::string title = link.first;
            const std::string url = link.second;
            //const std::string text("menus/common/download"_i18n + link.first + "menus/common/from"_i18n + url);
            const std::string text("menus/common/download"_i18n + link.first);
            listItem = new brls::ListItem(link.first);
            listItem->setHeight(LISTITEM_HEIGHT);
            listItem->getClickEvent()->subscribe([this, type, text, url, title](brls::View* view) {
                brls::StagedAppletFrame* stagedFrame = new brls::StagedAppletFrame();
                stagedFrame->setTitle(fmt::format("menus/main/getting"_i18n, contentTypeNames[(int)type].data()));
                stagedFrame->addStage(new ConfirmPage(stagedFrame, text));

                //if (type != contentType::payloads && type != contentType::hekate_ipl) {
                    //if (type != contentType::cheats || (this->newCheatsVer != this->currentCheatsVer && this->newCheatsVer != "offline")) {
                        stagedFrame->addStage(new WorkerPage(stagedFrame, "menus/common/downloading"_i18n, [this, type, url]() { util::downloadArchive(url, type); }));
                    //}
                    stagedFrame->addStage(new WorkerPage(stagedFrame, "menus/common/extracting"_i18n, [this, type]() { util::extractArchive(type); }));
                //}
                /*else if (type == contentType::payloads) {
                    fs::createTree(BOOTLOADER_PL_PATH);
                    std::string path = std::string(BOOTLOADER_PL_PATH) + title;
                    stagedFrame->addStage(new WorkerPage(stagedFrame, "menus/common/downloading"_i18n, [url, path]() { download::downloadFile(url, path, OFF); }));
                }
                else if (type == contentType::hekate_ipl) {
                    fs::createTree(BOOTLOADER_PATH);
                    std::string path = std::string(BOOTLOADER_PATH) + title;
                    stagedFrame->addStage(new WorkerPage(stagedFrame, "menus/common/downloading"_i18n, [url, path]() { download::downloadFile(url, path, OFF); }));
                }*/

                std::string doneMsg = "menus/common/all_done"_i18n;
                switch (type) {
                    case contentType::translations: {
                        stagedFrame->addStage(new ConfirmPage(stagedFrame, doneMsg, true));
                        break;












                    }
                    default:
                        stagedFrame->addStage(new ConfirmPage(stagedFrame, doneMsg, true));
                        break;
                }
                brls::Application::pushView(stagedFrame);
            });
            this->addView(listItem);
        }
    }
    else {
        this->displayNotFound();
    }
}

void ListTranslationsTab::displayNotFound()
{
    brls::Label* notFound = new brls::Label(
        brls::LabelStyle::SMALL,
        "menus/main/links_not_found"_i18n,
        true);
    notFound->setHorizontalAlign(NVG_ALIGN_CENTER);
    this->addView(notFound);
}

void ListTranslationsTab::setDescription()
{
    this->setDescription(this->type);
}

void ListTranslationsTab::setDescription(contentType type)
{
    brls::Label* description = new brls::Label(brls::LabelStyle::DESCRIPTION, "", true);

    switch (type) {
        case contentType::translations: {
            SetSysFirmwareVersion ver;
            description->setText("menus/main/translations_text"_i18n);
            break;
        }
        default:
            break;
    }

    this->addView(description);
}
