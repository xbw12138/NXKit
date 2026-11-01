//
// music: browse page (recommendations, toplists or the tracks of a playlist).
//

#pragma once

#include <borealis.hpp>
#include <nlohmann/json.hpp>

#include <memory>
#include <string>

#include "nxkit/ui/cover_grid_page.hpp"

namespace music {

class BrowsePage : public brls::Box {
public:
    enum class Mode { Personalized, Toplist, Playlist };

    explicit BrowsePage(Mode mode, long playlistId = 0);

    ~BrowsePage() override;

    void reload();

private:
    void loadFirstPage();
    void loadNextPage();
    void showPlaylists(const nlohmann::json& json);
    void showSongs(const nlohmann::json& json, bool firstPage);
    void showError(const std::string& error);

    Mode mode;
    long playlistId;
    int offset = 0;
    bool loading = false;
    bool noMore = false;

    /// Flipped to false when the page is destroyed, so late http callbacks
    /// can detect the page is gone.
    std::shared_ptr<bool> alive = std::make_shared<bool>(true);

    nxkit::CoverGridPage* grid = nullptr;
};

}  // namespace music
