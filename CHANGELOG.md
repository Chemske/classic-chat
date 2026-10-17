# Wijzigingen

## v0.2.0 — 17 oktober 2026

Antwoorden verschijnen tijdens het genereren. Annuleren of een verbroken stream bewaart geen onvolledig antwoord. De plusknop opent een donkere bestandskiezer met SD-kaart, toestelopslag en mapnavigatie. Eén kleine UTF-8-tekstbijlage of verkleinde foto kan per bericht worden meegestuurd. Het model kent BB10 en kan een veilig begrensd tekstbestand aanbieden met een echte Opslaan op SD-knop. De relay gebruikt alleen geheugen voor deze bestanden; de app bewaart downloadbare tekst bij de lokale chat. Bestaande bestanden worden niet overschreven en downloads vallen niet terug op interne opslag. Oude clients blijven werken. Inclusief twintig servertests en een native SD-testhulpmiddel.

## v0.1.1 — 17 oktober 2026

Vast HTTPS-adres via Caddy op de VPS, zelfstandig opstartende relay zonder tijdelijke tunnel en gecontroleerde volledige VPS-herstart. Toestelherstart bevestigt opnieuw roottoegang en werkend mobiel netwerk; de app opent zodra de root-bootstrap klaar is. TLS 1.2 via de native libcurl vervangt de beperkte Qt-TLS-route, met verplichte CA/hostnaamcontrole en gebundeld publiek ISRG Root X1. Behoud van annuleren, antwoordlimiet en time-out. De gebruiker bevestigt een echt antwoord via de bijgewerkte app. Installatie vergelijkt nu ook het CA-bestand. Beheer- en herstelnotities bijgewerkt.

## v0.1.0 — 17 oktober 2026

Eerste werkende native basis: C++/QML-chat, eigen VPS-relay, internetzoeken met inline bronlinks, aparte toestel-authenticatie, automatische SD-opslag en een compacte donkere interface met menu. Inclusief servertests, bouwscript, gecontroleerde installer en herstelnotities. Streaming en bijlagen zijn nog niet geïmplementeerd.
