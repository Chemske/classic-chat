#ifndef CHATCONTROLLER_HPP
#define CHATCONTROLLER_HPP
#include <QObject>
#include <QVariantList>
#include <QSettings>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QTimer>
#include <QStringList>
#include <QCache>
#include <bb/cascades/ArrayDataModel>

class ChatController : public QObject {
    Q_OBJECT
    Q_PROPERTY(bb::cascades::ArrayDataModel* messages READ messages CONSTANT)
    Q_PROPERTY(bb::cascades::ArrayDataModel* chats READ chats CONSTANT)
    Q_PROPERTY(bool busy READ busy NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(bb::cascades::ArrayDataModel* attachmentFiles READ attachmentFiles CONSTANT)
    Q_PROPERTY(QString attachmentFolder READ attachmentFolder NOTIFY folderChanged)
    Q_PROPERTY(QString attachmentName READ attachmentName NOTIFY attachmentChanged)
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
    int messageCount() const { return m_history.size() + (busy() ? 2 : 0); }
    QString attachmentName() const { return m_attachment.value("name").toString(); }
    bb::cascades::ArrayDataModel* attachmentFiles() const { return m_attachmentFiles; }
    QString attachmentFolder() const;
    Q_INVOKABLE bool openAttachmentBrowser(bool onSd);
    Q_INVOKABLE bool pickAttachment(int index);
    Q_INVOKABLE QVariant attachmentThumbnail(const QString &path);
    Q_INVOKABLE void parentAttachmentFolder();
    Q_INVOKABLE void clearAttachment();
    Q_INVOKABLE bool downloadFile(int messageIndex);
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
    void attachmentChanged();
    void streamUpdated();
    void scrollRequested();
    void folderChanged();
private slots:
    void receiveChunk();
    void finished();
    void timedOut();
    void fileSelected(const QStringList &paths);
    void renderStream();
private:
    QString storageRoot();
    bool sdMounted() const;
    bool validId(const QString &id) const;
    bool writeAtomic(const QString &path, const QByteArray &bytes);
    void setStatus(const QString &text);
    void showMessages();
    void refreshAttachmentFolder();
    bb::cascades::ArrayDataModel *m_messages, *m_chats;
    QSettings m_settings;
    QNetworkReply *m_reply;
    QTimer m_timer, m_renderTimer, m_scrollTimer;
    bb::cascades::ArrayDataModel *m_attachmentFiles;
    QString m_browserRoot, m_browserPath;
    QCache<QString, QVariant> m_thumbnailCache;
    QVariantMap m_attachment, m_streamResult;
    QString m_streamText, m_rendered, m_streamError;
    bool m_streamDone;
    int m_receivedBytes;
    QByteArray m_response;
    QVariantList m_history;
    QString m_id, m_title, m_status, m_pending;
    bool m_dirty, m_cancelled, m_timedOut, m_tooLarge;
};
#endif
