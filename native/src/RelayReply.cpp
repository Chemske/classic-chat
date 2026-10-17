#include "RelayReply.hpp"
#include <QDir>
#include <QFile>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <curl/curl.h>
#include <string.h>
#include <limits.h>
#include <QMutex>
#include <openssl/crypto.h>

namespace {
QMutex *sslLocks = 0;
void lockSsl(int mode, int index, const char *, int) {
    if (mode & CRYPTO_LOCK) sslLocks[index].lock(); else sslLocks[index].unlock();
}
unsigned long sslThreadId() { return reinterpret_cast<unsigned long>(QThread::currentThreadId()); }
}
bool initializeRelayNetwork() {
    // Install callbacks required by the system OpenSSL 1.0.2 before workers start.
    if (!CRYPTO_get_locking_callback()) {
        sslLocks = new QMutex[CRYPTO_num_locks()];
        CRYPTO_set_id_callback(sslThreadId); CRYPTO_set_locking_callback(lockSsl);
    }
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) return false;
    return curl_version_info(CURLVERSION_NOW)->version_num >= 0x072200;
}

RelayWorker::RelayWorker(const QUrl &address, const QByteArray &bytes, const QByteArray &secret, QObject *parent)
    : QThread(parent), url(address.toEncoded()), body(bytes), token(secret),
      caPath(QFile::encodeName(QDir::currentPath() + "/app/native/assets/certificates/isrgrootx1.pem")),
      cancelled(0), status(0), failed(true), tooLarge(false) {}
size_t RelayWorker::receive(char *data, size_t size, size_t count, void *context) {
    RelayWorker *w = static_cast<RelayWorker *>(context);
    if (w->cancelled.fetchAndAddOrdered(0) || (size && count > size_t(-1) / size)) return 0;
    const size_t bytes = size * count;
    if (bytes > size_t(256 * 1024 - w->response.size())) { w->tooLarge = true; return 0; }
    w->response.append(data, int(bytes)); return bytes;
}
int RelayWorker::progress(void *context, double, double, double, double) {
    return static_cast<RelayWorker *>(context)->cancelled.fetchAndAddOrdered(0) ? 1 : 0;
}
void RelayWorker::run() {
    CURL *curl = curl_easy_init(); if (!curl) return;
    struct curl_slist *headers = 0;
    headers = curl_slist_append(headers, "Content-Type: application/json; charset=utf-8");
    headers = curl_slist_append(headers, ("Authorization: Bearer " + token).constData());
    headers = curl_slist_append(headers, "Expect:");
    curl_easy_setopt(curl, CURLOPT_URL, url.constData());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.constData());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, long(body.size()));
    curl_easy_setopt(curl, CURLOPT_CAINFO, caPath.constData());
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    // BB10 10.3.3 ships curl 7.42; the older SDK header lacks this enum name.
    const long tls12 = 6; // CURL_SSLVERSION_TLSv1_2, supported since curl 7.34.
    curl_easy_setopt(curl, CURLOPT_SSLVERSION, tls12);
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS, long(CURLPROTO_HTTPS));
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 0L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 100L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &RelayWorker::receive);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, &RelayWorker::progress);
    curl_easy_setopt(curl, CURLOPT_PROGRESSDATA, this);
    failed = curl_easy_perform(curl) != CURLE_OK;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);
    curl_slist_free_all(headers); curl_easy_cleanup(curl);
    token.fill(0); token.clear(); body.clear();
}
RelayReply::RelayReply(const QNetworkRequest &request, const QByteArray &body, const QByteArray &token, QObject *parent)
    : QNetworkReply(parent), worker(new RelayWorker(request.url(), body, token, this)), offset(0) {
    setRequest(request); setUrl(request.url()); setOperation(QNetworkAccessManager::PostOperation);
    open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    connect(worker, SIGNAL(finished()), this, SLOT(complete())); worker->start();
}
RelayReply::~RelayReply() { worker->cancelled.fetchAndStoreOrdered(1); worker->wait(); }
void RelayReply::abort() { worker->cancelled.fetchAndStoreOrdered(1); }
qint64 RelayReply::bytesAvailable() const { return buffer.size() - offset + QNetworkReply::bytesAvailable(); }
qint64 RelayReply::readData(char *data, qint64 maxSize) {
    const qint64 left = buffer.size() - offset; if (!left) return -1;
    const qint64 count = qMin(left, maxSize); memcpy(data, buffer.constData() + offset, size_t(count)); offset += count; return count;
}
void RelayReply::complete() {
    setAttribute(QNetworkRequest::HttpStatusCodeAttribute, int(worker->status));
    buffer = worker->response;
    if (worker->tooLarge) buffer = QByteArray(256 * 1024 + 1, ' ');
    if (worker->failed) setError(QNetworkReply::UnknownNetworkError, "HTTPS request failed");
    setFinished(true);
    if (!buffer.isEmpty()) emit readyRead();
    emit finished();
}
