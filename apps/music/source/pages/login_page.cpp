//
// music: QR code login page.
//

#include "music/pages/login_page.hpp"

#include <borealis/core/task.hpp>

#include "music/netease_client.hpp"
#include "nxkit/ui/qr_login_view.hpp"

using namespace brls::literals;

namespace music {

namespace {

const int kPollPeriodMs = 1500;

class QrPollTask : public brls::RepeatingTask {
public:
    explicit QrPollTask(LoginPage* page)
        : brls::RepeatingTask(kPollPeriodMs), page(page) {}

    void run() override { this->page->checkQr(); }

private:
    LoginPage* page;
};

}  // namespace

LoginPage::LoginPage() {
    brls::Box* box = new brls::Box(brls::Axis::COLUMN);
    box->setGrow(1);
    box->setAlignItems(brls::AlignItems::CENTER);
    box->setJustifyContent(brls::JustifyContent::CENTER);

    brls::Label* title = new brls::Label();
    title->setText("music/login/title"_i18n);
    title->setFontSize(28);
    title->setMarginBottom(24);
    box->addView(title);

    this->qrView = new nxkit::QRLoginView();
    this->qrView->setOnRefresh([this] { this->requestQr(); });
    box->addView(this->qrView);

    this->setContentView(box);
    this->requestQr();
}

LoginPage::~LoginPage() {
    *this->alive = false;
    this->stopPolling();
}

void LoginPage::requestQr() {
    this->finished = false;
    this->stopPolling();
    this->qrView->setRefreshVisible(false);
    this->qrView->showLoading("music/login/requesting"_i18n);

    NeteaseClient::instance().qrKey(
        [this, alive = this->alive](const std::string& key,
                                    const std::string& error) {
            brls::sync([this, alive, key, error] {
                if (!*alive) return;
                if (!error.empty()) {
                    this->qrView->setStatus(error);
                    this->qrView->setRefreshVisible(true);
                    return;
                }
                this->qrKey = key;

                NeteaseClient::instance().qrCreate(
                    key, [this, alive](const std::string& url,
                                       const std::string& error2) {
                        brls::sync([this, alive, url, error2] {
                            if (!*alive) return;
                            if (!error2.empty()) {
                                this->qrView->setStatus(error2);
                                this->qrView->setRefreshVisible(true);
                                return;
                            }
                            this->qrView->setQrContent(url);
                            this->qrView->setStatus("music/login/scan"_i18n);
                            this->startPolling();
                        });
                    });
            });
        });
}

void LoginPage::startPolling() {
    this->stopPolling();
    this->pollTask = new QrPollTask(this);
    this->pollTask->start();
}

void LoginPage::stopPolling() {
    if (!this->pollTask) return;
    this->pollTask->stop();
    delete this->pollTask;
    this->pollTask = nullptr;
}

void LoginPage::checkQr() {
    if (this->finished || this->qrKey.empty()) return;

    NeteaseClient::instance().qrCheck(
        this->qrKey,
        [this, alive = this->alive](int code, const std::string& message,
                                    const std::string& cookie,
                                    const std::string& error) {
            brls::sync([this, alive, code, message, cookie, error] {
                if (!*alive) return;
                // Transient network errors are retried on the next tick.
                if (!error.empty()) return;

                switch (code) {
                    case 800:
                        this->stopPolling();
                        this->qrView->setStatus("music/login/expired"_i18n);
                        this->qrView->setRefreshVisible(true);
                        break;
                    case 801:
                        this->qrView->setStatus("music/login/waiting"_i18n);
                        break;
                    case 802:
                        this->qrView->setStatus("music/login/scanned"_i18n);
                        break;
                    case 803:
                        this->finished = true;
                        this->stopPolling();
                        this->qrKey.clear();
                        NeteaseClient::instance().setCookie(cookie);
                        brls::Application::notify(
                            "music/login/success"_i18n);
                        brls::Application::popActivity();
                        break;
                    default:
                        break;
                }
            });
        });
}

}  // namespace music
