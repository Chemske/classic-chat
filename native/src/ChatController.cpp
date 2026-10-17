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
#include <QImageReader>
#include <QImage>
#include <bb/cascades/Image>
#include <QBuffer>
#include <QTextCodec>

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
bool validFiles(const QVariantList &files) {
    if (files.size() > 1) return false;
    for (int i = 0; i < files.size(); ++i) {
        const QVariantMap f = files[i].toMap();
        if (f.size() != 2 || f.value("name").type() != QVariant::String || f.value("content").type() != QVariant::String ||
            !QRegExp("[A-Za-z0-9][A-Za-z0-9_.-]{0,79}\\.(txt|md|csv|json)").exactMatch(f.value("name").toString()) ||
            f.value("name").toString().contains("..") || f.value("content").toString().isEmpty() ||
            f.value("content").toString().contains(QChar(0)) || f.value("content").toString().toUtf8().size() > 16 * 1024) return false;
    }
    return true;
}
const char * const sdRoot = "/accounts/1000/removable/sdcard";
}
ChatController::ChatController(QObject *parent) : QObject(parent),
    m_messages(new bb::cascades::ArrayDataModel(this)),
    m_chats(new bb::cascades::ArrayDataModel(this)),
    m_settings("ClassicChat", "ClassicChat"), m_reply(0),
    m_attachmentFiles(new bb::cascades::ArrayDataModel(this)), m_streamDone(false), m_receivedBytes(0), m_dirty(false), m_cancelled(false), m_timedOut(false), m_tooLarge(false) {
    m_timer.setSingleShot(true); m_timer.setInterval(100000);
    connect(&m_timer, SIGNAL(timeout()), this, SLOT(timedOut()));
    m_thumbnailCache.setMaxCost(32);
    m_scrollTimer.setSingleShot(true); m_scrollTimer.setInterval(140);
    connect(&m_scrollTimer, SIGNAL(timeout()), this, SIGNAL(scrollRequested()));
    m_renderTimer.setInterval(150); connect(&m_renderTimer, SIGNAL(timeout()), this, SLOT(renderStream()));
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
        item["messageIndex"] = i;
        const QVariantList files = item.value("files").toList();
        item["fileName"] = files.isEmpty() ? QString() : files.first().toMap().value("name").toString();
        m_messages->append(item);
    }
    if (busy()) {
        QVariantMap user = message("user", m_pending); user["displayText"] = escaped(m_pending); user["fileName"] = "";
        m_messages->append(user);
        QVariantMap assistant = message("assistant", m_streamText); assistant["displayText"] = escaped(m_streamText.isEmpty() ? QString("…") : m_streamText); assistant["fileName"] = "";
        m_messages->append(assistant); m_scrollTimer.start();
    }
    emit stateChanged();
}
void ChatController::renderStream() {
    if (!busy() || m_rendered == m_streamText || m_messages->size() < 2) return;
    m_rendered = m_streamText;
    QVariantMap item = message("assistant", m_streamText); item["displayText"] = escaped(m_streamText); item["fileName"] = "";
    m_messages->replace(m_messages->size() - 1, item); emit streamUpdated(); m_scrollTimer.start();
}
bool ChatController::newChat() {
    if (busy() || (m_dirty && !saveCurrent())) return false;
    m_history.clear(); m_id = uuid(); m_title = "Nieuwe chat"; m_dirty = false;
    clearAttachment(); showMessages(); setStatus("Klaar voor een nieuwe chat."); return true;
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
    QVariantList wireHistory;
    for (int i = 0; i < history.size(); ++i) {
        const QVariantMap m = history[i].toMap(); wireHistory.append(message(m.value("role").toString(), m.value("content").toString()));
    }
    QVariantMap payload; payload["messages"] = wireHistory; payload["stream"] = true;
    if (!m_attachment.isEmpty()) payload["attachment"] = m_attachment;
    const QByteArray bytes = encode(payload);
    if (bytes.size() > 1024 * 1024) { setStatus("Dit gesprek en de bijlage zijn samen te groot. Begin een nieuwe chat."); return; }
    if (bytes.isEmpty()) { setStatus("Het bericht kon niet worden omgezet."); return; }
    QNetworkRequest request(QUrl(serverUrl() + "/v1/chat"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json; charset=utf-8");
    // RelayReply verifies the HTTPS hostname and CA using native TLS 1.2.
    m_response.clear(); m_pending = text; m_cancelled = m_timedOut = m_tooLarge = false;
    m_streamDone = false; m_receivedBytes = 0; m_streamText.clear(); m_rendered.clear(); m_streamError.clear(); m_streamResult.clear();
    m_reply = new RelayReply(request, bytes, m_settings.value("deviceToken").toString().toUtf8(), this);
    connect(m_reply, SIGNAL(readyRead()), this, SLOT(receiveChunk()));
    connect(m_reply, SIGNAL(finished()), this, SLOT(finished()));
    m_timer.start(); m_renderTimer.start(); showMessages(); setStatus("Antwoord ophalen…");
}
void ChatController::receiveChunk() {
    if (!m_reply || m_tooLarge) return;
    const QByteArray bytes = m_reply->readAll(); m_receivedBytes += bytes.size(); m_response += bytes;
    if (m_receivedBytes > 256 * 1024) { m_tooLarge = true; m_reply->abort(); return; }
    int end;
    while ((end = m_response.indexOf('\n')) >= 0) {
        const QByteArray line = m_response.left(end); m_response.remove(0, end + 1);
        if (line.trimmed().isEmpty()) continue;
        const QVariantMap event = decode(line).toMap(); const QString type = event.value("type").toString();
        if (m_streamDone || type.isEmpty()) { m_streamError = "Ongeldige antwoordstream."; m_reply->abort(); return; }
        if (type == "delta") {
            if (event.value("text").type() != QVariant::String) { m_streamError = "Ongeldige antwoordtekst."; m_reply->abort(); return; }
            m_streamText += event.value("text").toString();
            if (m_streamText.length() > 16000) { m_tooLarge = true; m_reply->abort(); return; }
        } else if (type == "done") { m_streamResult = event; m_streamDone = true; }
        else if (type == "error") {
            m_streamError = event.value("error").toMap().value("message").toString().left(300);
            if (m_streamError.isEmpty()) m_streamError = "Het antwoord kon niet worden afgerond.";
        } else if (type == "status") setStatus(event.value("text").toString().left(100));
        else if (type != "start") { m_streamError = "Onbekend streambericht."; m_reply->abort(); return; }
    }
}
void ChatController::cancel() { if (m_reply) { m_cancelled = true; m_reply->abort(); } }
void ChatController::timedOut() { if (m_reply) { m_timedOut = true; m_reply->abort(); } }
void ChatController::finished() {
    if (!m_reply) return;
    receiveChunk(); m_timer.stop(); m_renderTimer.stop(); QNetworkReply *reply = m_reply; m_reply = 0;
    const int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    const QVariantMap errorResult = decode(m_response).toMap();
    const QString answer = m_streamResult.value("text").toString();
    const QVariantList files = m_streamResult.value("files").toList();
    int existingFiles = 0; for (int i = 0; i < m_history.size(); ++i) existingFiles += m_history[i].toMap().value("files").toList().size();
    QString failure;
    if (m_cancelled) failure = "Geannuleerd. Het gedeeltelijke antwoord is niet opgeslagen; je tekst en bijlage blijven staan.";
    else if (m_timedOut) failure = "Geen antwoord op tijd. Je tekst en bijlage blijven staan.";
    else if (m_tooLarge) failure = "Het antwoord is te groot.";
    else if (!m_streamError.isEmpty()) failure = m_streamError;
    else if (code >= 300 && code < 400) failure = "Gebruik het uiteindelijke HTTPS-adres, zonder doorverwijzing.";
    else if (code != 200 || reply->error() != QNetworkReply::NoError) {
        failure = errorResult.value("error").toMap().value("message").toString();
        if (failure.isEmpty()) failure = "Verbinding mislukt. Controleer netwerk, HTTPS en toestel-token.";
    } else if (!m_streamDone || !m_response.trimmed().isEmpty()) failure = "Het antwoord is onderbroken. Je bericht blijft staan.";
    else if (answer.trimmed().isEmpty() || answer.length() > 16000) failure = "Geen bruikbaar antwoord ontvangen.";
    else if ((m_streamResult.contains("files") && m_streamResult.value("files").type() != QVariant::List) || !validFiles(files)) failure = "Het aangeboden bestand is ongeldig of te groot.";
    else if (existingFiles + files.size() > 8) failure = "Deze chat bevat al acht bestanden. Begin een nieuwe chat.";
    reply->deleteLater(); m_response.clear();
    if (!failure.isEmpty()) { m_pending.clear(); showMessages(); setStatus(failure); return; }
    if (m_history.isEmpty()) m_title = m_pending.left(48);
    QString userText = m_pending;
    if (!m_attachment.isEmpty()) userText += "\n[Bijlage: " + attachmentName() + "; alleen bij dit bericht meegestuurd]";
    // Names are metadata; original file bytes are deliberately not copied into chat history.
    if (userText.length() > 16000) userText = m_pending;
    m_history.append(message("user", userText));
    QVariantMap assistant = message("assistant", answer); if (!files.isEmpty()) assistant["files"] = files;
    m_history.append(assistant); m_pending.clear(); m_dirty = true; clearAttachment(); showMessages(); emit answerReceived(); m_scrollTimer.start();
    saveCurrent();
}
QString ChatController::attachmentFolder() const {
    const QString rootName = m_browserRoot == QString::fromLatin1(sdRoot) ? "SD-kaart" : "Toestel";
    const QString suffix = m_browserPath.mid(m_browserRoot.size()); return rootName + suffix;
}
bool ChatController::openAttachmentBrowser(bool onSd) {
    if (busy()) return false;
    if (onSd && !sdMounted()) { setStatus("De SD-kaart is niet beschikbaar."); return false; }
    m_browserRoot = onSd ? QString::fromLatin1(sdRoot) : "/accounts/1000/shared";
    m_browserPath = m_browserRoot; refreshAttachmentFolder(); return true;
}
void ChatController::refreshAttachmentFolder() {
    m_attachmentFiles->clear(); m_thumbnailCache.clear();
    const QFileInfoList entries = QDir(m_browserPath).entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks, QDir::DirsFirst | QDir::Name);
    for (int i = 0; i < entries.size() && m_attachmentFiles->size() < 500; ++i) {
        const QFileInfo info = entries[i]; const QString suffix = info.suffix().toLower();
        if (!info.isDir() && suffix != "txt" && suffix != "md" && suffix != "csv" && suffix != "json" && suffix != "jpg" && suffix != "jpeg" && suffix != "png") continue;
        QVariantMap row; row["name"] = info.fileName();
        row["directory"] = info.isDir(); row["path"] = info.absoluteFilePath(); m_attachmentFiles->append(row);
    }
    setStatus(m_attachmentFiles->isEmpty() ? "Geen ondersteunde bestanden in deze map." : "Kies een bestand of foto.");
    emit folderChanged();
}
QVariant ChatController::attachmentThumbnail(const QString &source) {
    const QFileInfo info(source); const QString path = info.canonicalFilePath();
    const QVariant fallback = QVariant::fromValue(bb::cascades::Image(QUrl("asset:///icons/document.png")));
    if (!path.startsWith(m_browserRoot + "/")) return fallback;
    if (const QVariant *cached = m_thumbnailCache.object(path)) return *cached;
    QVariant result = fallback;
    if (info.isDir()) result = QVariant::fromValue(bb::cascades::Image(QUrl("asset:///icons/folder.png")));
    else {
        const QString suffix = info.suffix().toLower();
        if (suffix == "jpg" || suffix == "jpeg" || suffix == "png") {
            result = QVariant::fromValue(bb::cascades::Image(QUrl("asset:///icons/photo.png")));
            if (info.size() <= 12 * 1024 * 1024) {
                QImageReader reader(path); const QSize size = reader.size();
                if (size.isValid() && qint64(size.width()) * size.height() <= 16000000) {
                    QSize small = size; if (small.width() > 160 || small.height() > 160) small.scale(QSize(160, 160), Qt::KeepAspectRatio);
                    reader.setScaledSize(small); const QImage image = reader.read();
                    QByteArray bytes; QBuffer buffer(&bytes); buffer.open(QIODevice::WriteOnly);
                    if (!image.isNull() && image.save(&buffer, "PNG") && bytes.size() <= 128 * 1024)
                        result = QVariant::fromValue(bb::cascades::Image(bytes));
                }
            }
        }
    }
    m_thumbnailCache.insert(path, new QVariant(result)); return result;
}
void ChatController::parentAttachmentFolder() {
    if (busy() || m_browserPath == m_browserRoot) return;
    const QString parent = QFileInfo(m_browserPath).absolutePath();
    if (parent == m_browserRoot || parent.startsWith(m_browserRoot + "/")) { m_browserPath = parent; refreshAttachmentFolder(); }
}
bool ChatController::pickAttachment(int index) {
    if (busy() || index < 0 || index >= m_attachmentFiles->size()) return false;
    const QVariantMap row = m_attachmentFiles->value(index).toMap(); const QString path = QFileInfo(row.value("path").toString()).canonicalFilePath();
    if (!path.startsWith(m_browserRoot + "/")) { setStatus("Ongeldige bestandslocatie."); return false; }
    if (row.value("directory").toBool()) { m_browserPath = path; refreshAttachmentFolder(); return false; }
    const QVariantMap previous = m_attachment; m_attachment.clear(); fileSelected(QStringList() << path);
    if (m_attachment.isEmpty()) { m_attachment = previous; emit attachmentChanged(); return false; }
    return true;
}
void ChatController::clearAttachment() { if (busy()) return; m_attachment.clear(); emit attachmentChanged(); }
void ChatController::fileSelected(const QStringList &paths) {
    if (busy() || paths.size() != 1) return;
    const QFileInfo info(paths.first()); const QString path = info.canonicalFilePath();
    if ((!path.startsWith("/accounts/1000/shared/") && !path.startsWith(QString::fromLatin1(sdRoot) + "/")) || !info.isFile() || info.size() <= 0) {
        setStatus("Kies een gewoon bestand uit gedeelde opslag of SD."); return;
    }
    const QString suffix = info.suffix().toLower(); QVariantMap attachment;
    QString name = info.fileName().left(100);
    for (int i = 0; i < name.size(); ++i) if (name[i].unicode() < 32 || name[i] == '/' || name[i] == '\\') name[i] = '_';
    attachment["name"] = name;
    if (suffix == "txt" || suffix == "md" || suffix == "csv" || suffix == "json") {
        if (info.size() > 12 * 1024) { setStatus("Tekstbijlagen mogen maximaal 12 KiB zijn."); return; }
        QFile file(path); if (!file.open(QIODevice::ReadOnly)) { setStatus("Bestand niet leesbaar."); return; }
        const QByteArray bytes = file.read(12 * 1024 + 1); QTextCodec::ConverterState state;
        QString text = QTextCodec::codecForName("UTF-8")->toUnicode(bytes.constData(), bytes.size(), &state);
        if (state.invalidChars || state.remainingChars || text.contains(QChar(0)) || text.trimmed().isEmpty() || bytes.size() > 12 * 1024) {
            setStatus("Gebruik een leesbaar UTF-8-tekstbestand."); return;
        }
        attachment["type"] = "text/plain"; attachment["text"] = text;
    } else if (suffix == "jpg" || suffix == "jpeg" || suffix == "png") {
        if (info.size() > 12 * 1024 * 1024) { setStatus("De originele foto is te groot (maximaal 12 MiB)."); return; }
        QImageReader reader(path); const QSize size = reader.size();
        if (!size.isValid() || qint64(size.width()) * size.height() > 16000000) { setStatus("Deze foto is te groot of niet leesbaar."); return; }
        QSize scaled = size; if (scaled.width() > 1280 || scaled.height() > 1280) scaled.scale(QSize(1280, 1280), Qt::KeepAspectRatio);
        reader.setScaledSize(scaled);
        const QImage image = reader.read(); if (image.isNull()) { setStatus("De foto kon niet worden gelezen."); return; }
        QByteArray jpeg; QBuffer buffer(&jpeg); buffer.open(QIODevice::WriteOnly);
        if (!image.save(&buffer, "JPEG", 75) || jpeg.size() > 512 * 1024) { setStatus("De verkleinde foto is nog te groot."); return; }
        attachment["name"] = "foto.jpg"; attachment["type"] = "image/jpeg"; attachment["data"] = QString::fromLatin1(jpeg.toBase64());
    } else { setStatus("Kies txt, md, csv, json, jpg of png."); return; }
    m_attachment = attachment; emit attachmentChanged(); setStatus("Bijlage klaar; wordt alleen met je volgende bericht meegestuurd.");
}
bool ChatController::downloadFile(int messageIndex) {
    if (busy() || messageIndex < 0 || messageIndex >= m_history.size()) return false;
    const QVariantList files = m_history[messageIndex].toMap().value("files").toList();
    if (files.isEmpty() || !validFiles(files)) { setStatus("Geen geldig bestand beschikbaar."); return false; }
    if (!sdMounted()) { setStatus("Plaats een SD-kaart om dit bestand op te slaan."); return false; }
    const QVariantMap file = files.first().toMap();
    const QString name = m_id + "-" + QString::number(messageIndex) + "-" + file.value("name").toString();
    const QString path = QString::fromLatin1(sdRoot) + "/ClassicChat/downloads/" + name;
    if (QFileInfo(path).isSymLink()) { setStatus("Ongeldig opslagdoel."); return false; }
    const QByteArray bytes = file.value("content").toString().toUtf8();
    if (QFile::exists(path)) {
        QFile existing(path);
        if (!existing.open(QIODevice::ReadOnly) || existing.size() != bytes.size() || existing.read(16 * 1024 + 1) != bytes) {
            setStatus("Deze bestandsnaam bestaat al. Er is niets overschreven."); return false;
        }
        setStatus("Dit bestand staat al in ClassicChat/downloads op SD."); return true;
    }
    const QString folder = QFileInfo(path).absolutePath();
    if (!QDir().mkpath(folder) || QFileInfo(folder).canonicalFilePath() != folder) { setStatus("Ongeldige downloadmap op SD."); return false; }
    if (!writeAtomic(path, bytes)) return false;
    setStatus("Bestand opgeslagen in ClassicChat/downloads op SD."); return true;
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
    int fileCount = 0;
    for (int i = 0; i < history.size(); ++i) {
        const QVariantMap m = history[i].toMap();
        const QVariantList files = m.value("files").toList(); fileCount += files.size();
        if (!validFiles(files) || fileCount > 8 || (i % 2 == 0 && !files.isEmpty())) { setStatus("Ongeldige bestanden in chat."); return false; }
        if (m.value("role").toString() != (i % 2 ? "assistant" : "user") ||
            m.value("content").type() != QVariant::String || m.value("content").toString().isEmpty() ||
            m.value("content").toString().length() > 16000) { setStatus("Ongeldige berichten in chatbestand."); return false; }
    }
    m_id = id; m_title = record.value("title").toString().left(80); m_history = history; m_dirty = false;
    clearAttachment(); showMessages(); setStatus("Chat geopend."); return true;
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
