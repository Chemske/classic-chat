import bb.cascades 1.3
Page {
    signal selected()
    titleBar: TitleBar { title: "Gesprekken" }
    Container {
        background: Color.create("#101418")
        leftPadding: ui.du(3)
        rightPadding: ui.du(3)
        topPadding: ui.du(2)
        Label {
            text: "Recente gesprekken op de gekozen opslaglocatie (maximaal 200)."
            multiline: true
            textStyle.base: SystemDefaults.TextStyles.SmallText
        }
        ListView {
            id: saved
            layoutProperties: StackLayoutProperties { spaceQuota: 1 }
            dataModel: _chat.chats
            listItemComponents: [
                ListItemComponent {
                    type: ""
                    StandardListItem {
                        title: ListItemData.title
                        description: ListItemData.updated
                    }
                }
            ]
            onTriggered: {
                var item = dataModel.data(indexPath);
                if (_chat.loadChat(item.id)) selected();
            }
        }
        Label { text: _chat.status; multiline: true; textFormat: TextFormat.Plain }
    }
}
