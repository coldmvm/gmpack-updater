#include "list_download_page_changelog.hpp"

#include "utils.hpp"

namespace i18n = brls::i18n;
using namespace i18n::literals;
ListDownloadChangelogPage::ListDownloadChangelogPage() : AppletFrame(true, true)
{
    this->setTitle("menus/changelog/changelog"_i18n);

    list = new brls::List();

    listItem = new brls::ListItem("TESTEEEEEEEEEEEE");
    list->addView(listItem);

    label = new brls::Label(brls::LabelStyle::REGULAR, "Este texto não aparecerá novamente.", true);
    label->setHorizontalAlign(NVG_ALIGN_LEFT);
    list->addView(label);

    this->setContentView(list);
}
