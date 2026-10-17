import bb.cascades 1.3
Page {
    signal saved()
    titleBar: TitleBar { title: "Instellingen" }
    ScrollView {
        Container {
            background: Color.create("#101418")
            topPadding: ui.du(3)
            leftPadding: ui.du(3)
            rightPadding: ui.du(3)
            Label { text: "Jouw tussenserver"; textStyle.base: SystemDefaults.TextStyles.TitleText }
            TextField {
                id: server
                hintText: "https://jouw-server.example"
                text: _chat.serverUrl
                inputMode: TextFieldInputMode.Url
            }
            Label { topMargin: ui.du(3); text: "Toestel-token" }
            TextField {
                id: token
                hintText: _chat.tokenConfigured ? "Ingesteld; leeg laten om te behouden" : "Eigen token, minimaal 32 tekens"
                inputMode: TextFieldInputMode.Password
            }
            Label {
                text: "Dit is het toegangstoken van jouw server. De OpenAI-sleutel blijft uitsluitend op je server."
                multiline: true
                textStyle.base: SystemDefaults.TextStyles.SmallText
            }
            Container {
                topMargin: ui.du(3)
                layout: StackLayout { orientation: LayoutOrientation.LeftToRight }
                Label { text: "Chats op SD-kaart"; layoutProperties: StackLayoutProperties { spaceQuota: 1 } }
                ToggleButton { id: sd; checked: _chat.saveOnSd }
            }
            Label {
                text: "Met SD-opslag aan worden chats niet op interne opslag bewaard. Zonder kaart blijven nieuwe antwoorden alleen in het geheugen. Met deze optie uit kies je bewust voor interne opslag."
                multiline: true
                textStyle.base: SystemDefaults.TextStyles.SmallText
            }
            Button {
                topMargin: ui.du(3)
                horizontalAlignment: HorizontalAlignment.Fill
                text: "Instellingen opslaan"
                onClicked: { if (_chat.configure(server.text, token.text, sd.checked)) { token.text = ""; saved(); } }
            }
            Label { text: _chat.status; multiline: true; textFormat: TextFormat.Plain }
        }
    }
}
