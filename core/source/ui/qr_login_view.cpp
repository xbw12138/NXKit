//
// GF: generic QR code login view.
//

#include "nxkit/ui/qr_login_view.hpp"

#include <borealis/views/button.hpp>
#include <qrcodegen.hpp>

#include "nxkit/ui/svg_image.hpp"

namespace nxkit {

static std::string qrToSvg(const std::string& content) {
    const int border = 4;
    const int scale = 8;

    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(
        content.c_str(), qrcodegen::QrCode::Ecc::MEDIUM);
    int size = qr.getSize();
    int pixels = (size + border * 2) * scale;

    std::string svg;
    svg.reserve(pixels * pixels);
    svg += "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 ";
    svg += std::to_string(pixels);
    svg += " ";
    svg += std::to_string(pixels);
    svg += "\"><rect width=\"" + std::to_string(pixels) + "\" height=\"" +
           std::to_string(pixels) + "\" fill=\"#FFFFFF\"/><path d=\"";
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            if (!qr.getModule(x, y)) continue;
            int px = (x + border) * scale;
            int py = (y + border) * scale;
            svg += "M" + std::to_string(px) + "," + std::to_string(py) + "h" +
                   std::to_string(scale) + "v" + std::to_string(scale) + "h-" +
                   std::to_string(scale) + "z";
        }
    }
    svg += "\" fill=\"#000000\"/></svg>";
    return svg;
}

QRLoginView::QRLoginView() {
    this->setAxis(brls::Axis::COLUMN);
    this->setAlignItems(brls::AlignItems::CENTER);
    this->setJustifyContent(brls::JustifyContent::CENTER);

    this->qrImage = new SVGImage();
    this->qrImage->setDimensions(320, 320);
    this->qrImage->setVisibility(brls::Visibility::GONE);
    this->addView(this->qrImage);

    this->statusLabel = new brls::Label();
    this->statusLabel->setFontSize(18);
    this->statusLabel->setHorizontalAlign(brls::HorizontalAlign::CENTER);
    this->statusLabel->setMarginTop(24);
    this->addView(this->statusLabel);

    this->refreshButton = new brls::Button();
    this->refreshButton->setText("Refresh");
    this->refreshButton->setMarginTop(16);
    this->refreshButton->setVisibility(brls::Visibility::GONE);
    this->refreshButton->registerClickAction([this](brls::View* view) {
        if (this->onRefresh) this->onRefresh();
        return true;
    });
    this->addView(this->refreshButton);
}

void QRLoginView::setQrContent(const std::string& content) {
    if (content.empty()) {
        this->qrImage->setVisibility(brls::Visibility::GONE);
        return;
    }
    this->qrImage->setImageFromSVGString(qrToSvg(content));
    this->qrImage->setVisibility(brls::Visibility::VISIBLE);
}

void QRLoginView::setStatus(const std::string& status) {
    this->statusLabel->setText(status);
}

void QRLoginView::showLoading(const std::string& message) {
    this->qrImage->setVisibility(brls::Visibility::GONE);
    this->setStatus(message);
}

void QRLoginView::setRefreshVisible(bool visible) {
    this->refreshButton->setVisibility(
        visible ? brls::Visibility::VISIBLE : brls::Visibility::GONE);
}

}  // namespace nxkit
