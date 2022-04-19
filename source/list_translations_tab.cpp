#include "list_translations_tab.hpp"

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

struct Translation
{
    std::string name;
    std::string folders;
    std::string url;
    std::string size;
};

struct obj_finder
{
    obj_finder(std::string key) : key_(key) {}

    bool operator()(const Translation& o) const
    {
        return key_ == o.name;
    }

    const std::string key_;
};

bool findTranslation(std::vector<Translation>& vec, std::string findwhat)
{
    std::vector<Translation>::iterator it;
    it = find_if (vec.begin(), vec.end(), obj_finder(findwhat));
    return (!it->name.empty());
}

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
    std::vector<Translation> vecTranslations;
    const nlohmann::ordered_json jsonTranslations = util::getValueFromKey(this->nxlinks, contentTypeNames[(int)type].data());
    if (jsonTranslations.size()) {
        for (auto it = jsonTranslations.begin(); it != jsonTranslations.end(); ++it)
        {
            const std::string title = it.key();
            const std::string folders = (*it)["folders"].get<std::string>();
            const std::string url = (*it)["link"].get<std::string>();
			const std::string size = (*it)["size"].get<std::string>();
            struct Translation t = {title, folders, url, size};
            vecTranslations.push_back(t);
        }

        for (const auto& entry : vecTranslations) {

            std::string foundTitle;

            const std::string folders = entry.folders;
            const std::string url = entry.url;

            std::vector<std::string> itemFoldersTemp;
            itemFoldersTemp.clear();

            //splitting folders and inserting into the vector
            size_t pos_start = 0, pos_end, delim_len = 1;
            std::string token;
            while((pos_end = folders.find(",", pos_start)) != std::string::npos) {
                token = folders.substr(pos_start, pos_end - pos_start);
                pos_start = pos_end + delim_len;
                itemFoldersTemp.push_back(token);

                std::string path = util::getContentsPath();
                if (std::filesystem::exists(path + token))
                    foundTitle = "\u2605";
                else
                    foundTitle = "";
            }
            itemFoldersTemp.push_back(folders.substr(pos_start));

            const std::string title = fmt::format("{} {} ({})", foundTitle, entry.name, entry.size);
			util::writeLog("title: " + title);
			util::writeLog("entry.name: " + entry.name);
            util::writeLog("entry.size: " + entry.size);
            util::writeLog("\n");
            //const std::string title = foundTitle + entry.name;

			const std::vector<std::string> itemFolders(itemFoldersTemp);

            const std::string text("menus/common/download"_i18n + title);
            listItem = new brls::ListItem(title);
            listItem->setHeight(LISTITEM_HEIGHT);
            listItem->getClickEvent()->subscribe([this, type, text, title, folders, url, itemFolders](brls::View* view) {
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
            description->setText("menus/main/translations_text"_i18n);
            break;
        }
        default:
            break;
    }

    this->addView(description);
}
