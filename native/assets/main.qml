import bb.cascades 1.3
import bb.system 1.0

NavigationPane {
    id: navigation
    onPopTransitionEnded: page.destroy()
    onCreationCompleted: {
        _chat.answerReceived.connect(function() {
            composer.text = "";
            conversation.scrollToPosition(ScrollPosition.End, ScrollAnimation.Smooth);
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
                property real itemWidth: 720
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
                        text: _chat.status.length > 0 ? _chat.status : "Klaar voor je bericht"
                        multiline: true
                        textStyle.base: SystemDefaults.TextStyles.SmallText
                        textStyle.color: Color.create("#9AA9B5")
                        textFormat: TextFormat.Plain
                        layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                    }
                }
                Container {
                    topMargin: ui.du(1)
                    layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                    TextArea {
                        id: composer
                        hintText: "Schrijf een bericht…"
                        enabled: !_chat.busy
                        maximumLength: 16000
                        preferredHeight: ui.du(10)
                        textFormat: TextFormat.Plain
                        layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                    }
                    Button {
                        leftMargin: ui.du(1)
                        text: _chat.busy ? "Stop" : "Stuur"
                        preferredWidth: ui.du(13)
                        verticalAlignment: VerticalAlignment.Center
                        enabled: _chat.busy || composer.text.trim().length > 0
                        onClicked: { if (_chat.busy) _chat.cancel(); else _chat.sendMessage(composer.text); }
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
        ComponentDefinition { id: historyPage; source: "HistoryPage.qml" },
        ComponentDefinition { id: settingsPage; source: "SettingsPage.qml" },
        SystemDialog {
            id: deleteDialog
            title: "Deze chat verwijderen?"
            body: "Alleen dit gesprek wordt uit Classic Chat verwijderd. Eventuele tekstexports blijven bewaard."
            confirmButton.label: "Verwijder"
            cancelButton.label: "Annuleer"
            onFinished: {
                if (result === SystemUiResult.ConfirmButtonSelection) _chat.deleteCurrent();
            }
        }
    ]
}
