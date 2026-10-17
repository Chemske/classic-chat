# Classic Chat — werkende native installatie, 17 oktober 2026

Classic Chat is op de BlackBerry Classic SQC100-1 geïnstalleerd via de voorbereide root-pakketinstaller, zonder debug token. Het programma en de QML-schermen zijn teruggelezen en komen exact overeen met het geleverde BAR-pakket. De app blijft draaien; QML meldt geen laadfouten. De gebruiker bevestigt dat chatten werkt.

Het toestel gebruikt HTTPS naar de eigen Oracle VPS. Alleen het aparte toestel-token staat in de privé-instellingen van de app; de OpenAI API-sleutel blijft op de server. Het eenmalige configuratiebestand is na import verwijderd. Het actuele tijdelijke adres is https://pts-bracelets-beginners-organizational.trycloudflare.com. De Mac hoeft voor chatgebruik niet aan te staan. Het tijdelijke adres kan bij een tunnelherstart veranderen.

Een geslaagd gesprek is daadwerkelijk op de SD-kaart aangetroffen in ClassicChat/chats. Het JSON-bestand is uitgelezen en bevat zowel het gebruikersbericht als een niet-leeg antwoord. De inhoud is niet in het publieke testverslag opgenomen. De app schrijft geen chats naar interne opslag wanneer SD-opslag is ingeschakeld en de kaart ontbreekt.

De opstartfout was een niet-geregistreerd SystemDialog-type. De build linkt nu bbsystem en registreert SystemDialog, SystemUiButton en SystemUiResult vóór het laden van QML. Ook het privé-opslagpad is gecorrigeerd: QDir::homePath() verwijst op dit toestel al naar de data-map. De installer controleert dat geen meerdere SUD-werkers actief zijn en vergelijkt de geïnstalleerde programmabestanden na installatie.

TXT-export, chats heropenen/verwijderen via de interface, mobiel internet en functioneren na een volledige toestelherstart zijn nog niet apart gecontroleerd. Herstart het toestel voorlopig niet alleen om te testen; de root-opstartketen en de USB-netwerkconfiguratie moeten nog verder worden gecontroleerd. Wifi werkt.

De telefoon is eerder met originele OS- en radiofirmware hersteld en daarna met de aangepaste gebruikerspartitie opgestart. Er is een gecontroleerde kopie van gedeelde bestanden en de SD-kaart, maar geen volledige privégegevens- of ruwe firmwareback-up. Voor deze crashfix is geen firmware opnieuw geflasht en zijn geen andere apps verwijderd.

## Donker ontwerp en internetzoeken

De donkere update is geïnstalleerd; webzoeken met bronverwijzingen is ingeschakeld op de VPS. Zie `docs/dark-web-update.md` voor de actuele controles.

## Compactere interface

De vaste onderbalk is vervangen door Menu rechtsboven. De bovenbalk is lager en vult de volledige breedte. Zie `docs/compact-ui-update.md`.
