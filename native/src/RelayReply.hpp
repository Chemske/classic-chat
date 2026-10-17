#ifndef RELAYREPLY_HPP
#define RELAYREPLY_HPP
#include <QNetworkReply>
#include <QThread>
#include <QAtomicInt>

bool initializeRelayNetwork();

// A bounded, cancellable request on BB10's TLS 1.2 capable native libcurl.
class RelayWorker : public QThread {
public:
    RelayWorker(const QUrl &url, const QByteArray &body, const QByteArray &token, QObject *parent);
    QByteArray url, body, token, caPath, response;
    QAtomicInt cancelled;
    long status;
    bool failed, tooLarge;
protected:
    void run();
private:
    static size_t receive(char *, size_t, size_t, void *);
    static int progress(void *, double, double, double, double);
};
class RelayReply : public QNetworkReply {
    Q_OBJECT
public:
    RelayReply(const QNetworkRequest &request, const QByteArray &body, const QByteArray &token, QObject *parent);
    ~RelayReply();
    void abort();
    qint64 bytesAvailable() const;
protected:
    qint64 readData(char *data, qint64 maxSize);
private slots:
    void complete();
private:
    RelayWorker *worker;
    QByteArray buffer;
    qint64 offset;
};
#endif
