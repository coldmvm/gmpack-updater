#include "credits_tab.hpp"

namespace i18n = brls::i18n;
using namespace i18n::literals;

CreditsTab::CreditsTab()
{
/*    // Subtitle
    brls::Label* subTitle = new brls::Label(
        brls::LabelStyle::REGULAR,
        "menus/credits/title"_i18n,
        true);
    subTitle->setHorizontalAlign(NVG_ALIGN_CENTER);
    this->addView(subTitle);

    // Copyright
    brls::Label* copyright = new brls::Label(
        brls::LabelStyle::DESCRIPTION,
        "menus/credits/copyright"_i18n + "\n© 2022 GameMod design",
        true);
    copyright->setHorizontalAlign(NVG_ALIGN_CENTER);
    this->addView(copyright);
*/

    brls::Label* text = new brls::Label(brls::LabelStyle::DESCRIPTION, "menus/credits/title1"_i18n, true);
    this->addView(text);

    // Credits
    this->addView(new brls::Header("menus/credits/title2"_i18n));
    brls::Label* links = new brls::Label(brls::LabelStyle::SMALL, "menus/credits/credits"_i18n, true);
    this->addView(links);
}