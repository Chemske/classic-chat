# VPS en vast HTTPS-adres

Actueel, 17 oktober 2026: https://classicchat.130-61-37-71.sslip.io. Het publieke IP is 130.61.37.71. Dit adres gebruikt sslip.io en vereist geen afzonderlijk domein of Cloudflare-account. Het blijft gelijk zolang het publieke VPS-IP gelijk blijft; DNS blijft afhankelijk van sslip.io.

## Actieve inrichting

Ubuntu 24.04 ARM64 met een onprivileged `classicchat`-account. Node staat onder `/opt/classicchat/node`; de relay onder `/opt/classicchat/server/server.mjs`. De private configuratie staat uitsluitend in `/etc/classicchat/relay.env`, eigenaar root:classicchat, modus 640. De relay luistert op 127.0.0.1:8787.

Caddy uit de ondertekende Ubuntu-pakketten beheert HTTPS en certificaatvernieuwing. `/etc/caddy/Caddyfile` bevat het vaste adres, `tls { key_type rsa2048 }` en `reverse_proxy 127.0.0.1:8787`. Zie `deploy/vps/Caddyfile.example`. De ACME-CA is expliciet Let's Encrypt. TCP 80 en 443 moeten zowel in Oracle als op de host bereikbaar blijven. Poort 8787 wordt niet openbaar gemaakt.

`classicchat.service` start de relay rechtstreeks; er draait geen tijdelijke Cloudflare-tunnel meer. `classicchat` en `caddy` zijn ingeschakeld voor automatisch opstarten. De bestaande Oracle-firewallregels zijn behouden; alleen TCP 80/443 zijn toegestaan en met netfilter-persistent opgeslagen.

## Nieuwe installatie

De voorbeelden zijn geen script om een bestaande server te overschrijven.

1. Gebruik een lege stagingmap op een Ubuntu ARM64-machine. Download met `deploy/vps/download-runtime.py` de gecontroleerde Node-runtime. De optionele `--with-tunnel` is uitsluitend voor het oude tijdelijke ontwikkeltransport.
2. Kopieer `server/server.mjs`, `deploy/vps/classicchat.service` en `deploy/vps/install.sh` naar dezelfde stagingmap. `install.sh` verwacht de runtime en `runtime-provenance.json` daar en weigert een bestaande `/opt/classicchat` te overschrijven. Voer het script met sudo uit.
3. Maak `/etc/classicchat/relay.env` op basis van `server/.env.example`, met de echte API-sleutel en een apart toestel-token. Gebruik root:classicchat en modus 640. Geen sleutels in Git, shellgeschiedenis of BAR-pakketten.
4. Installeer Caddy uit de Ubuntu-pakketten. Vul een correct resolvend hostadres in het configuratievoorbeeld in en laat Caddy de configuratie valideren. Open in Oracle en de hostfirewall TCP 80/443; behoud de bestaande SSH- en Oracle-regels.
5. Start en activeer classicchat en caddy met systemd. Controleer `/health` vanaf buiten de VPS en controleer dat een aanvraag zonder toestel-token 401 krijgt.
6. Configureer de app met het HTTPS-adres en uitsluitend het aparte toestel-token. Versie 0.1.1 gebruikt de native TLS 1.2-route; oudere Qt-builds zijn niet geschikt voor deze endpoint.

## Gecontroleerd op de actieve VPS

- DNS resolveert naar 130.61.37.71; HTTPS en geforceerd TLS 1.2 worden vanaf de Mac geverifieerd.
- `/health` geeft 200 met `ok:true,demo:false`; `/.env` geeft 404 en een chatrequest zonder token 401.
- Een volledige VPS-herstart is uitgevoerd. De Linux boot-ID veranderde; beide diensten waren actief en ingeschakeld en extern HTTPS werkte na circa 26 seconden opnieuw.
- De BlackBerry gebruikt libcurl 7.42.0 / OpenSSL 1.0.2g. TLS 1.2 met het vaste adres is op het toestel getest; een onjuist HTTPS-hostadres wordt afgewezen. De gebruiker heeft een echt antwoord via de bijgewerkte native app bevestigd.

## Beheer en storingen

Controleer `systemctl status classicchat caddy` en beperkte journalregels zonder berichtinhoud of headers te publiceren. De runtime logt geen chatinhoud. Het publieke adres is geen geheim; beide toegangssleutels zijn dat wel. Back-ups van de private configuratie en Caddy-certificaatopslag moeten privé blijven.

Bij een veranderd VPS-IP moet ook het ingebedde IP in het sslip.io-adres veranderen. Bij certificaatproblemen controleer DNS, poorten, klok, Caddy en de CA-keten; schakel certificaat- of hostnaamcontrole nooit uit. Bewaar poort 80 voor ACME/HTTPS-doorverwijzing. De huidige appbundel vertrouwt ISRG Root X1 voor deze Let's Encrypt-keten. Een andere CA kan een bewust bijgewerkte trustbundel vereisen.

Een noodterugkeer naar de historische tunnel is een afzonderlijke beheerhandeling: een quick-tunneladres wijzigt bij herstart en de app moet dan opnieuw geconfigureerd worden. De vorige relay-unit staat privé op de server als `classicchat.service.before-fixed`; de oude URL is niet meer actief.
