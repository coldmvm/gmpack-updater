#pragma once

#include <borealis.hpp>
#include <json.hpp>
#include <set>

class AmsTab : public brls::List
{
private:
    brls::ListItem* listItem;
    brls::Label* description;
    int size = 0;
    bool erista;
    void CreateStagedFrames(const std::string& text, const std::string& url, const std::string& operation, bool erista);
    void CreateDownloadItems(const nlohmann::ordered_json& cfw_links, const std::string& pack = "");

public:
    AmsTab(const nlohmann::json& nxlinks, const bool erista = true, const bool hideStandardEntries = false);
};
