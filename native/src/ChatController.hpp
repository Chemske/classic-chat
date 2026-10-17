#ifndef CHATCONTROLLER_HPP
#define CHATCONTROLLER_HPP
#include <QObject>
#include <QVariantList>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <bb/cascades/ArrayDataModel>

class ChatController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bb::cascades::ArrayDataModel* messages READ messages CONSTANT)
    Q_PROPERTY(bb::cascades::ArrayDataModel* chats READ chats CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(QString title READ title NOTIFY stateChanged)
    Q_PROPERTY(int messageCount READ messageCount NOTIFY stateChanged)
    Q_PROPERTY(QString serverUrl READ serverUrl NOTIFY settingsChanged)
    Q_PROPERTY(bool tokenConfigured READ tokenConfigured NOTIFY settingsChanged)
    Q_PROPERTY(bool saveOnSd READ saveOnSd NOTIFY settingsChanged)
public:
    explicit ChatController(QObject *parent = 0);
    bb::cascades::ArrayDataModel* messages() const { return m_messages; }
    bb::cascades::ArrayDataModel* chats() const { return m_chats; }
    bool busy() const { return m_reply != 0; }
    QString status() const { return m_status; }
    QString title() const { return m_title; }
    int messageCount() const { return m_history.size(); }
    QString serverUrl() const;
    bool tokenConfigured() const;
    bool saveOnSd() const;
    Q_INVOKABLE bool configure(const QString &url, const QString &token, bool onSd);
    Q_INVOKABLE bool newChat();
    Q_INVOKABLE void sendMessage(const QString &text);
    Q_INVOKABLE bool saveCurrent();
    Q_INVOKABLE bool exportText();
    Q_INVOKABLE bool loadChat(const QString &id);
    Q_INVOKABLE bool deleteCurrent();
    Q_INVOKABLE void refreshChats();
    Q_INVOKABLE void cancel();
signals:
    void stateChanged();
    void settingsChanged();
    void answerReceived();
private slots:
    void receiveChunk();
    void finished();
    void timedOut();
private:
    QString storageRoot();
    bool sdMounted() const;
    bool validId(const QString &id) const;
    bool writeAtomic(const QString &path, const QByteArray &bytes);
    void setStatus(const QString &text);
    void showMessages();
    bb::cascades::ArrayDataModel *m_messages, *m_chats;
    QSettings m_settings;
    QNetworkReply *m_reply;
    QTimer m_timer;
    QByteArray m_response;
    QVariantList m_history;
    QString m_id, m_title, m_status, m_pending;
    bool m_dirty, m_cancelled, m_timedOut, m_tooLarge;
};
#endif
