#include "changelog_page.hpp"

#include "utils.hpp"

namespace i18n = brls::i18n;
using namespace i18n::literals;
ChangelogPage::ChangelogPage() : AppletFrame(true, true)
{
    this->setTitle("menus/changelog/changelog"_i18n);
    list = new brls::List();
    std::vector<std::string> verTitles;
    std::string change;
    std::vector<std::string> changes;

    verTitles.push_back("v1.0.1");
    changes.push_back("\uE016 Versão inicial.");

    verTitles.push_back("v1.0.2");
    changes.push_back("\uE016 Página 'Ferramentas' incluída.\n\uE016 Várias opções incluídas na página 'Ferramentas'.\n\uE016 Opção de atualizar o homebrew online incluída.\n\uE016 Código limpo.");
	
	verTitles.push_back("v1.0.3");
	changes.push_back("\uE016 Funções legadas do AIO que não serão usados pelo APG foram removidas.\n\uE016 Mais limpeza no código doi feita.\n\n\uE016 Erros de tradução foram corrigidos.");
	
	verTitles.push_back("v1.1.0");
	changes.push_back("\uE016 Foi incluída a funções de baixar traduções.\n\uE016 Limpeza de código.\n\n\uE016 Erros de tradução foram corrigidos.");

    for (int i = verTitles.size() - 1; i >= 0; i--) {
        listItem = new brls::ListItem(verTitles[i]);
        change = changes[i];
        listItem->getClickEvent()->subscribe([change](brls::View* view) {
            util::showDialogBoxInfo(change);
        });
        list->addView(listItem);
    }
    this->setContentView(list);
}

void ChangelogPage::ShowChangelogContent(const std::string version, const std::string content)
{
    listItem = new brls::ListItem(version);
    listItem->getClickEvent()->subscribe([version, content](brls::View* view) {
        brls::AppletFrame* appView = new brls::AppletFrame(true, true);

        brls::PopupFrame::open(version, appView, "", "");
    });
    list->addView(listItem);
}