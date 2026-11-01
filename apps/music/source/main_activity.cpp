//
// music: main activity with the four tabs.
//

#include "music/main_activity.hpp"

#include "music/pages/browse_page.hpp"
#include "music/pages/profile_page.hpp"
#include "music/pages/search_page.hpp"
#include "nxkit/ui/auto_tab_frame.hpp"

using namespace brls::literals;

namespace music {

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

    addTab(tabs, "music/tab/recommend"_i18n, [] {
        return new BrowsePage(BrowsePage::Mode::Personalized);
    });
    addTab(tabs, "music/tab/ranking"_i18n,
           [] { return new BrowsePage(BrowsePage::Mode::Toplist); });
    addTab(tabs, "music/tab/search"_i18n, [] { return new SearchPage(); });
    addTab(tabs, "music/tab/profile"_i18n, [] { return new ProfilePage(); });

    this->setContentView(tabs);
}

}  // namespace music
