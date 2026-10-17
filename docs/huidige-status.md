# Classic Chat — 17 oktober 2026

Versie 0.1.1 staat op de BlackBerry Classic SQC100-1, OS 10.3.3.3216 / radio 10.3.3.3217. Native tekstchat, internetzoeken met bronverwijzingen, compacte donkere interface en SD-opslag zijn eerder op het toestel bevestigd. De gebruiker bevestigt ook dat de nieuwe versie een echt antwoord geeft via https://classicchat.130-61-37-71.sslip.io.

De broncode staat op https://github.com/Chemske/classic-chat. De eerste werkende basis is bewaard als v0.1.0. Private configuraties, SSH-sleutels, media, firmware en back-ups zijn uitgesloten.

Een volledige toestelherstart is uitgevoerd. De standaard mobiele dienst coreServices2 startte vanzelf en de gebruiker bevestigde dat mobiel netwerk aan bleef. De root-bootstrap voltooide opnieuw en meldde daarna succes. Een eerste poging Classic Chat te openen vóór dat moment sloot onmiddellijk; na voltooiing bleef de app open. Wacht na een volgende herstart dus op het beginscherm én voltooiing van de root-voorbereiding. Eén herstart is bevestigd; langdurige betrouwbaarheid is nog niet vastgesteld.

De VPS is ook volledig herstart. De boot-ID veranderde en relay, HTTPS-proxy en vast adres kwamen automatisch terug. De Mac is voor chatgebruik niet nodig. Er is geen Cloudflare-account of eigen domein vereist; het adres blijft afhankelijk van het bestaande publieke IP en sslip.io.

Qt op dit toestel bood alleen TLS 1.0 aan. Versie 0.1.1 gebruikt daarom een cancellable worker met de aanwezige native libcurl en expliciet TLS 1.2. Het publieke ISRG Root X1-certificaat is gebundeld. Certificaat- en hostnaamcontrole blijven verplicht; HTTP en redirects worden niet gevolgd. Het aparte toestel-token staat alleen in de private appinstellingen; de OpenAI-sleutel blijft op de VPS. Het eenmalige setupbestand is na import verwijderd.

Het geïnstalleerde programma, de drie QML-schermen en het CA-certificaat zijn teruggelezen en komen exact met de nieuwe BAR overeen. De app laadt zonder QML-fouten. In deze update is geen firmware geflasht, geen andere app verwijderd en geen chat/media gewist.

Chats staan standaard op SD onder ClassicChat/chats; exports onder ClassicChat/exports. TXT-export en de volledige geschiedenisinterface zijn nog niet apart doorlopen. Streaming en bijlagen zijn nog niet geïmplementeerd. Zie VPS-handleiding.md, build.md en recovery.md voor beheer en herstel.
