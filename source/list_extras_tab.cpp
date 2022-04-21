#include "list_extra_tab.hpp"

#include <filesystem>
#include <fstream>
#include <string>

#include "confirm_page.hpp"
#include "list_download_tab_confirmation.hpp"
#include "download.hpp"
#include "utils.hpp"
#include "worker_page.hpp"

namespace i18n = brls::i18n;
using namespace i18n::literals;

std::string listItemName(std::string folder)
{
    std::string path = util::getContentsPath();
    if (std::filesystem::exists(path + folder))
        return "\u2605";
    else
        return "";
}

ListExtraTab::ListExtraTab(const contentType type, const nlohmann::ordered_json& nxlinks) : brls::List(), type(type), nxlinks(nxlinks)
{
    this->setDescription();

    this->createList();
}

void ListExtraTab::createList()
{
    ListExtraTab::createList(this->type);
}

void ListExtraTab::createList(contentType type)
{
    int counter = 0;
    const nlohmann::ordered_json jsonExtras = util::getValueFromKey(this->nxlinks, contentTypeNames[(int)type].data());
    if (jsonExtras.size()) {
        for (auto it = jsonExtras.begin(); it != jsonExtras.end(); ++it)
        {
            const std::string title = it.key();
            const std::string folders = (*it)["folders"].get<std::string>();
            const std::string url = (*it)["link"].get<std::string>();
            const std::string size = (*it)["size"].get<std::string>();
            bool enabled = (*it)["enabled"].get<bool>();

            if (enabled) {
                counter++;

                std::string foundTitle;

                std::vector<std::string> itemFolders;
                itemFolders.clear();

                //splitting folders and inserting into the vector
                size_t pos_start = 0, pos_end, delim_len = 1;
                std::string token;

                if (folders.find(",", pos_start) != std::string::npos)
                {
                    while((pos_end = folders.find(",", pos_start)) != std::string::npos) {
                        token = folders.substr(pos_start, pos_end - pos_start);
                        pos_start = pos_end + delim_len;
                        itemFolders.push_back(token);
                        foundTitle = listItemName(token);
                    }
                }
                itemFolders.push_back(folders.substr(pos_start));
                foundTitle = listItemName(folders.substr(pos_start));

                const std::string finalTitle = fmt::format("{} {} ({})", foundTitle, title, size);

                const std::string text("menus/common/download"_i18n + title);
                listItem = new brls::ListItem(finalTitle);
                listItem->setHeight(LISTITEM_HEIGHT);
                listItem->getClickEvent()->subscribe([this, type, text, url, itemFolders](brls::View* view) {
                    brls::StagedAppletFrame* stagedFrame = new brls::StagedAppletFrame();
                    stagedFrame->setTitle(fmt::format("menus/main/getting"_i18n, contentTypeFullNames[(int)type].data()));

                    if (util::isExtraPresent(itemFolders))
                    {

                        switch (type) {
                           case contentType::translations: {
                               stagedFrame->addStage(new ListDownloadConfirmationPage(stagedFrame, "menus/main/translation_exists_warning"_i18n));
                               break;
                            }
                            case contentType::modifications: {
                                stagedFrame->addStage(new ListDownloadConfirmationPage(stagedFrame, "menus/main/modification_exists_warning"_i18n));
                                break;
                            }
                                default:
                                break;
                            }

                        stagedFrame->addStage(new WorkerPage(stagedFrame, "menus/common/deleting"_i18n, [this, type, itemFolders]() { util::doDelete(itemFolders); }));
                    }
                    else
                    {
                        stagedFrame->addStage(new ConfirmPage(stagedFrame, text));
                        stagedFrame->addStage(new WorkerPage(stagedFrame, "menus/common/downloading"_i18n, [this, type, url]() { util::downloadArchive(url, type); }));
                        stagedFrame->addStage(new WorkerPage(stagedFrame, "menus/common/extracting"_i18n, [this, type]() { util::extractArchive(type); }));
                    }

                    stagedFrame->addStage(new ConfirmPage(stagedFrame, "menus/common/all_done"_i18n, true));
                    brls::Application::pushView(stagedFrame);
                });
                this->addView(listItem);
            }
        }
    }
    else {
        this->displayNotFound();
    }

    if (counter <= 0)
        this->noItemsToDisplay();
}

void ListExtraTab::displayNotFound()
{
    brls::Label* notFound = new brls::Label(
        brls::LabelStyle::SMALL,
        "menus/main/links_not_found"_i18n,
        true);
    notFound->setHorizontalAlign(NVG_ALIGN_CENTER);
    this->addView(notFound);
}

void ListExtraTab::noItemsToDisplay()
{
    brls::Label* notFound = new brls::Label(
        brls::LabelStyle::SMALL,
        "menus/main/no_items_to_display"_i18n,
        true);
    notFound->setHorizontalAlign(NVG_ALIGN_CENTER);
    this->addView(notFound);
}

void ListExtraTab::setDescription()
{
    this->setDescription(this->type);
}

void ListExtraTab::setDescription(contentType type)
{
    brls::Label* description = new brls::Label(brls::LabelStyle::DESCRIPTION, "", true);

    switch (type) {
        case contentType::translations: {
            description->setText("menus/main/translations_text"_i18n);
            break;
        }
        case contentType::modifications: {
            description->setText("menus/main/modifications_text"_i18n);
            break;
        }
        default:
            break;
    }

    this->addView(description);
}
