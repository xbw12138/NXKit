#pragma once

#include <cpr/cpr.h>
#include <nlohmann/json.hpp>

#include <functional>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace nxkit {

using Query = std::vector<std::pair<std::string, std::string>>;

struct HttpResult {
    bool ok = false;
    long status = 0;
    std::string body;
    std::string error;
    /// Raw "Set-Cookie" header returned by the server (may be empty).
    std::string setCookie;

    /// Parses body as JSON. Returns a discarded value on failure.
    nlohmann::json json() const;
};

/// Thin, reusable wrapper around cpr: base url, default query/headers,
/// cookies and JSON helpers. Safe to call from worker threads.
class HttpClient {
public:
    void setBaseUrl(const std::string& url);
    std::string baseUrl() const { return base; }

    void setHeader(const std::string& name, const std::string& value);
    void removeHeader(const std::string& name);
    void clearHeaders();

    void setCookie(const std::string& cookie) { cookieValue = cookie; }
    const std::string& cookie() const { return cookieValue; }

    void setBearerToken(const std::string& token) { bearer = token; }
    void setVerifySsl(bool verify) { verifySsl = verify; }

    void setTimeout(int milliseconds) { timeoutMs = milliseconds; }
    int timeout() const { return timeoutMs; }

    HttpResult get(const std::string& path, const Query& query = {}, int timeout = -1) const;
    HttpResult post(const std::string& path, const Query& query = {}, const Query& form = {}, int timeout = -1) const;
    HttpResult postJson(const std::string& path, const Query& query, const nlohmann::json& body,
                        int timeout = -1) const;

    void getAsync(const std::string& path, const Query& query,
                  std::function<void(const HttpResult&)> callback, int timeout = -1) const;
    void postAsync(const std::string& path, const Query& query, const Query& form,
                   std::function<void(const HttpResult&)> callback, int timeout = -1) const;

private:
    std::string urlFor(const std::string& path) const;
    cpr::Header headers() const;
    int timeoutFor(int timeout) const;

    static cpr::Parameters toParameters(const Query& query);
    static cpr::Payload toPayload(const Query& form);

    std::string base;
    std::map<std::string, std::string> extraHeaders;
    std::string cookieValue;
    std::string bearer;
    bool verifySsl = true;
    int timeoutMs = 10000;
};

}  // namespace nxkit
