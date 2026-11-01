//
// music: keyword search page.
//

#pragma once

#include <borealis.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <string>

#include "nxkit/ui/cover_grid_page.hpp"

namespace music {

class SearchPage : public brls::Box {
public:
    SearchPage();

    ~SearchPage() override;

private:
    void openKeyboard();
    void search(const std::string& keywords);
    void loadNextPage();
    void showSongs(const nlohmann::json& json, bool firstPage);
    void showError(const std::string& error);

    std::string keywords;
    int offset = 0;
    int songCount = 0;
    bool loading = false;
    bool noMore = true;

    std::shared_ptr<bool> alive = std::make_shared<bool>(true);

    brls::Label* hintLabel = nullptr;
    nxkit::CoverGridPage* grid = nullptr;
};

}  // namespace music
