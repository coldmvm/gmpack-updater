#pragma once

#include <borealis.hpp>
#include <json.hpp>

class ToolsTab : public brls::List
{
private:
    brls::ListItem* updateApp;
    brls::ListItem* changelog;
    brls::ListItem* cleanUp;
    brls::ListItem* netSettings;
    brls::ListItem* browser;

    brls::StagedAppletFrame* stagedFrame;

public:
    ToolsTab(const std::string& tag, bool erista = true, const nlohmann::json& hideStatus = {});
};