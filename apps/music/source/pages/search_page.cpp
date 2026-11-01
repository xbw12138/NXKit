//
// music: keyword search page.
//

#include "music/pages/search_page.hpp"

#include <borealis/views/button.hpp>

#include "music/netease_client.hpp"
#include "nxkit/utils/number_helper.hpp"

using namespace brls::literals;

namespace music {

static const int kSearchPageSize = 30;

static std::string searchCover(const nlohmann::json& song) {
    if (song.contains("al") && song["al"].is_object() &&
        song["al"].contains("picUrl") && song["al"]["picUrl"].is_string())
        return song["al"]["picUrl"].get<std::string>();
    return "";
}

static std::string searchArtists(const nlohmann::json& song) {
    std::string artists;
    if (!song.contains("ar") || !song["ar"].is_array()) return artists;
    for (const auto& artist : song["ar"]) {
        if (!artist.contains("name")) continue;
        if (!artists.empty()) artists += " / ";
        artists += artist["name"].get<std::string>();
    }
    return artists;
}

SearchPage::SearchPage() {
    this->setAxis(brls::Axis::COLUMN);

    brls::Box* bar = new brls::Box(brls::Axis::ROW);
    bar->setPadding(16, 16, 8, 16);
    bar->setAlignItems(brls::AlignItems::CENTER);

    brls::Button* button = new brls::Button();
    button->setText("music/search/button"_i18n);
    button->setWidth(240);
    button->registerClickAction([this](brls::View* view) {
        this->openKeyboard();
        return true;
    });
    bar->addView(button);

    this->hintLabel = new brls::Label();
    this->hintLabel->setText("music/search/hint"_i18n);
    this->hintLabel->setFontSize(16);
    this->hintLabel->setMarginLeft(16);
    this->hintLabel->setTextColor(
        brls::Application::getTheme()["brls/text_disabled"]);
    bar->addView(this->hintLabel);

    this->grid = new nxkit::CoverGridPage();
    this->grid->setOnSelect([](const nxkit::CoverItem& item) {
        brls::Application::notify("music/toast/playback_todo"_i18n);
    });
    this->grid->setOnNextPage([this] { this->loadNextPage(); });
    this->grid->setEmpty("music/search/empty"_i18n);

    this->addView(bar);
    this->addView(this->grid);
}

SearchPage::~SearchPage() { *this->alive = false; }

void SearchPage::openKeyboard() {
    brls::Application::getImeManager()->openForText(
        [this](std::string text) {
            if (text.empty()) return;
            this->search(text);
        },
        "music/search/title"_i18n, "music/search/placeholder"_i18n, 64,
        this->keywords);
}

void SearchPage::search(const std::string& text) {
    this->keywords = text;
    this->offset = 0;
    this->songCount = 0;
    this->noMore = false;
    this->loading = false;
    this->hintLabel->setText(text);
    this->grid->showSkeleton(12);

    NeteaseClient::instance().searchSongs(
        text, 0, kSearchPageSize,
        [this, alive = this->alive](const nlohmann::json& json,
                                    const std::string& error) {
            brls::sync([this, alive, json, error] {
                if (!*alive) return;
                this->loading = false;
                if (!error.empty()) {
                    this->showError(error);
                    return;
                }
                this->offset = 0;
                this->showSongs(json, true);
            });
        });
}

void SearchPage::loadNextPage() {
    if (this->keywords.empty() || this->loading || this->noMore) return;
    this->loading = true;

    NeteaseClient::instance().searchSongs(
        this->keywords, this->offset, kSearchPageSize,
        [this, alive = this->alive](const nlohmann::json& json,
                                    const std::string& error) {
            brls::sync([this, alive, json, error] {
                if (!*alive) return;
                this->loading = false;
                if (!error.empty()) {
                    this->showError(error);
                    return;
                }
                this->showSongs(json, false);
            });
        });
}

void SearchPage::showSongs(const nlohmann::json& json, bool firstPage) {
    const nlohmann::json* songs = nullptr;
    if (json.contains("result") && json["result"].is_object()) {
        const nlohmann::json& result = json["result"];
        if (result.contains("songCount") && result["songCount"].is_number())
            this->songCount = result["songCount"].get<int>();
        if (result.contains("songs") && result["songs"].is_array())
            songs = &result["songs"];
    }

    if (!songs) {
        this->showError("music/error/empty"_i18n);
        return;
    }

    std::vector<nxkit::CoverItem> items;
    for (const auto& song : *songs) {
        nxkit::CoverItem item;
        if (song.contains("id")) item.id = std::to_string(song["id"].get<long>());
        item.title = song.value("name", std::string());
        item.cover = searchCover(song);
        item.subtitle = searchArtists(song);
        if (song.contains("dt") && song["dt"].is_number())
            item.badge = wiliwili::sec2Time(song["dt"].get<long>() / 1000);
        items.push_back(std::move(item));
    }

    this->offset += (int)items.size();
    if ((int)items.size() < kSearchPageSize ||
        (this->songCount > 0 && this->offset >= this->songCount))
        this->noMore = true;

    if (firstPage) {
        if (items.empty())
            this->grid->setEmpty("music/error/empty"_i18n);
        else
            this->grid->setItems(items);
    } else if (!items.empty()) {
        this->grid->appendItems(items);
    }
}

void SearchPage::showError(const std::string& error) {
    this->grid->setError(error);
}

}  // namespace music
