//
// cloud: file browser page (list, navigate, file operations).
//

#include "cloud/pages/file_list_page.hpp"

#include <ctime>

#include "cloud/drive/drive_manager.hpp"
#include "cloud/pages/image_viewer.hpp"
#include "cloud/pages/player_page.hpp"
#include "nxkit/ui/recycling_grid.hpp"

using namespace brls::literals;

namespace cloud {

static const int kPageSize = 50;

static std::string formatSize(uint64_t bytes) {
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    double value = (double)bytes;
    int unit = 0;
    while (value >= 1024.0 && unit < 4) {
        value /= 1024.0;
        unit++;
    }
    char buf[64];
    if (unit == 0)
        snprintf(buf, sizeof(buf), "%d B", (int)bytes);
    else
        snprintf(buf, sizeof(buf), "%.1f %s", value, units[unit]);
    return buf;
}

static std::string formatDate(int64_t ms) {
    if (ms <= 0) return "";
    time_t sec = (time_t)(ms / 1000);
    struct tm tmv {};
    localtime_r(&sec, &tmv);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d", &tmv);
    return buf;
}

static std::string typeTagOf(const DriveFile& file) {
    if (file.isDir) return "cloud/files/type/dir"_i18n;
    switch (file.mediaKind()) {
        case MediaKind::Video: return "cloud/files/type/video"_i18n;
        case MediaKind::Audio: return "cloud/files/type/audio"_i18n;
        case MediaKind::Image: return "cloud/files/type/image"_i18n;
        default: return "cloud/files/type/other"_i18n;
    }
}

// -----------------------------------------------------------------------------
// FileRowCell
// -----------------------------------------------------------------------------

class FileRowCell : public RecyclingGridItem {
public:
    explicit FileRowCell(FileListPage* page) : page(page) {
        this->setAxis(brls::Axis::ROW);
        this->setAlignItems(brls::AlignItems::CENTER);
        this->setFocusable(true);
        this->setPaddingLeft(16);
        this->setPaddingRight(16);

        this->typeLabel = new brls::Label();
        this->typeLabel->setWidth(96);
        this->typeLabel->setFontSize(16);
        this->typeLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
        this->typeLabel->setMarginRight(12);
        this->addView(this->typeLabel);

        this->nameLabel = new brls::Label();
        this->nameLabel->setGrow(1);
        this->nameLabel->setFontSize(20);
        this->addView(this->nameLabel);

        this->sizeLabel = new brls::Label();
        this->sizeLabel->setWidth(120);
        this->sizeLabel->setFontSize(16);
        this->sizeLabel->setHorizontalAlign(brls::HorizontalAlign::RIGHT);
        this->sizeLabel->setMarginRight(16);
        this->addView(this->sizeLabel);

        this->dateLabel = new brls::Label();
        this->dateLabel->setWidth(130);
        this->dateLabel->setFontSize(16);
        this->dateLabel->setHorizontalAlign(brls::HorizontalAlign::RIGHT);
        this->addView(this->dateLabel);

        // X opens the item actions menu. getIndex() resolves at press time,
        // so reused cells always act on the currently bound item.
        this->registerAction(
            "", brls::BUTTON_X,
            [this](brls::View*) -> bool {
                this->page->showItemActions(this->getIndex());
                return true;
            },
            true);
    }

    void setFile(const DriveFile& file) {
        this->typeLabel->setText(typeTagOf(file));
        this->nameLabel->setText(file.name);
        this->sizeLabel->setText(file.isDir ? "-" : formatSize(file.size));
        this->dateLabel->setText(formatDate(file.updatedAt));
    }

    static RecyclingGridItem* create(FileListPage* page) {
        return new FileRowCell(page);
    }

private:
    FileListPage* page;
    brls::Label* typeLabel;
    brls::Label* nameLabel;
    brls::Label* sizeLabel;
    brls::Label* dateLabel;
};

// -----------------------------------------------------------------------------
// FileListDataSource
// -----------------------------------------------------------------------------

class FileListDataSource : public RecyclingGridDataSource {
public:
    explicit FileListDataSource(FileListPage* page) : page(page) {}

    size_t getItemCount() override { return this->files.size(); }

    RecyclingGridItem* cellForRow(RecyclingGrid* recycler,
                                  size_t index) override {
        auto* cell = (FileRowCell*)recycler->dequeueReusableCell("file");
        cell->setFile(this->files[index]);
        return cell;
    }

    float heightForRow(RecyclingGrid*, size_t) override { return 64; }

    void onItemSelected(RecyclingGrid*, size_t index) override {
        this->page->openItem(index);
    }

    void clearData() override { this->files.clear(); }

    FileListPage* page;
    std::vector<DriveFile> files;
};

// -----------------------------------------------------------------------------
// FileListPage
// -----------------------------------------------------------------------------

FileListPage::FileListPage() : pickerMode(false) {
    this->provider = DriveManager::instance().current();
    this->pathStack.emplace_back(this->provider->rootId(), "");
    this->setup();
    this->refresh();
}

FileListPage::FileListPage(
    std::function<void(const std::string& dirId, const std::string& dirName)>
        onPick)
    : pickerMode(true), pickCallback(std::move(onPick)) {
    this->provider = DriveManager::instance().current();
    this->pathStack.emplace_back(this->provider->rootId(), "");
    this->setup();
    this->refresh();
}

FileListPage::~FileListPage() { *this->alive = false; }

void FileListPage::setup() {
    this->setAxis(brls::Axis::COLUMN);

    brls::Box* header = new brls::Box(brls::Axis::ROW);
    header->setAlignItems(brls::AlignItems::CENTER);
    header->setPadding(12, 16, 8, 16);

    this->breadcrumb = new brls::Label();
    this->breadcrumb->setGrow(1);
    this->breadcrumb->setFontSize(20);
    header->addView(this->breadcrumb);

    this->hintLabel = new brls::Label();
    this->hintLabel->setFontSize(16);
    this->hintLabel->setText(this->pickerMode ? "cloud/files/hint_picker"_i18n
                                              : "cloud/files/hint_browse"_i18n);
    header->addView(this->hintLabel);

    this->addView(header);

    this->grid = new RecyclingGrid();
    this->grid->setGrow(1);
    this->grid->spanCount = 1;
    this->grid->estimatedRowHeight = 72;
    this->grid->estimatedRowSpace = 8;
    this->grid->setPaddingBottom(16);
    this->grid->setPaddingLeft(12);
    this->grid->setPaddingRight(12);

    auto* source = new FileListDataSource(this);
    this->dataSource = source;
    this->grid->registerCell("file", [this] { return FileRowCell::create(this); });
    this->grid->setDataSource(source);
    this->grid->onNextPage([this] { this->loadNextPage(); });

    this->addView(this->grid);

    // B: leave the current folder first, then the page.
    this->registerAction(
        "", brls::BUTTON_B,
        [this](brls::View*) -> bool { return this->goUp(); }, true);

    if (this->pickerMode) {
        // Y picks the current directory.
        this->registerAction(
            "cloud/files/pick_here"_i18n, brls::BUTTON_Y,
            [this](brls::View*) -> bool {
                if (this->pickCallback)
                    this->pickCallback(this->pathStack.back().first,
                                       this->pathStack.back().second);
                brls::Application::popActivity();
                return true;
            });
    } else {
        // Y creates a folder inside the current directory.
        this->registerAction(
            "cloud/files/mkdir"_i18n, brls::BUTTON_Y,
            [this](brls::View*) -> bool {
                this->promptMakeDir();
                return true;
            });
    }

    this->updateBreadcrumb();
}

void FileListPage::updateBreadcrumb() {
    std::string path = "cloud/files/root"_i18n;
    for (size_t i = 1; i < this->pathStack.size(); i++)
        path += " / " + this->pathStack[i].second;
    this->breadcrumb->setText(path);
}

void FileListPage::refresh() {
    if (!this->provider->isLoggedIn()) {
        ((FileListDataSource*)this->dataSource)->files.clear();
        this->grid->notifyDataChanged();
        this->grid->setError("cloud/files/login_hint"_i18n);
        return;
    }
    this->page = 0;
    this->total = -1;
    this->loading = false;
    ((FileListDataSource*)this->dataSource)->files.clear();
    this->grid->notifyDataChanged();
    this->grid->showSkeleton(10);
    this->loadNextPage();
}

void FileListPage::loadNextPage() {
    if (this->loading) return;
    if (this->total >= 0 &&
        (int)((FileListDataSource*)this->dataSource)->files.size() >=
            this->total)
        return;
    this->loading = true;
    int nextPage = this->page + 1;
    std::string dirId = this->pathStack.back().first;

    this->provider->listFiles(
        dirId, nextPage, kPageSize,
        [this, alive = this->alive, dirId](const std::vector<DriveFile>& files,
                                           int total,
                                           const std::string& error) {
            brls::sync([this, alive, dirId, files, total, error] {
                if (!*alive) return;
                // Stale response after navigating away.
                if (dirId != this->pathStack.back().first) return;
                this->loading = false;
                this->applyFiles(files, total, this->page == 0, error);
            });
        });
}

void FileListPage::applyFiles(const std::vector<DriveFile>& files, int total,
                              bool firstPage, const std::string& error) {
    auto* source = (FileListDataSource*)this->dataSource;
    if (!error.empty()) {
        if (firstPage)
            this->grid->setError(error);
        else
            brls::Application::notify(error);
        return;
    }

    this->page += 1;
    this->total = total;

    for (const auto& file : files) {
        if (this->pickerMode && !file.isDir) continue;
        source->files.push_back(file);
    }

    if (source->files.empty())
        this->grid->setEmpty(this->pickerMode ? "cloud/files/empty_picker"_i18n
                                              : "cloud/files/empty"_i18n);
    else
        this->grid->notifyDataChanged();
}

void FileListPage::enterFolder(const std::string& dirId,
                               const std::string& name) {
    this->pathStack.emplace_back(dirId, name);
    this->updateBreadcrumb();
    this->refresh();
}

bool FileListPage::goUp() {
    if (this->pathStack.size() <= 1) return false;  // let the activity pop
    this->pathStack.pop_back();
    this->updateBreadcrumb();
    this->refresh();
    return true;
}

void FileListPage::openItem(size_t index) {
    auto* source = (FileListDataSource*)this->dataSource;
    if (index >= source->files.size()) return;
    const DriveFile& file = source->files[index];

    if (file.isDir) {
        this->enterFolder(file.id, file.name);
        return;
    }
    if (this->pickerMode) return;

    switch (file.mediaKind()) {
        case MediaKind::Video:
            brls::Application::pushActivity(new brls::Activity(
                new PlayerPage(this->provider, file, false)));
            break;
        case MediaKind::Audio:
            brls::Application::pushActivity(new brls::Activity(
                new PlayerPage(this->provider, file, true)));
            break;
        case MediaKind::Image: {
            std::vector<DriveFile> images;
            size_t start = 0;
            for (const auto& f : source->files) {
                if (f.mediaKind() != MediaKind::Image) continue;
                if (f.id == file.id) start = images.size();
                images.push_back(f);
            }
            brls::Application::pushActivity(new brls::Activity(
                new ImageViewer(this->provider, std::move(images), start)));
            break;
        }
        default:
            brls::Application::notify("cloud/files/unsupported"_i18n);
            break;
    }
}

void FileListPage::showItemActions(size_t index) {
    if (this->pickerMode) return;
    auto* source = (FileListDataSource*)this->dataSource;
    if (index >= source->files.size()) return;
    DriveFile file = source->files[index];

    std::vector<std::string> options = {
        "cloud/files/action/rename"_i18n,
        "cloud/files/action/delete"_i18n,
        "cloud/files/action/move"_i18n,
    };
    new brls::Dropdown(file.name, options, [this, file](int selected) {
        switch (selected) {
            case 0: this->promptRename(file); break;
            case 1: this->confirmDelete(file); break;
            case 2: this->pickMoveTarget(file); break;
            default: break;
        }
    });
}

void FileListPage::promptMakeDir() {
    brls::Application::getImeManager()->openForText(
        [this](std::string name) {
            if (name.empty()) return;
            this->provider->makeDir(
                this->pathStack.back().first, name,
                [this, alive = this->alive](const std::string& error) {
                    brls::sync([this, alive, error] {
                        if (!*alive) return;
                        if (!error.empty())
                            brls::Application::notify(error);
                        else
                            this->refresh();
                    });
                });
        },
        "cloud/files/mkdir"_i18n, "", 64);
}

void FileListPage::promptRename(const DriveFile& file) {
    brls::Application::getImeManager()->openForText(
        [this, file](std::string name) {
            if (name.empty() || name == file.name) return;
            this->provider->rename(
                file.id, name,
                [this, alive = this->alive](const std::string& error) {
                    brls::sync([this, alive, error] {
                        if (!*alive) return;
                        if (!error.empty())
                            brls::Application::notify(error);
                        else
                            this->refresh();
                    });
                });
        },
        "cloud/files/action/rename"_i18n, "", 255, file.name);
}

void FileListPage::confirmDelete(const DriveFile& file) {
    auto* dialog = new brls::Dialog(
        "cloud/files/delete_confirm"_i18n + ": " + file.name);
    dialog->addButton("cloud/files/action/cancel"_i18n, [] {});
    dialog->addButton("cloud/files/action/delete"_i18n, [this, file] {
        this->provider->remove(
            {file.id}, [this, alive = this->alive](const std::string& error) {
                brls::sync([this, alive, error] {
                    if (!*alive) return;
                    if (!error.empty())
                        brls::Application::notify(error);
                    else
                        this->refresh();
                });
            });
    });
    dialog->setCancelable(true);
    dialog->open();
}

void FileListPage::pickMoveTarget(const DriveFile& file) {
    auto* picker = new FileListPage(
        [this, file](const std::string& dirId, const std::string&) {
            this->provider->move(
                {file.id}, dirId,
                [this, alive = this->alive](const std::string& error) {
                    brls::sync([this, alive, error] {
                        if (!*alive) return;
                        if (!error.empty())
                            brls::Application::notify(error);
                        else
                            this->refresh();
                    });
                });
        });
    brls::Application::pushActivity(new brls::Activity(picker));
}

}  // namespace cloud
