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

De uitkomst van die volledige herstarttest is bij de eerste broncodeversie nog niet bevestigd.
