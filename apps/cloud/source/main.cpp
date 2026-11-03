//
// cloud: entry point of the cloud drive app.
//

#include <borealis.hpp>

#include <cstdlib>

#include "cloud/drive/drive_manager.hpp"
#include "cloud/main_activity.hpp"
#include "cloud/pages/login_page.hpp"
#include "nxkit/app.hpp"

int main(int argc, char** argv) {
    nxkit::AppConfig config;
    config.title = "NXKit Cloud";
    // Quark-ish green accent.
    config.lightAccent = nvgRGB(0, 160, 110);
    config.darkAccent = nvgRGB(64, 200, 150);

    // Temporary: CLOUD_DEBUG_LOGIN=1 opens the QR login page directly.
    const bool debugLogin = std::getenv("CLOUD_DEBUG_LOGIN") != nullptr;

    // Created after runApp() has loaded the config, so providers see the
    // credentials saved by a previous session.
    config.createActivity = [debugLogin] -> brls::Activity* {
        cloud::DriveManager::instance().init();
        if (debugLogin) return new cloud::LoginPage();
        return new cloud::MainActivity();
    };

    return nxkit::runApp(config);
}
