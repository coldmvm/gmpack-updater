#pragma once

#include <borealis.hpp>
#include <chrono>

class ListDownloadConfirmationPage : public brls::View
{
private:
    brls::Button* button = nullptr;
    brls::Label* label = nullptr;
	brls::Image* icon = nullptr;
	brls::Image* image = nullptr;
    std::chrono::system_clock::time_point start = std::chrono::high_resolution_clock::now();
    bool done = false;

public:
    ListDownloadConfirmationPage(brls::StagedAppletFrame* frame, const std::string& text, bool done = false);
    ~ListDownloadConfirmationPage();

    void draw(NVGcontext* vg, int x, int y, unsigned width, unsigned height, brls::Style* style, brls::FrameContext* ctx) override;
    void layout(NVGcontext* vg, brls::Style* style, brls::FontStash* stash) override;
    brls::View* getDefaultFocus() override;
};