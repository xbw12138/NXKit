//
// cloud: main activity with the file browser and profile tabs.
//

#include "cloud/main_activity.hpp"

#include "cloud/pages/file_list_page.hpp"
#include "cloud/pages/profile_page.hpp"
#include "nxkit/ui/auto_tab_frame.hpp"

using namespace brls::literals;

namespace cloud {

static void addTab(AutoTabFrame* tabs, const std::string& label,
                   TabViewCreator creator) {
    AutoSidebarItem* item = new AutoSidebarItem();
    item->setTabStyle(AutoTabBarStyle::ACCENT);
    item->setLabel(label);
    tabs->addTab(item, std::move(creator));
}

MainActivity::MainActivity() {
    AutoTabFrame* tabs = new AutoTabFrame();
    tabs->setSideBarPosition(AutoTabBarPosition::TOP);
    tabs->setHorizontalMode(true);
    tabs->setFontSize(24);

    addTab(tabs, "cloud/tab/files"_i18n, [] { return new FileListPage(); });
    addTab(tabs, "cloud/tab/profile"_i18n, [] { return new ProfilePage(); });

    this->setContentView(tabs);
}

}  // namespace cloud
