//
// music: entry point of the sample app.
//

#include <borealis.hpp>

#include "music/main_activity.hpp"
#include "nxkit/app.hpp"

int main(int argc, char** argv) {
    nxkit::AppConfig config;
    config.title = "NXKit Music";

    // Netease-ish red accent, adjusted for the dark theme.
    config.lightAccent = nvgRGB(203, 43, 41);
    config.darkAccent = nvgRGB(255, 102, 153);

    config.createActivity = [] { return new music::MainActivity(); };

    return nxkit::runApp(config);
}
