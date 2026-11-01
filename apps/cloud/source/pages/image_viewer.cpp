//
// cloud: fullscreen image viewer (L/R to flip pages).
//

#include "cloud/pages/image_viewer.hpp"

#include <thread>

#include "nxkit/http_client.hpp"

using namespace brls::literals;

namespace cloud {

ImageViewer::ImageViewer(std::shared_ptr<DriveProvider> provider,
                         std::vector<DriveFile> images, size_t index)
    : provider(std::move(provider)),
      images(std::move(images)),
      index(index) {
    this->setAxis(brls::Axis::COLUMN);
    this->setGrow(1);

    this->image = new brls::Image();
    this->image->setGrow(1);
    this->image->setScalingType(brls::ImageScalingType::FIT);
    this->addView(this->image);

    this->statusLabel = new brls::Label();
    this->statusLabel->setFontSize(18);
    this->statusLabel->setMargins(12, 16, 12, 16);
    this->addView(this->statusLabel);

    this->registerAction(
        "cloud/image/prev"_i18n, brls::BUTTON_LB,
        [this](brls::View*) -> bool {
            if (this->images.empty()) return true;
            this->index =
                (this->index + this->images.size() - 1) % this->images.size();
            this->showCurrent();
            return true;
        });
    this->registerAction(
        "cloud/image/next"_i18n, brls::BUTTON_RB,
        [this](brls::View*) -> bool {
            if (this->images.empty()) return true;
            this->index = (this->index + 1) % this->images.size();
            this->showCurrent();
            return true;
        });

    this->showCurrent();
}

ImageViewer::~ImageViewer() { *this->alive = false; }

void ImageViewer::showCurrent() {
    if (this->index >= this->images.size()) return;
    const DriveFile file = this->images[this->index];
    const size_t generation = ++this->generation;

    this->image->clear();
    this->statusLabel->setText("cloud/image/loading"_i18n);

    this->provider->downloadInfo(
        file.id,
        [this, alive = this->alive, generation](
            const std::string& url, const std::string& headers,
            const std::string& error) {
            if (!error.empty() || url.empty()) {
                brls::sync([this, alive, generation, error] {
                    if (!*alive || generation != this->generation) return;
                    this->statusLabel->setText(
                        error.empty() ? "cloud/image/error"_i18n : error);
                });
                return;
            }
            // The download url needs the provider's cookie; fetch the bytes
            // ourselves (ImageHelper cannot attach custom headers).
            std::thread([this, alive, generation, url, headers] {
                nxkit::HttpClient client;
                // Split the CRLF separated header block into pairs.
                size_t pos = 0;
                while (pos < headers.size()) {
                    size_t end = headers.find("\r\n", pos);
                    if (end == std::string::npos) end = headers.size();
                    std::string line = headers.substr(pos, end - pos);
                    pos = end + 2;
                    size_t colon = line.find(':');
                    if (colon == std::string::npos) continue;
                    std::string name = line.substr(0, colon);
                    std::string value = line.substr(colon + 1);
                    while (!value.empty() && value.front() == ' ')
                        value.erase(value.begin());
                    if (!name.empty() && !value.empty())
                        client.setHeader(name, value);
                }
                client.setTimeout(30000);
                nxkit::HttpResult result = client.get(url);
                brls::sync([this, alive, generation, result] {
                    if (!*alive || generation != this->generation) return;
                    if (!result.ok || result.body.empty()) {
                        this->statusLabel->setText(
                            result.error.empty() ? "cloud/image/error"_i18n
                                                 : result.error);
                        return;
                    }
                    this->image->setImageFromMem(
                        (unsigned char*)result.body.data(),
                        (int)result.body.size());
                    this->statusLabel->setText(
                        std::to_string(this->index + 1) + " / " +
                        std::to_string(this->images.size()));
                });
            }).detach();
        });
}

}  // namespace cloud
