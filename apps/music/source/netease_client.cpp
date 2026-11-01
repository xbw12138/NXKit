//
// music: thin client for a self hosted NeteaseCloudMusicApiEnhanced server.
//

#include "music/netease_client.hpp"

#include "nxkit/auth_store.hpp"
#include "nxkit/config.hpp"

namespace music {

static const char* kApiBaseUrlKey = "api.baseUrl";
static const char* kDefaultBaseUrl = "http://127.0.0.1:3000";

NeteaseClient& NeteaseClient::instance() {
    static NeteaseClient client;
    return client;
}

NeteaseClient::NeteaseClient() { this->loadSettings(); }

void NeteaseClient::loadSettings() {
    std::string url =
        nxkit::Config::instance().getString(kApiBaseUrlKey, kDefaultBaseUrl);
    this->http.setBaseUrl(url);
    this->http.setHeader("User-Agent",
                         "Mozilla/5.0 (Nintendo Switch) nxkit-music/1.0");
    this->setCookie(nxkit::AuthStore::instance().cookie());
}

std::string NeteaseClient::baseUrl() const { return this->http.baseUrl(); }

void NeteaseClient::setBaseUrl(const std::string& url) {
    nxkit::Config::instance().setString(kApiBaseUrlKey, url);
    nxkit::Config::instance().save();
    this->http.setBaseUrl(url);
}

void NeteaseClient::setCookie(const std::string& cookie) {
    nxkit::AuthStore::instance().set("cookie", cookie);
    this->http.setCookie(cookie);
}

void NeteaseClient::logout() {
    nxkit::AuthStore::instance().clear();
    this->http.setCookie("");
}

bool NeteaseClient::isLoggedIn() {
    return !nxkit::AuthStore::instance().cookie().empty();
}

static void handleResult(const nxkit::HttpResult& result,
                         const NeteaseClient::JsonCallback& callback) {
    if (!result.ok) {
        callback(nlohmann::json(),
                 result.error.empty() ? "network error" : result.error);
        return;
    }

    nlohmann::json json = result.json();
    if (!json.is_object()) {
        callback(nlohmann::json(), "invalid response");
        return;
    }

    if (json.contains("code") && json["code"].is_number()) {
        int code = json["code"].get<int>();
        if (code != 200) {
            std::string message =
                json.value("message", "request failed (code " +
                                          std::to_string(code) + ")");
            callback(json, message);
            return;
        }
    }

    callback(json, "");
}

void NeteaseClient::get(const std::string& path, const nxkit::Query& query,
                        JsonCallback callback) {
    this->http.getAsync(path, query,
                        [callback](const nxkit::HttpResult& result) {
                            handleResult(result, callback);
                        });
}

void NeteaseClient::personalized(int limit, JsonCallback callback) {
    this->get("/personalized", {{"limit", std::to_string(limit)}},
              std::move(callback));
}

void NeteaseClient::toplist(JsonCallback callback) {
    this->get("/toplist", {}, std::move(callback));
}

void NeteaseClient::playlistTracks(long playlistId, int offset, int limit,
                                   JsonCallback callback) {
    this->get("/playlist/track/all",
              {{"id", std::to_string(playlistId)},
               {"offset", std::to_string(offset)},
               {"limit", std::to_string(limit)}},
              std::move(callback));
}

void NeteaseClient::searchSongs(const std::string& keywords, int offset,
                                int limit, JsonCallback callback) {
    this->get("/search",
              {{"keywords", keywords},
               {"type", "1"},
               {"offset", std::to_string(offset)},
               {"limit", std::to_string(limit)}},
              std::move(callback));
}

void NeteaseClient::loginStatus(
    std::function<void(bool, const std::string&, const std::string&)> callback) {
    if (!this->isLoggedIn()) {
        callback(false, "", "");
        return;
    }

    this->http.getAsync(
        "/login/status",
        {{"timestamp", std::to_string(std::time(nullptr))}},
        [callback](const nxkit::HttpResult& result) {
            if (!result.ok) {
                callback(false, "", result.error);
                return;
            }

            nlohmann::json json = result.json();
            if (!json.is_object() || !json.contains("profile") ||
                json["profile"].is_null()) {
                callback(false, "", "");
                return;
            }

            std::string nickname =
                json["profile"].value("nickname", std::string());
            callback(true, nickname, "");
        });
}

void NeteaseClient::qrKey(
    std::function<void(const std::string&, const std::string&)> callback) {
    this->get("/login/qr/key", {}, [callback](const nlohmann::json& json,
                                              const std::string& error) {
        if (!error.empty()) {
            callback("", error);
            return;
        }
        if (!json.contains("data") || !json["data"].contains("unikey")) {
            callback("", "missing unikey");
            return;
        }
        callback(json["data"]["unikey"].get<std::string>(), "");
    });
}

void NeteaseClient::qrCreate(
    const std::string& key,
    std::function<void(const std::string&, const std::string&)> callback) {
    this->get("/login/qr/create",
              {{"key", key}, {"qrimg", "true"}},
              [callback](const nlohmann::json& json, const std::string& error) {
                  if (!error.empty()) {
                      callback("", error);
                      return;
                  }
                  if (!json.contains("data") ||
                      !json["data"].contains("qrurl")) {
                      callback("", "missing qrurl");
                      return;
                  }
                  callback(json["data"]["qrurl"].get<std::string>(), "");
              });
}

void NeteaseClient::qrCheck(const std::string& key, QrCheckCallback callback) {
    this->http.getAsync(
        "/login/qr/check", {{"key", key}, {"timestamp", std::to_string(std::time(nullptr))}},
        [callback](const nxkit::HttpResult& result) {
            if (!result.ok) {
                callback(0, "", "", result.error.empty() ? "network error"
                                                         : result.error);
                return;
            }

            nlohmann::json json = result.json();
            if (!json.is_object() || !json.contains("code")) {
                callback(0, "", "", "invalid response");
                return;
            }

            int code = json["code"].get<int>();
            std::string message = json.value("message", std::string());
            std::string cookie = json.value("cookie", std::string());
            callback(code, message, cookie, "");
        });
}

}  // namespace music
