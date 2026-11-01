//
// music: account + framework settings page.
//

#pragma once

#include <memory>
#include <string>

#include "nxkit/ui/settings_page.hpp"

namespace music {

class ProfilePage : public nxkit::SettingsPage {
public:
    ProfilePage();

    void willAppear(bool resetState = false) override;

private:
    void refreshAccount();
    void rebuild();

    std::shared_ptr<bool> alive = std::make_shared<bool>(true);

    bool loggedIn = false;
    std::string nickname;
};

}  // namespace music
