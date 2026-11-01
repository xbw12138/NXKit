//
// music: browse page (recommendations, toplists or the tracks of a playlist).
//

#include "music/pages/browse_page.hpp"

#include "music/netease_client.hpp"
#include "nxkit/utils/number_helper.hpp"

using namespace brls::literals;

namespace music {

static const int kPageSize = 50;

static std::string coverOf(const nlohmann::json& song) {
    if (song.contains("al") && song["al"].is_object() &&
        song["al"].contains("picUrl") && song["al"]["picUrl"].is_string())
        return song["al"]["picUrl"].get<std::string>();
    return "";
}

static std::string artistsOf(const nlohmann::json& song) {
    std::string artists;
    if (!song.contains("ar") || !song["ar"].is_array()) return artists;
    for (const auto& artist : song["ar"]) {
        if (!artist.contains("name")) continue;
        if (!artists.empty()) artists += " / ";
        artists += artist["name"].get<std::string>();
    }
    return artists;
}

static std::string durationOf(const nlohmann::json& song) {
    if (!song.contains("dt") || !song["dt"].is_number()) return "";
    return wiliwili::sec2Time(song["dt"].get<long>() / 1000);
}

BrowsePage::BrowsePage(Mode mode, long playlistId)
    : mode(mode), playlistId(playlistId) {
    this->setAxis(brls::Axis::COLUMN);

    this->grid = new nxkit::CoverGridPage();
    this->grid->setOnSelect([this](const nxkit::CoverItem& item) {
        if (this->mode == Mode::Playlist) {
            brls::Application::notify("music/toast/playback_todo"_i18n);
            return;
        }

        long id = 0;
        try {
            id = std::stol(item.id);
        } catch (...) {
            return;
        }
        if (id <= 0) return;
        brls::Application::pushActivity(
            new brls::Activity(new BrowsePage(Mode::Playlist, id)));
    });

    if (this->mode == Mode::Playlist) {
        this->grid->setOnNextPage([this] { this->loadNextPage(); });
    }

    this->addView(this->grid);
    this->reload();
}

BrowsePage::~BrowsePage() { *this->alive = false; }

void BrowsePage::reload() {
    this->loading = false;
    this->noMore = false;
    this->offset = 0;
    this->grid->showSkeleton(12);
    this->loadFirstPage();
}

void BrowsePage::loadFirstPage() {
    switch (this->mode) {
        case Mode::Personalized:
            NeteaseClient::instance().personalized(
                30, [this, alive = this->alive](const nlohmann::json& json,
                                                const std::string& error) {
                    brls::sync([this, alive, json, error] {
                        if (!*alive) return;
                        if (!error.empty()) {
                            this->showError(error);
                            return;
                        }
                        this->showPlaylists(json);
                    });
                });
            break;
        case Mode::Toplist:
            NeteaseClient::instance().toplist(
                [this, alive = this->alive](const nlohmann::json& json,
                                            const std::string& error) {
                    brls::sync([this, alive, json, error] {
                        if (!*alive) return;
                        if (!error.empty()) {
                            this->showError(error);
                            return;
                        }
                        this->showPlaylists(json);
                    });
                });
            break;
        case Mode::Playlist:
            this->loadNextPage();
            break;
    }
}

void BrowsePage::loadNextPage() {
    if (this->loading || this->noMore) return;
    this->loading = true;

    int offset = this->offset;
    NeteaseClient::instance().playlistTracks(
        this->playlistId, offset, kPageSize,
        [this, alive = this->alive, offset](const nlohmann::json& json,
                                            const std::string& error) {
            brls::sync([this, alive, json, error, offset] {
                if (!*alive) return;
                this->loading = false;
                if (!error.empty()) {
                    this->showError(error);
                    return;
                }
                this->showSongs(json, offset == 0);
            });
        });
}

void BrowsePage::showPlaylists(const nlohmann::json& json) {
    std::vector<nxkit::CoverItem> items;

    const nlohmann::json* list = nullptr;
    if (json.contains("result") && json["result"].is_array())
        list = &json["result"];
    else if (json.contains("list") && json["list"].is_array())
        list = &json["list"];

    if (!list) {
        this->showError("music/error/empty"_i18n);
        return;
    }

    for (const auto& entry : *list) {
        nxkit::CoverItem item;
        if (entry.contains("id")) item.id = std::to_string(entry["id"].get<long>());
        item.title = entry.value("name", std::string());

        if (this->mode == Mode::Toplist) {
            item.cover = entry.value("coverImgUrl", std::string());
            item.subtitle = entry.value("updateFrequency", std::string());
        } else {
            item.cover = entry.value("picUrl", std::string());
            if (entry.contains("playCount") && entry["playCount"].is_number())
                item.subtitle =
                    wiliwili::num2w(entry["playCount"].get<size_t>()) + " " +
                    "music/label/play_count"_i18n;
        }

        if (entry.contains("trackCount") && entry["trackCount"].is_number())
            item.badge = std::to_string(entry["trackCount"].get<int>()) + " " +
                         "music/label/track_count"_i18n;

        items.push_back(std::move(item));
    }

    if (items.empty())
        this->grid->setEmpty("music/error/empty"_i18n);
    else
        this->grid->setItems(items);
}

void BrowsePage::showSongs(const nlohmann::json& json, bool firstPage) {
    const nlohmann::json* songs = nullptr;
    if (json.contains("songs") && json["songs"].is_array())
        songs = &json["songs"];
    else if (json.contains("result") && json["result"].is_object() &&
             json["result"].contains("songs") &&
             json["result"]["songs"].is_array())
        songs = &json["result"]["songs"];

    if (!songs) {
        this->showError("music/error/empty"_i18n);
        return;
    }

    std::vector<nxkit::CoverItem> items;
    for (const auto& song : *songs) {
        nxkit::CoverItem item;
        if (song.contains("id")) item.id = std::to_string(song["id"].get<long>());
        item.title = song.value("name", std::string());
        item.cover = coverOf(song);
        item.subtitle = artistsOf(song);
        item.badge = durationOf(song);
        items.push_back(std::move(item));
    }

    this->offset += (int)items.size();
    if ((int)items.size() < kPageSize) this->noMore = true;

    if (firstPage) {
        if (items.empty())
            this->grid->setEmpty("music/error/empty"_i18n);
        else
            this->grid->setItems(items);
    } else if (!items.empty()) {
        this->grid->appendItems(items);
    }
}

void BrowsePage::showError(const std::string& error) {
    this->grid->setError(error);
}

}  // namespace music
