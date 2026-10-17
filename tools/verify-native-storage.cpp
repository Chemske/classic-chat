// Use only with an explicitly prepared, disposable ClassicChat test conversation UUID.
// No network request, token, or private chat contents are printed.
#include "ChatController.hpp"
#include "RelayReply.hpp"
#include <QCoreApplication>
#include <stdio.h>
int main(int argc, char **argv) {
    if (argc != 2 || !initializeRelayNetwork()) return 2;
    QCoreApplication app(argc, argv); ChatController chat;
    const bool browser = chat.openAttachmentBrowser(true);
    const int items = chat.attachmentFiles()->size();
    const bool loaded = chat.loadChat(QString::fromLatin1(argv[1]));
    const bool saved = loaded && chat.downloadFile(1);
    fprintf(stdout, "sd_browser=%d entries=%d fixture_loaded=%d download_saved=%d\n", browser, items, loaded, saved);
    if (!saved) fprintf(stdout, "status=%s\n", chat.status().toUtf8().constData());
    return browser && loaded && saved ? 0 : 1;
}
