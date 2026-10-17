# Toestelinstallatie en herstel

Deze notities beschrijven de geteste SQC100-1. Ze zijn geen algemene firmwarehandleiding voor andere BlackBerry-modellen.

## Vastgestelde basis

- OS 10.3.3.3216; radio 10.3.3.3217; hardware-ID 0x9600270a.
- Originele OS- en radiofirmware zijn daadwerkelijk gebruikt om het toestel te herstellen na een mislukte voorbereiding. Daarna is de aangepaste gebruikerspartitie opgestart.
- Er is een op controlesommen gecontroleerde kopie van gedeelde bestanden en de SD-kaart. Er is geen volledige privégegevens- of ruwe firmwareback-up.
- Roottoegang is op het toestel bevestigd. Classic Chat is zonder debug token geïnstalleerd en de executable plus QML zijn teruggelezen.
- Alle firmwarebestanden, sleutels en persoonlijke back-ups blijven buiten Git.

## Mobiele radio schakelt niet in

Bij het onderzochte probleem werd de Proximus-simkaart correct herkend en was vliegtuigmodus uit. De systeemdienst `coreServices2` draaide niet. Daardoor had `/pps/services/unifiedradiomgr/control` geen dienst die netwerkopdrachten afhandelde.

De oorspronkelijke startaanroep uit het fabrieksopstartscript was:

```sh
/base/usr/sbin/coreServices2 -U30:0,22,31,88,100,335,361,368,1000,1001,1002,1200,750,751 -S31:32 -M32:32
```

Voer die alleen uit vanuit een gecontroleerde rootverbinding wanneer de dienst ontbreekt. Een tweede instantie starten terwijl de dienst al draait is geen algemene herstelstap. Na de geteste herstart stonden modem en mobiele verbinding weer aan; de gebruiker bevestigde herstel. Waarom de dienst was gestopt, is nog niet vastgesteld.

## Een volledige herstart testen

1. Bewaar een herstelkopie van de huidige app en de private instellingen buiten Git.
2. Controleer batterij, bereikbaarheid en de ontwikkelverbinding.
3. Voer een gewone toestelherstart uit; gebruik geen BootROM-, flash- of wiscommando.
4. Controleer wifi, simkaart, mobiele radio, coreServices2 en het openen van Classic Chat.
5. Als de app niet start, leg eerst foutcode en rootstatus vast; installeer of flash niet blind opnieuw.

De uitgevoerde controle staat hieronder.

## Herstartcontrole, 17 oktober 2026

Een volledige normale toestelherstart is bevestigd. coreServices2 startte zelfstandig; mobiel netwerk bleef volgens de gebruiker aan. Roottoegang via de bestaande helper is opnieuw gecontroleerd. De bootstrap meldt na enige tijd dat het toestel rooted is. Classic Chat sloot bij een start vóór die voltooiing, maar bleef daarna open. Wacht na herstart op de root-melding en open de app opnieuw; flash daarvoor geen firmware.

Een eerdere opdracht via de Mac werd onderbroken voordat de normale afsluitprocedure klaar was. Gebruik het aan/uitmenu voor een normale herstart; stuur geen tweede herstart terwijl de eerste loopt. Er is voor deze controle geen firmware overgedragen.

Versie 0.1.1 is daarna als appupdate geïnstalleerd en byte voor byte gecontroleerd, inclusief het openbare CA-certificaat. De oorspronkelijke werkende bron blijft in Git-tag v0.1.0. Gebruik bij een rollback ook het installatiescript uit die tag: de nieuwere installer verwacht aanvullend het CA-bestand dat nog niet in het oude pakket zat. De BAR-bestanden en bestaande privé-back-ups blijven lokaal en horen niet in Git. De oude tijdelijke endpoint is gestopt; een oude build heeft geen bevestigde TLS 1.2-route naar het nieuwe vaste adres.

## Appupdate 0.2.0

De update verandert alleen Classic Chat en de relay; er is geen firmware geflasht. De vorige relaycode staat privé op de VPS onder /opt/classicchat/server/server.before-0.2.0.mjs. Een rollback bestaat uit het gecontroleerd terugplaatsen van die code en herstarten van classicchat.service; de oude native client gebruikt het bestaande JSON-protocol. De nieuwe relay accepteert ook oude clients.

De lokale BAR van versie 0.1.1 en de private instellingen vóór 0.2.0 zijn behouden. Voor een oude BAR gebruik je het installatiescript uit dezelfde Git-tag: de nieuwe installer verwacht ook FilesPage.qml en de drie nieuwe pictogrammen. Bijlagen worden niet duurzaam naar de VPS geschreven. Downloads die de gebruiker op SD opslaat, blijven bij het verwijderen van een chat bestaan.
