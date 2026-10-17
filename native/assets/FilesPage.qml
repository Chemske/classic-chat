import bb.cascades 1.3
Page {
    signal selected()
    titleBar: TitleBar { title: "Bestanden en foto’s" }
    Container {
        background: Color.create("#101418")
        horizontalAlignment: HorizontalAlignment.Fill
        topPadding: ui.du(1)
        leftPadding: ui.du(2)
        rightPadding: ui.du(2)
        Container {
            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
            Button {
                text: "SD-kaart"
                layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                onClicked: _chat.openAttachmentBrowser(true)
            }
            Button {
                text: "Toestel"
                leftMargin: ui.du(2)
                layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                onClicked: _chat.openAttachmentBrowser(false)
            }
        }
        Container {
            topMargin: ui.du(2)
            bottomMargin: ui.du(2)
            layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
            Label {
                text: _chat.attachmentFolder
                multiline: true
                textFormat: TextFormat.Plain
                textStyle.color: Color.create("#8FD8C5")
                textStyle.base: SystemDefaults.TextStyles.SmallText
                layoutProperties: StackLayoutProperties { spaceQuota: 1 }
                verticalAlignment: VerticalAlignment.Center
            }
            Button { text: "Omhoog"; leftMargin: ui.du(2); preferredWidth: ui.du(13); onClicked: _chat.parentAttachmentFolder() }
        }
        ListView {
            id: files
            property variant chatController: _chat
            dataModel: _chat.attachmentFiles
            layout: GridListLayout { columnCount: 3; cellAspectRatio: 0.85; horizontalCellSpacing: ui.du(2); verticalCellSpacing: ui.du(2) }
            layoutProperties: StackLayoutProperties { spaceQuota: 1 }
            listItemComponents: [
                ListItemComponent {
                    type: ""
                    Container {
                        id: tile
                        horizontalAlignment: HorizontalAlignment.Fill
                        verticalAlignment: VerticalAlignment.Fill
                        background: Color.create("#1C232B")
                        topPadding: ui.du(1.5)
                        bottomPadding: ui.du(1)
                        leftPadding: ui.du(1)
                        rightPadding: ui.du(1)
                        ImageView {
                            image: tile.ListItem.view.chatController.attachmentThumbnail(ListItemData.path)
                            horizontalAlignment: HorizontalAlignment.Center
                            scalingMethod: ScalingMethod.AspectFit
                            preferredWidth: ui.du(12)
                            preferredHeight: ui.du(11)
                        }
                        Label {
                            text: ListItemData.name
                            multiline: true
                            textFormat: TextFormat.Plain
                            textStyle.base: SystemDefaults.TextStyles.SmallText
                            textStyle.color: ListItemData.directory ? Color.create("#8FD8C5") : Color.create("#F1F4F6")
                            maxHeight: ui.du(7)
                            horizontalAlignment: HorizontalAlignment.Fill
                        }
                    }
                }
            ]
            onTriggered: if (_chat.pickAttachment(indexPath[0])) selected()
        }
        Label {
            text: _chat.status
            multiline: true
            textFormat: TextFormat.Plain
            textStyle.color: Color.create("#9AA9B5")
            textStyle.base: SystemDefaults.TextStyles.SmallText
        }
    }
}
