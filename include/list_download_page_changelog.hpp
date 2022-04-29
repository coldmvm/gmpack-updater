#pragma once

#include <borealis.hpp>

class ListDownloadChangelogPage : public brls::AppletFrame
{
private:
    brls::Label* label;
    brls::List* list;
    brls::ListItem* listItem;

public:
    ListDownloadChangelogPage();
};