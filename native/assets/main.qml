import bb.cascades 1.3
import bb.system 1.0

NavigationPane {
    id: navigation
    onPopTransitionEnded: page.destroy()
    onCreationCompleted: {
        _chat.answerReceived.connect(function() {
            composer.text = "";
        });
        _chat.scrollRequested.connect(function() {
            if (conversation.followAnswer) conversation.scrollToPosition(ScrollPosition.End, ScrollAnimation.Smooth);
        });
        _chat.stateChanged.connect(function() {
            if (_chat.busy) spinner.start(); else spinner.stop();
        });
    }
    Page {
        Container {
            horizontalAlignment: HorizontalAlignment.Fill
            verticalAlignment: VerticalAlignment.Fill
            background: Color.create("#101418")
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                leftPadding: ui.du(2)
                rightPadding: ui.du(1)
                topPadding: ui.du(0.5)
                bottomPadding: ui.du(0.5)
                background: Color.create("#171D23")
                Label {
                    text: "Classic Chat"
                    verticalAlignment: VerticalAlignment.Center
                    layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                    textStyle.base: SystemDefaults.TextStyles.BodyText
                    textStyle.fontWeight: FontWeight.Bold
                    textStyle.color: Color.create("#F1F4F6")
                }
                Button {
                    text: "↓"
                    visible: !conversation.followAnswer && _chat.messageCount > 0
                    preferredWidth: ui.du(7)
                    preferredHeight: ui.du(6)
                    onClicked: { conversation.followAnswer = true; conversation.scrollToPosition(ScrollPosition.End, ScrollAnimation.Smooth); }
                }
                Button {
                    text: "Menu"
                    preferredWidth: ui.du(11)
                    preferredHeight: ui.du(6)
                    enabled: !_chat.busy
                    onClicked: menuDialog.open()
                }
            }
            Container {
                visible: _chat.messageCount === 0
                layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                verticalAlignment: VerticalAlignment.Center
                leftPadding: ui.du(4)
                rightPadding: ui.du(4)
                topPadding: ui.du(5)
                Label {
                    text: "Waar denk je aan?"
                    textStyle.base: SystemDefaults.TextStyles.TitleText
                    textStyle.color: Color.create("#F1F4F6")
                }
                Label {
                    text: "Stel een vraag, werk een idee uit of zoek iets actueels op."
                    multiline: true
                    textStyle.color: Color.create("#9AA9B5")
                }
                Label {
                    topMargin: ui.du(3)
                    text: "Probeer: Hoe wordt het weer vandaag in Brussel?"
                    multiline: true
                    textStyle.base: SystemDefaults.TextStyles.SmallText
                    textStyle.color: Color.create("#8FD8C5")
                }
            }
            ListView {
                id: conversation
                property variant chatController: _chat
                property real itemWidth: 720
                property bool followAnswer: true
                onTouch: if (event.isMove()) followAnswer = false
                visible: _chat.messageCount > 0
                layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                dataModel: _chat.messages
                attachedObjects: [
                    LayoutUpdateHandler { onLayoutFrameChanged: conversation.itemWidth = layoutFrame.width }
                ]
                listItemComponents: [
                    ListItemComponent {
                        type: ""
                        Container {
                            id: messageRow
                            property variant chatController: ListItem.view.chatController
                            preferredWidth: ListItem.view.itemWidth
                            topPadding: ui.du(1)
                            bottomPadding: ui.du(1)
                            leftPadding: ListItemData.role === "user" ? ui.du(6) : ui.du(2)
                            rightPadding: ListItemData.role === "user" ? ui.du(2) : ui.du(6)
                            Container {
                                horizontalAlignment: HorizontalAlignment.Fill
                                background: ListItemData.role === "user" ? Color.create("#223C40") : Color.create("#1C232B")
                                topPadding: ui.du(2)
                                bottomPadding: ui.du(2)
                                leftPadding: ui.du(2.5)
                                rightPadding: ui.du(2.5)
                                Label {
                                    text: ListItemData.role === "user" ? "JIJ" : "CLASSIC"
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.fontWeight: FontWeight.Bold
                                    textStyle.color: ListItemData.role === "user" ? Color.create("#8FD8C5") : Color.create("#A8B9CB")
                                }
                                Label {
                                    topMargin: ui.du(1)
                                    text: ListItemData.displayText
                                    horizontalAlignment: HorizontalAlignment.Fill
                                    textFormat: TextFormat.Html
                                    multiline: true
                                    textStyle.base: SystemDefaults.TextStyles.BodyText
                                    textStyle.color: Color.create("#F1F4F6")
                                }
                                Label {
                                    visible: ListItemData.fileName !== undefined && ListItemData.fileName.length > 0
                                    text: ListItemData.fileName !== undefined ? ListItemData.fileName : ""
                                    multiline: true
                                    textFormat: TextFormat.Plain
                                    textStyle.base: SystemDefaults.TextStyles.SmallText
                                    textStyle.color: Color.create("#8FD8C5")
                                }
                                Button {
                                    visible: ListItemData.fileName !== undefined && ListItemData.fileName.length > 0
                                    text: "Opslaan op SD"
                                    enabled: !messageRow.chatController.busy
                                    onClicked: messageRow.chatController.downloadFile(ListItemData.messageIndex)
                                }
                            }
                        }
                    }
                ]
            }
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                background: Color.create("#171D23")
                topPadding: ui.du(1.5)
                bottomPadding: ui.du(1.5)
                leftPadding: ui.du(2)
                rightPadding: ui.du(2)
                Container {
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                    ActivityIndicator { id: spinner; visible: _chat.busy; preferredWidth: ui.du(3); preferredHeight: ui.du(3) }
                    Label {
                        text: _chat.busy ? (_chat.status.indexOf("Zoeken") === 0 ? "Zoeken op internet…" : "Antwoord wordt geschreven…") : (_chat.status.length > 0 ? _chat.status : "Klaar voor je bericht")
                        multiline: true
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: Color.create("#9AA9B5")
                        textFormat: TextFormat.Plain
                        layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                    }
                    Button { text: "Stop"; visible: _chat.busy; preferredWidth: ui.du(11); preferredHeight: ui.du(6); onClicked: _chat.cancel() }
                }
                Container {
                    visible: !_chat.busy && _chat.attachmentName.length > 0
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                    Label {
                        text: "Bijlage: " + _chat.attachmentName
                        textFormat: TextFormat.Plain
                        multiline: true
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: Color.create("#8FD8C5")
                        layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                    }
                    Button { text: "Wis"; preferredWidth: ui.du(10); enabled: !_chat.busy; onClicked: _chat.clearAttachment() }
                }
                Container {
                    visible: !_chat.busy
                    topMargin: ui.du(1)
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                    Button {
                        text: "+"
                        preferredWidth: ui.du(7)
                        verticalAlignment: VerticalAlignment.Center
                        enabled: !_chat.busy
                        onClicked: {
                            if (!_chat.openAttachmentBrowser(true)) _chat.openAttachmentBrowser(false);
                            var page = attachmentPage.createObject();
                            page.selected.connect(function() { navigation.pop(); }); navigation.push(page);
                        }
                    }
                    TextArea {
                        id: composer
                        hintText: "Schrijf een bericht…"
                        enabled: !_chat.busy
                        maximumLength: 16000
                        preferredHeight: ui.du(8)
                        minHeight: ui.du(7)
                        maxHeight: ui.du(11)
                        textFormat: TextFormat.Plain
                        layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                    }
                    Button {
                        leftMargin: ui.du(1)
                        text: _chat.busy ? "Stop" : "Stuur"
                        preferredWidth: ui.du(13)
                        verticalAlignment: VerticalAlignment.Center
                        enabled: _chat.busy || composer.text.trim().length > 0
                        onClicked: { if (_chat.busy) _chat.cancel(); else { conversation.followAnswer = true; _chat.sendMessage(composer.text); } }
                    }
                }
            }
        }
    }
    attachedObjects: [
        Dialog {
            id: menuDialog
            Container {
                horizontalAlignment: HorizontalAlignment.Fill
                verticalAlignment: VerticalAlignment.Fill
                layout: DockLayout { }
                background: Color.create("#B3000000")
                Container {
                    preferredWidth: ui.du(46)
                    horizontalAlignment: HorizontalAlignment.Center
                    verticalAlignment: VerticalAlignment.Center
                    background: Color.create("#1C232B")
                    topPadding: ui.du(2)
                    bottomPadding: ui.du(2)
                    leftPadding: ui.du(2)
                    rightPadding: ui.du(2)
                    Label {
                        text: "Classic Chat"
                        textStyle.base: SystemDefaults.TextStyles.BodyText
                        textStyle.fontWeight: FontWeight.Bold
                        textStyle.color: Color.create("#F1F4F6")
                    }
                    Button {
                        text: "Nieuwe chat"
                        horizontalAlignment: HorizontalAlignment.Fill
                        onClicked: { menuDialog.close(); if (_chat.newChat()) composer.text = ""; }
                    }
                    Button {
                        text: "Gesprekken"
                        horizontalAlignment: HorizontalAlignment.Fill
                        onClicked: {
                            menuDialog.close(); _chat.refreshChats();
                            var page = historyPage.createObject();
                            page.selected.connect(function() { navigation.pop(); }); navigation.push(page);
                        }
                    }
                    Button {
                        text: "Instellingen"
                        horizontalAlignment: HorizontalAlignment.Fill
                        onClicked: {
                            menuDialog.close(); var page = settingsPage.createObject();
                            page.saved.connect(function() { navigation.pop(); }); navigation.push(page);
                        }
                    }
                    Button { text: "Chat opslaan"; horizontalAlignment: HorizontalAlignment.Fill; onClicked: { menuDialog.close(); _chat.saveCurrent(); } }
                    Button { text: "Exporteer als tekst"; horizontalAlignment: HorizontalAlignment.Fill; onClicked: { menuDialog.close(); _chat.exportText(); } }
                    Button { text: "Deze chat verwijderen"; horizontalAlignment: HorizontalAlignment.Fill; onClicked: { menuDialog.close(); deleteDialog.show(); } }
                    Button { text: "Sluiten"; horizontalAlignment: HorizontalAlignment.Fill; onClicked: menuDialog.close() }
                }
            }
        },
        ComponentDefinition { id: attachmentPage; source: "FilesPage.qml" },
        ComponentDefinition { id: historyPage; source: "HistoryPage.qml" },
        ComponentDefinition { id: settingsPage; source: "SettingsPage.qml" },
        SystemDialog {
            id: deleteDialog
            title: "Deze chat verwijderen?"
            body: "Alleen dit gesprek wordt uit Classic Chat verwijderd. Downloads en tekstexports blijven bewaard."
            confirmButton.label: "Verwijder"
            cancelButton.label: "Annuleer"
            onFinished: {
                if (result === SystemUiResult.ConfirmButtonSelection) _chat.deleteCurrent();
            }
        }
    ]
}
