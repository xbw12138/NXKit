//
// cloud: entry point of the cloud drive app.
//

#include <borealis.hpp>

#include "cloud/drive/drive_manager.hpp"
#include "cloud/main_activity.hpp"
#include "nxkit/app.hpp"

int main(int argc, char** argv) {
    nxkit::AppConfig config;
    config.title = "NXKit Cloud";
    // Quark-ish green accent.
    config.lightAccent = nvgRGB(0, 160, 110);
    config.darkAccent = nvgRGB(64, 200, 150);

    // Created after runApp() has loaded the config, so providers see the
    // credentials saved by a previous session.
    config.createActivity = [] {
        cloud::DriveManager::instance().init();
        return new cloud::MainActivity();
    };

    return nxkit::runApp(config);
}
