#include "nxkit/http_client.hpp"

#include <algorithm>
#include <cctype>
#include <map>

namespace nxkit {

namespace {

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

HttpResult fromResponse(const cpr::Response& response) {
    HttpResult result;
    result.status = response.status_code;
    result.body = response.text;

    for (const auto& header : response.header) {
        if (lower(header.first) == "set-cookie") {
            if (!result.setCookie.empty()) result.setCookie += "; ";
            result.setCookie += header.second;
        }
    }

    if (response.error) {
        result.error = response.error.message;
        return result;
    }

    result.ok = response.status_code >= 200 && response.status_code < 300;
    if (!result.ok) {
        result.error = "HTTP " + std::to_string(response.status_code);
    }
    return result;
}

}  // namespace

nlohmann::json HttpResult::json() const {
    nlohmann::json parsed = nlohmann::json::parse(body, nullptr, false);
    return parsed;
}

void HttpClient::setBaseUrl(const std::string& url) {
    base = url;
    while (!base.empty() && base.back() == '/') base.pop_back();
}

void HttpClient::setHeader(const std::string& name, const std::string& value) {
    extraHeaders[name] = value;
}

void HttpClient::removeHeader(const std::string& name) { extraHeaders.erase(name); }

void HttpClient::clearHeaders() { extraHeaders.clear(); }

std::string HttpClient::urlFor(const std::string& path) const {
    if (path.empty()) return base;
    if (path[0] == '/') return base + path;
    return base + "/" + path;
}

cpr::Header HttpClient::headers() const {
    cpr::Header header;
    for (const auto& entry : extraHeaders) header[entry.first] = entry.second;
    if (!cookieValue.empty()) header["Cookie"] = cookieValue;
    if (!bearer.empty()) header["Authorization"] = "Bearer " + bearer;
    return header;
}

int HttpClient::timeoutFor(int timeout) const { return timeout > 0 ? timeout : timeoutMs; }

cpr::Parameters HttpClient::toParameters(const Query& query) {
    cpr::Parameters parameters;
    for (const auto& pair : query) parameters.Add(cpr::Parameter(pair.first, pair.second));
    return parameters;
}

cpr::Payload HttpClient::toPayload(const Query& form) {
    std::vector<cpr::Pair> pairs;
    pairs.reserve(form.size());
    for (const auto& pair : form) pairs.emplace_back(pair.first, pair.second);
    return cpr::Payload(pairs.begin(), pairs.end());
}

HttpResult HttpClient::get(const std::string& path, const Query& query, int timeout) const {
    cpr::Response response = cpr::Get(cpr::Url{urlFor(path)}, toParameters(query), headers(),
                                      cpr::VerifySsl{verifySsl}, cpr::Timeout{timeoutFor(timeout)});
    return fromResponse(response);
}

HttpResult HttpClient::post(const std::string& path, const Query& query, const Query& form,
                            int timeout) const {
    cpr::Response response =
        cpr::Post(cpr::Url{urlFor(path)}, toParameters(query), headers(), toPayload(form),
                  cpr::VerifySsl{verifySsl}, cpr::Timeout{timeoutFor(timeout)});
    return fromResponse(response);
}

HttpResult HttpClient::postJson(const std::string& path, const Query& query,
                                const nlohmann::json& body, int timeout) const {
    cpr::Header header = headers();
    header["Content-Type"] = "application/json";

    cpr::Response response =
        cpr::Post(cpr::Url{urlFor(path)}, toParameters(query), header, cpr::Body{body.dump()},
                  cpr::VerifySsl{verifySsl}, cpr::Timeout{timeoutFor(timeout)});
    return fromResponse(response);
}

void HttpClient::getAsync(const std::string& path, const Query& query,
                          std::function<void(const HttpResult&)> callback, int timeout) const {
    cpr::GetCallback([callback](const cpr::Response& response) { callback(fromResponse(response)); },
                     cpr::Url{urlFor(path)}, toParameters(query), headers(),
                     cpr::VerifySsl{verifySsl}, cpr::Timeout{timeoutFor(timeout)});
}

void HttpClient::postAsync(const std::string& path, const Query& query, const Query& form,
                           std::function<void(const HttpResult&)> callback, int timeout) const {
    cpr::PostCallback([callback](const cpr::Response& response) { callback(fromResponse(response)); },
                      cpr::Url{urlFor(path)}, toParameters(query), headers(), toPayload(form),
                      cpr::VerifySsl{verifySsl}, cpr::Timeout{timeoutFor(timeout)});
}

}  // namespace nxkit
