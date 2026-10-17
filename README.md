# Classic Chat

Een native AI-chatapp voor de BlackBerry Classic met BlackBerry 10. De C++/QML-client gebruikt Cascades en praat via HTTPS met een eigen Node.js-tussenserver. De OpenAI API-sleutel blijft op de server; het toestel gebruikt een afzonderlijk toegangstoken.

## Huidige versie — 0.1.1

De werkende basis is op 17 oktober 2026 getest op een SQC100-1 met OS 10.3.3.3216 en radio 10.3.3.3217. Chatten, internetzoeken met bronverwijzingen en automatisch opslaan op SD zijn op het toestel bevestigd. De donkere interface heeft een compacte bovenbalk en een Menu-knop voor gesprekken en instellingen.

De app ondersteunt tekstgesprekken, annuleren, lokale gespreksgeschiedenis en TXT-export. De server biedt het model een webzoekfunctie aan. De huidige client ontvangt het volledige antwoord; streaming, bestandsbijlagen en foto-invoer zijn geplande uitbreidingen.

Versie 0.1.1 gebruikt de aanwezige native libcurl met TLS 1.2 en een gebundeld publiek ISRG Root X1-certificaat. Certificaat- en hostnaamcontrole blijven ingeschakeld. Het vaste serveradres is op de BlackBerry getest met een echt antwoord.

## Project

- `native/`: C++/QML-client en pakketdescriptor.
- `server/`: Node.js-relay met toestel-authenticatie, limieten en webzoeken.
- `tests/`: servertests met een nagebootste modelprovider.
- `tools/`: native bouwscript en installer voor een reeds voorbereid toestel.
- `deploy/vps/`: VPS-installatie en systemd-voorbeeldconfiguratie.
- `docs/`: bouwroute, toestelinstallatie en herstelnotities.

## Server lokaal testen

Gebruik Node.js 22 of nieuwer. Maak een private configuratie op basis van `server/.env.example`; commit die configuratie niet.

```sh
node --test tests/server.test.mjs
node --env-file=/pad/naar/private.env server/server.mjs
```

De relay luistert op `127.0.0.1:8787`. `DEMO_MODE=1` voorkomt modelaanroepen. Voor echte gesprekken zijn `OPENAI_API_KEY`, `OPENAI_MODEL` en `DEVICE_TOKEN` vereist. De modelnaam komt uitsluitend uit de serverconfiguratie. De client kan geen systeeminstructies, model of tools toevoegen aan het protocol.

## Bouwen en installeren

Zie [de bouwinstructies](docs/build.md). De native build vereist de oorspronkelijke BB10-targetheaders, ARM-bibliotheken, Qt 4-moc en de BAR-packager; die gereedschappen en hun licenties worden niet meegeleverd. `tools/build-native.py` bouwt uitsluitend de app.

Het ontwikkelpakket is ongetekend. Een gewone stock-installatie kan een geldig debug token vereisen. De gebruikte route zonder debug token vereist een reeds voorbereid toestel met bevestigde roottoegang. `tools/install-classicchat-rooted.py` bereidt geen stocktoestel voor en flasht geen firmware. Het controleert de SSH-hostsleutel en vergelijkt na installatie de executable en schermbestanden.

## Opslag en privacy

Chats staan standaard op de SD-kaart onder `ClassicChat/chats`; TXT-exports staan onder `ClassicChat/exports`. Zonder kaart valt de app niet stilzwijgend terug op interne opslag. De kleine appinstellingen gebruiken wel interne opslag. Chatbestanden op SD zijn gewone, onversleutelde tekst.

De relay bewaart geen blijvend chatarchief en logt geen berichtinhoud. Hij gebruikt de Responses API met `store:false`; dat is geen garantie dat OpenAI geen andere bewaartermijnen toepast. Internetzoeken kan aanvullende API-kosten veroorzaken.

Sleutels, tokens, persoonlijke media, toestelback-ups, firmware-images en ontwikkeltools horen niet in deze repository.

## Bekende aandachtspunten

- Het vaste adres is https://classicchat.130-61-37-71.sslip.io. Het blijft afhankelijk van sslip.io en het publieke VPS-IP. Zie [de VPS-handleiding](docs/VPS-handleiding.md).
- Toestel en VPS zijn volledig herstart en gecontroleerd. Wacht op het toestel tot de root-voorbereiding klaar is voordat je de app opent; een eerdere start kan sluiten. Zie [de huidige status](docs/huidige-status.md).
- Bij een eerder uitgeschakelde mobiele radio bleek `coreServices2` niet te draaien. Starten van de oorspronkelijke dienst herstelde de netwerkschakelaar. Zie [de herstelnotities](docs/recovery.md).
- TXT-export, de volledige geschiedenisinterface en ontbrekende/volle SD-kaarten zijn nog niet allemaal apart getest.

## Bronnen

- [BlackBerry Cascades-voorbeelden](https://github.com/blackberry/Cascades-Samples)
- [OpenAI Responses API](https://developers.openai.com/api/docs/guides/text)
- [OpenAI web search](https://developers.openai.com/api/docs/guides/tools-web-search)
