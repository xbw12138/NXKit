//
// music: account + framework settings page.
//

#include "music/pages/profile_page.hpp"

#include "music/netease_client.hpp"
#include "music/pages/login_page.hpp"

using namespace brls::literals;

namespace music {

ProfilePage::ProfilePage() {
    this->rebuild();
    this->refreshAccount();
}

void ProfilePage::willAppear(bool resetState) {
    nxkit::SettingsPage::willAppear(resetState);
    this->refreshAccount();
}

void ProfilePage::refreshAccount() {
    NeteaseClient::instance().loginStatus(
        [this, alive = this->alive](bool loggedIn, const std::string& name,
                                    const std::string& error) {
            brls::sync([this, alive, loggedIn, name] {
                if (!*alive) return;
                this->loggedIn = loggedIn;
                this->nickname = name;
                this->rebuild();
            });
        });
}

void ProfilePage::rebuild() {
    this->clear();

    this->addSection("music/profile/section/account"_i18n);
    if (this->loggedIn) {
        this->addInfo("music/profile/nickname"_i18n,
                      this->nickname.empty() ? "music/profile/unknown"_i18n
                                             : this->nickname);
        this->addAction("music/profile/logout"_i18n, "", [this] {
            NeteaseClient::instance().logout();
            this->loggedIn = false;
            this->nickname.clear();
            this->rebuild();
            brls::Application::notify("music/profile/logged_out"_i18n);
        });
    } else {
        this->addAction("music/profile/login"_i18n, "", [] {
            brls::Application::pushActivity(new LoginPage());
        });
    }

    this->addSection("music/profile/section/server"_i18n);
    this->addInput(
        "music/profile/api_server"_i18n, NeteaseClient::instance().baseUrl(),
        [](const std::string& value) {
            if (value.empty()) return;
            NeteaseClient::instance().setBaseUrl(value);
            brls::Application::notify("music/profile/server_saved"_i18n);
        },
        "http://127.0.0.1:3000", "music/profile/api_server_hint"_i18n);

    this->addSection("music/profile/section/about"_i18n);
    this->addInfo("music/profile/version"_i18n, "1.0.0");
    this->addAction("music/profile/refresh"_i18n, "", [this] {
        this->refreshAccount();
        brls::Application::notify("music/profile/refreshing"_i18n);
    });

    this->reload();
}

}  // namespace music
