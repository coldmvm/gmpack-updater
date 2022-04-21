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
    const nlohmann::ordered_json jsonTranslations = util::getValueFromKey(this->nxlinks, contentTypeNames[(int)type].data());
    if (jsonTranslations.size()) {
        for (auto it = jsonTranslations.begin(); it != jsonTranslations.end(); ++it)
        {
            const std::string title = it.key();
            const std::string folders = (*it)["folders"].get<std::string>();
            const std::string url = (*it)["link"].get<std::string>();
			const std::string size = (*it)["size"].get<std::string>();
            bool enabled = (*it)["enabled"].get<bool>();

            if (enabled) {

                std::string foundTitle;

                std::vector<std::string> itemFolders;
                itemFolders.clear();

                //splitting folders and inserting into the vector
                size_t pos_start = 0, pos_end, delim_len = 1;
                std::string token;
                while((pos_end = folders.find(",", pos_start)) != std::string::npos) {
                    token = folders.substr(pos_start, pos_end - pos_start);
                    pos_start = pos_end + delim_len;
                    itemFolders.push_back(token);

                    std::string path = util::getContentsPath();
                    if (std::filesystem::exists(path + token))
                        foundTitle = "\u2605";
                    else
                        foundTitle = "";
                }
                itemFolders.push_back(folders.substr(pos_start));

                const std::string finalTitle = fmt::format("{} {} ({})", foundTitle, title, size);

                const std::string text("menus/common/download"_i18n + title);
                listItem = new brls::ListItem(finalTitle);
                listItem->setHeight(LISTITEM_HEIGHT);
                listItem->getClickEvent()->subscribe([this, type, text, url, itemFolders](brls::View* view) {
    				brls::StagedAppletFrame* stagedFrame = new brls::StagedAppletFrame();
                    stagedFrame->setTitle(fmt::format("menus/main/getting"_i18n, contentTypeFullNames[(int)type].data()));

                    if (util::isTranslationPresent(itemFolders))
				    {
                        stagedFrame->addStage(new ListDownloadConfirmationPage(stagedFrame, "menus/main/translation_exists_warning"_i18n));
                        stagedFrame->addStage(new WorkerPage(stagedFrame, "menus/common/deleting"_i18n, [this, type, itemFolders]() { util::doDelete(itemFolders, type); }));
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
