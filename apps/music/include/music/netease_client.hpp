//
// music: thin client for a self hosted NeteaseCloudMusicApiEnhanced server.
//

#pragma once

#include <nlohmann/json.hpp>

#include <functional>
#include <string>

#include "nxkit/http_client.hpp"

namespace music {

/// Wraps the NeteaseCloudMusicApiEnhanced endpoints used by the sample app.
/// All callbacks are invoked on a worker thread, UI code must marshal back
/// to the main thread with brls::sync.
class NeteaseClient {
public:
    static NeteaseClient& instance();

    /// Loads the base url and cookie from the framework config store.
    void loadSettings();

    std::string baseUrl() const;
    void setBaseUrl(const std::string& url);

    void setCookie(const std::string& cookie);
    void logout();
    bool isLoggedIn();

    /// Callback receives the parsed json and an empty error on success.
    using JsonCallback =
        std::function<void(const nlohmann::json& json, const std::string& error)>;

    void personalized(int limit, JsonCallback callback);
    void toplist(JsonCallback callback);
    void playlistTracks(long playlistId, int offset, int limit,
                        JsonCallback callback);
    void searchSongs(const std::string& keywords, int offset, int limit,
                     JsonCallback callback);

    /// Reports the current account nickname (empty when not logged in).
    void loginStatus(std::function<void(bool loggedIn, const std::string& nickname,
                                        const std::string& error)> callback);

    // --- QR code login -------------------------------------------------------
    void qrKey(std::function<void(const std::string& key,
                                  const std::string& error)> callback);
    void qrCreate(const std::string& key,
                  std::function<void(const std::string& url,
                                     const std::string& error)> callback);
    /// code: 800 expired, 801 waiting, 802 scanned, 803 authorized.
    using QrCheckCallback = std::function<void(int code, const std::string& message,
                                               const std::string& cookie,
                                               const std::string& error)>;
    void qrCheck(const std::string& key, QrCheckCallback callback);

private:
    NeteaseClient();

    void get(const std::string& path, const nxkit::Query& query,
             JsonCallback callback);

    nxkit::HttpClient http;
};

}  // namespace music
