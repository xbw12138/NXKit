//
// cloud: file browser page (list, navigate, file operations).
//

#pragma once

#include <borealis.hpp>

#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "cloud/drive/drive_provider.hpp"

class RecyclingGrid;
class RecyclingGridDataSource;

namespace cloud {

/// Browses a provider's directories. In picker mode only folders are shown
/// and Y picks the current directory (used by "move to...").
class FileListPage : public brls::Box {
public:
    /// Browse mode: lists the provider root.
    FileListPage();
    /// Picker mode: pick a directory, callback fires with its id.
    FileListPage(std::function<void(const std::string& dirId,
                                    const std::string& dirName)> onPick);
    ~FileListPage() override;

    /// Opens the item at `index` (called by the data source).
    void openItem(size_t index);
    /// Shows the actions menu for the item at `index` (X button).
    void showItemActions(size_t index);

private:
    void setup();
    void enterFolder(const std::string& dirId, const std::string& name);
    bool goUp();
    void refresh();
    void loadNextPage();
    void applyFiles(const std::vector<DriveFile>& files, int total,
                    bool firstPage, const std::string& error);
    void updateBreadcrumb();

    void promptMakeDir();
    void promptRename(const DriveFile& file);
    void confirmDelete(const DriveFile& file);
    void pickMoveTarget(const DriveFile& file);

    std::shared_ptr<bool> alive = std::make_shared<bool>(true);

    std::shared_ptr<DriveProvider> provider;
    bool pickerMode = false;
    std::function<void(const std::string&, const std::string&)> pickCallback;

    /// Path from the root: (dirId, dirName). Root is ("0", "").
    std::vector<std::pair<std::string, std::string>> pathStack;

    RecyclingGrid* grid = nullptr;
    RecyclingGridDataSource* dataSource = nullptr;
    brls::Label* breadcrumb = nullptr;
    brls::Label* hintLabel = nullptr;

    int page = 0;
    int total = -1;
    bool loading = false;
};

}  // namespace cloud
