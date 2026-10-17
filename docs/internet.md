# Classic Chat — VPS en verdere installatie

Bijgewerkt op 16 oktober 2026. Dit document vervangt de eerdere status waarin de Mac de publieke server draaide.

## Wat werkt

De tussenserver draait op jouw Oracle VPS, onafhankelijk van de Mac. Het huidige publieke adres is:

**https://retrieve-cos-questions-fancy.trycloudflare.com**

Je hebt de statuspagina op de BlackBerry geopend en bevestigd dat deze werkt. Een afzonderlijke echte API-aanroep via dit publieke adres gaf **“VPS werkt.”** terug. De server kan via wifi en mobiele data worden benaderd; de native app moet nog op het toestel worden geïnstalleerd en getest.

Dit is een tijdelijk Cloudflare-adres. Het kan veranderen wanneer de tunnel of VPS opnieuw start. Er is nog geen vast domein ingesteld. De Mac hoeft niet aan te staan. De VPS moet aanstaan en internet hebben.

De server gebruikt HTTPS en een apart toestel-token voor chatverzoeken. De OpenAI-sleutel staat in de private serverconfiguratie. De BlackBerry krijgt uiteindelijk alleen het serveradres en het toestel-token. Zonder token geeft chatten HTTP 401; een verzoek naar `/.env` gaf 404.

## Serverbeheer

Verbind via SSH als `ubuntu` op `130.61.37.71`, met de bestaande sleutel in `Bureaublad/blackberry/ssh-to-vps`. De dienst draait onder de eigen gebruiker `classicchat` en start automatisch met de VPS.

Na verbinden via SSH:

```sh
sudo systemctl status classicchat.service --no-pager
sudo cat /var/lib/classicchat/Internetadres.txt
```

De eerste opdracht controleert de dienst; de tweede toont het actuele internetadres. Dienst stoppen of opnieuw starten:

```sh
sudo systemctl stop classicchat.service
sudo systemctl restart classicchat.service
```

Na een herstart moet het actuele adres opnieuw worden gelezen en zo nodig in de app worden aangepast. De dienst maakt zelf een nieuwe tijdelijke tunnel aan.

| Onderdeel | Locatie op de VPS |
| --- | --- |
| Servercode en supervisor | `/opt/classicchat/server/` |
| Private configuratie | `/etc/classicchat/relay.env` |
| Actueel adres en dienststatus | `/var/lib/classicchat/` |
| Systeemdienst | `/etc/systemd/system/classicchat.service` |

De configuratie bevat de OpenAI-sleutel en het toestel-token. Neem de inhoud niet over in chat, screenshots of een broncodearchief. Er is een lokale ontwikkelkopie op de Mac gebleven. De eerdere Mac-internetdienst is gestopt en automatisch starten daarvan is uitgeschakeld.

De VPS gebruikt Node.js 24.22.0 en cloudflared 2026.10.1 voor ARM64. De downloads zijn tegen gepubliceerde SHA-256-waarden gecontroleerd. De relay luistert op localhost; Cloudflare Tunnel verzorgt de publieke HTTPS-verbinding. Hiervoor zijn geen routerpoorten of inkomende poorten 80/443 op de VPS geopend. De bestaande SSH- en firewallinstellingen zijn behouden.

## Back-up en herstel

De gedeelde interne bestanden en de SD-kaart zijn naar de Mac gekopieerd en met SHA-256 gecontroleerd:

| Kopie | Bestanden | Bytes |
| --- | ---: | ---: |
| Gedeelde interne bestanden | 46 | 68.748.602 |
| SD-kaart | 107 | 7.764.057 |
| Totaal | 153 | 76.512.659 |

Ze staan in `Bureaublad/blackberry/backups/2026-10-16-shared-files` en `2026-10-16-sdcard`. De eerste kopie registreerde een mislukte SD-selectie; de SD-kaart is vervolgens afzonderlijk volledig gekopieerd. De oorspronkelijke manifests blijven bewaard.

De standaardback-up voor private appgegevens en instellingen is **niet gelukt**. Er is geen bruikbaar volledig BBB-archief ontvangen. Je hebt bevestigd dat er geen belangrijke privégegevens aanwezig zijn. Er is ook **geen ruwe kopie van de huidige firmware** van het toestel gemaakt.

Een stockfirmwarearchief met dezelfde OS- en radioversie is gedownload en lokaal uitgepakt: OS 10.3.3.3216, radio 10.3.3.3217, qc8960. De checksum komt overeen met de community-archiefmetadata. Dit bestand is geen back-up van jouw toestel en herstel ermee is nog niet op jouw toestel bewezen.

## Native app: huidige blokkade

Classic Chat is als echte Cascades-app voor QNX ARMv7 gebouwd. Het BAR-bestand is aanwezig en de pakketcontroles slagen. De gewone installatie is eerder geweigerd met **881: no debug token found**. De app is nog niet geïnstalleerd.

Op 16 oktober is de herstelmodus zonder firmwarewijziging benaderd. Het BootROM-model is rechtstreeks uitgelezen als **0x9600270a**, passend bij de SQC100-1. Er is geen wachtwoordpoging in die herstelmodus verstuurd, geen RAM-loader geüpload en geen firmware geschreven. De telefoon is daarna weer normaal opgestart.

De beschikbare Mac-tool bevat geen gevalideerde loaderkoppeling voor dat exacte model. Het gedownloade stock-archief heeft evenmin een expliciete descriptor voor dit ID. Dat bewijst geen algemene incompatibiliteit, maar de herstelroute is hiermee nog onvoldoende bevestigd om te flashen.

De AutoRoot v3-patch is met REA en aanvullende broncode-inspectie onderzocht. De oorspronkelijke patch verwijdert apps en stopt diensten. In de lokale reviewversie zijn die onderdelen uitgeschakeld. Deze versie is nog geen vrijgegeven firmwarebeeld; de overige wijzigingen en modelondersteuning moeten eerst worden gevalideerd. De later beschreven AutoRoot v4-techniek levert momenteel evenmin een aangetoonde installatieroute op dit stocktoestel.

Volgende concrete stappen:

1. Een passende, ondertekende herstel-loader voor SQC100-1 valideren, met eerst alleen uitlezen en terug opstarten.
2. De aangepaste firmware offline bouwen en controleren, zonder expliciete appverwijdering en met eigen SSH-sleutels. Flashen van een gebruikersbestandssysteem kan bestaande appgegevens vervangen; de reviewpatch voorkomt dat niet op zichzelf.
3. Pas met een bewezen herstelroute de systeemwijziging en installatie van uitsluitend Classic Chat uitvoeren.
4. Op het toestel starten, toetsenbord/trackpad, HTTPS naar de VPS en SD-opslag testen.

De browserproef bewijst internetbereikbaarheid, maar nog niet dat de native Qt-netwerkclient dezelfde certificaatketen accepteert. Opslaan, exporteren en verwijderen van eigen chats moeten ook nog op het toestel worden getest.

## Bronnen en bewijs

- `ClassicChat-VPS-test.json`: echte API-aanroep via de VPS.
- `ClassicChat-netwerkcontrole.json`: bereikbaarheid en toegangscontroles.
- `BlackBerry-backupstatus.json`: samengevoegde back-upstatus.
- `BlackBerry-herstelcontrole/bootrom-model.json`: beperkte modeluitlezing zonder privé-identificatoren.
- [AutoRoot v3, instructies van de maker](https://bb10.root.sx/blog/pre-rooted-autoloaders-v3).
- [Broncode van bb10mt](https://github.com/bb10root/bb10mt).
- [Beschrijving van de v4-techniek](https://bb10.root.sx/blog/dynamic-library-hijacking-vulnerability-in-bb10).

Er is in deze testfase niets op de BlackBerry verwijderd, geroot of geflasht. Er is geen herstelactie uitgevoerd.
