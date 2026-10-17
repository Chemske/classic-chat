#include "ChatController.hpp"
#include "RelayReply.hpp"
#include <bb/data/JsonDataAccess>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryFile>
#include <QDateTime>
#include <QNetworkRequest>
#include <QUuid>
#include <QRegExp>
#include <QUrl>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

namespace {
QByteArray encode(const QVariant &value) {
    bb::data::JsonDataAccess json; QByteArray bytes;
    json.saveToBuffer(value, &bytes);
    return json.hasError() ? QByteArray() : bytes;
}
QVariant decode(const QByteArray &bytes) {
    bb::data::JsonDataAccess json; QVariant value = json.loadFromBuffer(bytes);
    return json.hasError() ? QVariant() : value;
}
QVariantMap message(const QString &role, const QString &text) {
    QVariantMap m; m["role"] = role; m["content"] = text; return m;
}
QString uuid() { QString id = QUuid::createUuid().toString(); return id.mid(1, 36); }
QString escaped(QString text) {
    return text.replace('&', "&amp;").replace('<', "&lt;").replace('>', "&gt;").replace('"', "&quot;").replace('\n', "<br/>");
}
QString displayText(const QString &text) {
    // Render only citation links; all model/user HTML stays escaped text.
    QRegExp link("\\[([0-9]{1,2})\\]\\((https?://[^\\s<>\"\\)]+)\\)");
    QString html; int position = 0, found;
    while ((found = link.indexIn(text, position)) >= 0) {
        html += escaped(text.mid(position, found - position));
        const QUrl url(link.cap(2));
        if (url.isValid() && !url.host().isEmpty() && url.userInfo().isEmpty())
            html += "<a href=\"" + escaped(url.toString()) + "\">[" + link.cap(1) + "]</a>";
        else html += escaped(link.cap(0));
        position = found + link.matchedLength();
    }
    return "<html><body>" + html + escaped(text.mid(position)) + "</body></html>";
}
const char * const sdRoot = "/accounts/1000/removable/sdcard";
}
ChatController::ChatController(QObject *parent) : QObject(parent),
    m_messages(new bb::cascades::ArrayDataModel(this)),
    m_chats(new bb::cascades::ArrayDataModel(this)),
    m_settings("ClassicChat", "ClassicChat"), m_reply(0),
    m_dirty(false), m_cancelled(false), m_timedOut(false), m_tooLarge(false) {
    m_timer.setSingleShot(true); m_timer.setInterval(100000);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(timedOut()));
    m_id = uuid(); m_title = "Nieuwe chat";
    refreshChats();
    // Optional, private one-time setup placed by the device installer.
    // The public BAR never contains a server token or an OpenAI key.
    const QString setupPath = QDir::homePath() + "/classicchat-setup.json";
    const QFileInfo setupInfo(setupPath);
    if (setupInfo.isFile() && !setupInfo.isSymLink() && setupInfo.size() > 0 && setupInfo.size() <= 4096) {
        QFile setupFile(setupPath);
        if (setupFile.open(QIODevice::ReadOnly)) {
            const QVariantMap setup = decode(setupFile.readAll()).toMap();
            setupFile.close();
            if (setup.value("serverUrl").type() == QVariant::String &&
                setup.value("deviceToken").type() == QVariant::String &&
                setup.value("saveOnSd").type() == QVariant::Bool &&
                configure(setup.value("serverUrl").toString(), setup.value("deviceToken").toString(), setup.value("saveOnSd").toBool())) {
                setupFile.remove();
            }
        }
    }
}
QString ChatController::serverUrl() const { return m_settings.value("serverUrl").toString(); }
bool ChatController::tokenConfigured() const { return !m_settings.value("deviceToken").toString().isEmpty(); }
bool ChatController::saveOnSd() const { return m_settings.value("saveOnSd", true).toBool(); }
void ChatController::setStatus(const QString &text) { m_status = text; emit stateChanged(); }
bool ChatController::configure(const QString &urlText, const QString &token, bool onSd) {
    if (busy()) return false;
    const QUrl url(urlText.trimmed());
    if (!url.isValid() || url.scheme() != "https" || url.host().isEmpty() ||
        !url.userInfo().isEmpty() || url.hasQuery() || url.hasFragment() ||
        (!url.path().isEmpty() && url.path() != "/")) {
        setStatus("Gebruik een HTTPS-serveradres zonder pad of inloggegevens."); return false;
    }
    if ((!token.isEmpty() && !QRegExp("[A-Za-z0-9_-]{32,256}").exactMatch(token)) || (token.isEmpty() && !tokenConfigured())) {
        setStatus("Voer een toestel-token van minimaal 32 tekens in."); return false;
    }
    // Do not silently replace the current chat's location with a different storage target.
    if (m_dirty && !saveCurrent()) return false;
    QString normal = url.toString(); if (normal.endsWith('/')) normal.chop(1);
    m_settings.setValue("serverUrl", normal);
    if (!token.isEmpty()) m_settings.setValue("deviceToken", token);
    m_settings.setValue("saveOnSd", onSd); m_settings.sync();
    if (m_settings.status() != QSettings::NoError) { setStatus("Instellingen konden niet worden opgeslagen."); return false; }
    emit settingsChanged(); refreshChats(); setStatus("Instellingen opgeslagen."); return true;
}
bool ChatController::sdMounted() const {
    // Avoid creating chat folders on internal storage beneath an absent card mount point.
    struct stat card, home;
    const QByteArray cardPath = QFile::encodeName(sdRoot);
    const QByteArray homePath = QFile::encodeName(QDir::homePath());
    return ::stat(cardPath.constData(), &card) == 0 && S_ISDIR(card.st_mode) &&
        ::stat(homePath.constData(), &home) == 0 && card.st_dev != home.st_dev;
}
QString ChatController::storageRoot() {
    if (saveOnSd() && !sdMounted()) { setStatus("SD-kaart ontbreekt. Deze chat blijft alleen in het geheugen."); return QString(); }
    return saveOnSd() ? QString::fromLatin1(sdRoot) + "/ClassicChat" : QDir::homePath() + "/ClassicChat";
}
bool ChatController::validId(const QString &id) const {
    return QRegExp("[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}").exactMatch(id);
}
bool ChatController::writeAtomic(const QString &path, const QByteArray &bytes) {
    if (bytes.isEmpty() || bytes.size() > 1024 * 1024) { setStatus("De chat is te groot of kon niet worden omgezet."); return false; }
    const QString folder = QFileInfo(path).absolutePath();
    if (!QDir().mkpath(folder)) { setStatus("De opslagmap is niet beschikbaar."); return false; }
    // Atomic POSIX rename replaces only our own UUID-named chat file, after successful flush.
    QTemporaryFile tmp(folder + "/.classicchat-XXXXXX");
    if (!tmp.open() || tmp.write(bytes) != bytes.size() || !tmp.flush() || ::fsync(tmp.handle()) != 0) {
        setStatus("Opslaan mislukt. Controleer de SD-kaart en vrije ruimte."); return false;
    }
    tmp.close();
    const QByteArray from = QFile::encodeName(tmp.fileName()), to = QFile::encodeName(path);
    if (::rename(from.constData(), to.constData()) != 0) { setStatus("Het chatbestand kon niet worden opgeslagen."); return false; }
    tmp.setAutoRemove(false); return true;
}
bool ChatController::saveCurrent() {
    if (m_history.isEmpty()) return true;
    const QString root = storageRoot(); if (root.isEmpty()) return false;
    QVariantMap record; record["schema"] = 1; record["id"] = m_id; record["title"] = m_title;
    record["updated"] = QDateTime::currentDateTimeUtc().toString(Qt::ISODate); record["messages"] = m_history;
    if (!writeAtomic(root + "/chats/" + m_id + ".json", encode(record))) return false;
    m_dirty = false; refreshChats(); setStatus("Opgeslagen."); return true;
}
void ChatController::showMessages() {
    m_messages->clear();
    for (int i = 0; i < m_history.size(); ++i) {
        QVariantMap item = m_history.at(i).toMap();
        item["displayText"] = item.value("role").toString() == "assistant" ? displayText(item.value("content").toString()) : escaped(item.value("content").toString());
        m_messages->append(item);
    }
    emit stateChanged();
}
bool ChatController::newChat() {
    if (busy() || (m_dirty && !saveCurrent())) return false;
    m_history.clear(); m_id = uuid(); m_title = "Nieuwe chat"; m_dirty = false;
    showMessages(); setStatus("Klaar voor een nieuwe chat."); return true;
}
void ChatController::sendMessage(const QString &input) {
    if (busy()) return;
    const QString text = input.trimmed();
    if (text.isEmpty() || text.length() > 16000) { setStatus("Gebruik 1 tot 16000 tekens."); return; }
    if (serverUrl().isEmpty() || !tokenConfigured()) { setStatus("Stel eerst je server en toestel-token in."); return; }
    // Never silently truncate old turns: the user explicitly starts another chat at this bound.
    if (m_history.size() >= 40) { setStatus("Deze chat is lang. Sla hem op en begin een nieuwe chat."); return; }
    QVariantList history = m_history; history.append(message("user", text));
    int total = 0;
    for (int i = 0; i < history.size(); ++i) total += history.at(i).toMap().value("content").toString().length();
    if (total > 64000) { setStatus("Deze chat is te lang voor een aanvraag. Begin een nieuwe chat."); return; }
    QVariantMap payload; payload["messages"] = history; const QByteArray bytes = encode(payload);
    if (bytes.isEmpty()) { setStatus("Het bericht kon niet worden omgezet."); return; }
    QNetworkRequest request(QUrl(serverUrl() + "/v1/chat"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    // RelayReply verifies the HTTPS hostname and CA using native TLS 1.2.
    m_response.clear(); m_pending = text; m_cancelled = m_timedOut = m_tooLarge = false;
    m_reply = new RelayReply(request, bytes, m_settings.value("deviceToken").toString().toUtf8(), this);
    connect(m_reply, SIGNAL(readyRead()), this, SLOT(receiveChunk()));
    connect(m_reply, SIGNAL(finished()), this, SLOT(finished()));
    m_timer.start(); setStatus("Antwoord ophalen…");
}
void ChatController::receiveChunk() {
    if (!m_reply || m_tooLarge) return;
    m_response += m_reply->readAll();
    if (m_response.size() > 256 * 1024) { m_tooLarge = true; m_reply->abort(); }
}
void ChatController::cancel() { if (m_reply) { m_cancelled = true; m_reply->abort(); } }
void ChatController::timedOut() { if (m_reply) { m_timedOut = true; m_reply->abort(); } }
void ChatController::finished() {
    if (!m_reply) return;
    receiveChunk(); m_timer.stop(); QNetworkReply *reply = m_reply; m_reply = 0;
    const int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QVariantMap result = decode(m_response).toMap();
    const QString answer = result.value("text").toString();
    QString failure;
    if (m_cancelled) failure = "Aanvraag geannuleerd. Je tekst blijft staan.";
    else if (m_timedOut) failure = "Geen antwoord op tijd. Je tekst blijft staan.";
    else if (m_tooLarge) failure = "Het antwoord is te groot.";
    else if (code >= 300 && code < 400) failure = "Gebruik het uiteindelijke HTTPS-adres, zonder doorverwijzing.";
    else if (code != 200 || reply->error() != QNetworkReply::NoError) {
        failure = result.value("error").toMap().value("message").toString();
        if (failure.isEmpty()) failure = "Verbinding mislukt. Controleer netwerk, HTTPS en toestel-token.";
    } else if (answer.trimmed().isEmpty() || answer.length() > 16000) failure = "Geen bruikbaar antwoord ontvangen.";
    reply->deleteLater(); m_response.clear();
    if (!failure.isEmpty()) { m_pending.clear(); setStatus(failure); return; }
    if (m_history.isEmpty()) m_title = m_pending.left(48);
    m_history.append(message("user", m_pending)); m_history.append(message("assistant", answer));
    m_pending.clear(); m_dirty = true; showMessages(); emit answerReceived();
    // A missing/full SD card leaves the successful response in RAM, with an explicit warning.
    saveCurrent();
}
void ChatController::refreshChats() {
    m_chats->clear(); const QString root = storageRoot(); if (root.isEmpty()) return;
    QDir dir(root + "/chats");
    const QFileInfoList files = dir.entryInfoList(QStringList() << "*.json", QDir::Files | QDir::NoSymLinks, QDir::Time);
    // Bound index memory and IO even if the user copies many files into the card directory.
    for (int i = 0; i < files.size() && i < 200; ++i) {
        if (!validId(files[i].completeBaseName()) || files[i].size() > 1024 * 1024) continue;
        QFile file(files[i].absoluteFilePath()); if (!file.open(QIODevice::ReadOnly)) continue;
        const QVariantMap record = decode(file.read(1024 * 1024 + 1)).toMap();
        if (record.value("schema").toInt() != 1 || record.value("id").toString() != files[i].completeBaseName()) continue;
        QVariantMap summary; summary["id"] = record.value("id"); summary["title"] = record.value("title").toString().left(80);
        summary["updated"] = record.value("updated"); m_chats->append(summary);
    }
}
bool ChatController::loadChat(const QString &id) {
    if (busy() || !validId(id) || (m_dirty && !saveCurrent())) return false;
    const QString root = storageRoot(); if (root.isEmpty()) return false;
    const QString path = root + "/chats/" + id + ".json";
    if (QFileInfo(path).isSymLink()) { setStatus("Ongeldig chatbestand."); return false; }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly) || file.size() > 1024 * 1024) { setStatus("Chat kon niet worden geopend."); return false; }
    const QVariantMap record = decode(file.read(1024 * 1024 + 1)).toMap(); const QVariantList history = record.value("messages").toList();
    if (record.value("schema").toInt() != 1 || record.value("id").toString() != id || history.size() > 40 || history.size() % 2) {
        setStatus("Ongeldig of te groot chatbestand."); return false;
    }
    for (int i = 0; i < history.size(); ++i) {
        const QVariantMap m = history[i].toMap();
        if (m.value("role").toString() != (i % 2 ? "assistant" : "user") ||
            m.value("content").type() != QVariant::String || m.value("content").toString().isEmpty() ||
            m.value("content").toString().length() > 16000) { setStatus("Ongeldige berichten in chatbestand."); return false; }
    }
    m_id = id; m_title = record.value("title").toString().left(80); m_history = history; m_dirty = false;
    showMessages(); setStatus("Chat geopend."); return true;
}
bool ChatController::exportText() {
    if (busy() || m_history.isEmpty()) return false;
    const QString root = storageRoot(); if (root.isEmpty()) return false;
    QString text = m_title + "\n\n";
    for (int i = 0; i < m_history.size(); ++i) {
        const QVariantMap m = m_history[i].toMap();
        text += (m.value("role").toString() == "user" ? "Jij" : "Assistent") + QString(":\n") + m.value("content").toString() + "\n\n";
    }
    if (!writeAtomic(root + "/exports/" + m_id + ".txt", text.toUtf8())) return false;
    setStatus("Tekstexport opgeslagen in ClassicChat/exports."); return true;
}
bool ChatController::deleteCurrent() {
    if (busy() || !validId(m_id)) return false;
    const QString root = storageRoot(); if (root.isEmpty()) return false;
    const QString path = root + "/chats/" + m_id + ".json";
    if (QFileInfo(path).isSymLink() || (QFile::exists(path) && !QFile::remove(path))) { setStatus("Chat kon niet worden verwijderd."); return false; }
    // This intentionally affects this app's current chat only; exported TXT copies remain separate.
    m_dirty = false; m_history.clear(); m_id = uuid(); m_title = "Nieuwe chat";
    showMessages(); refreshChats(); setStatus("Chat verwijderd. Eventuele tekstexports blijven bewaard."); return true;
}
