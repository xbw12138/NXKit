#include "nxkit/paths.hpp"

#include <sys/stat.h>

namespace nxkit {

std::string dataDirectory() {
#ifdef __SWITCH__
    return "sdmc:/switch/nxkit/";
#else
    return "./nxkit_data/";
#endif
}

std::string dataPath(const std::string& relative) {
    return dataDirectory() + relative;
}

void ensureDirectory(const std::string& path) {
    std::string current;
    for (size_t i = 0; i < path.size(); i++) {
        current += path[i];
        if (path[i] != '/' || i + 1 >= path.size()) continue;
        // Do not try to mkdir drive roots such as "sdmc:/".
        if (current.size() > 2 && current[current.size() - 2] == ':') continue;
        mkdir(current.c_str(), 0777);
    }
    if (!path.empty() && path.back() != '/') mkdir(path.c_str(), 0777);
}

void ensureDataDirectory() { ensureDirectory(dataDirectory()); }

}  // namespace nxkit
