#ifndef RELAYREPLY_HPP
#define RELAYREPLY_HPP
#include <QNetworkReply>
#include <QThread>
#include <QAtomicInt>
#include <QMutex>

bool initializeRelayNetwork();

// A bounded, cancellable request on BB10's TLS 1.2 capable native libcurl.
class RelayWorker : public QThread {
    Q_OBJECT
public:
    RelayWorker(const QUrl &url, const QByteArray &body, const QByteArray &token, QObject *parent);
    QByteArray url, body, token, caPath, response;
    QMutex responseMutex;
    int receivedBytes;
signals:
    void dataAvailable();
public:
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
    void consume();
    void complete();
private:
    RelayWorker *worker;
    QByteArray buffer;
    qint64 offset;
};
#endif
