# Streaming en bestanden — versie 0.2.0

De nieuwe client ontvangt antwoordtekst tijdens het genereren. Een voorlopig antwoord blijft zichtbaar tijdens de aanvraag; pas een geldig eindbericht wordt met het gesprek opgeslagen. Bij annuleren, een fout of een verbroken verbinding wordt het onvolledige antwoord niet opgeslagen. De invoertekst en eventuele bijlage blijven staan voor een bewuste nieuwe poging. Er is geen automatische betaalde herhaling.

## Bediening

- De plusknop naast het invoerveld opent het donkere bestandsraster van de app, met mappictogrammen, documenticonen en fotovoorbeelden. SD-kaart, Toestel en Omhoog geven expliciete toegang tot beide opslaglocaties. Per map worden maximaal 500 ondersteunde bestanden en mappen getoond; kleine fotovoorbeelden worden alleen op het toestel gemaakt en niet naar de assistent of server verstuurd. Selecteer één tekstbestand of foto en verstuur daarna je vraag. Met Wis verwijder je alleen de gekozen bijlage uit het invoerveld; het oorspronkelijke bestand blijft bestaan.
- Vraag bijvoorbeeld: ‘Maak een bestand boodschappen.txt met mijn boodschappenlijst’. Bij een geldig resultaat verschijnt Opslaan op SD onder het antwoord.
- Downloads staan in ClassicChat/downloads op de SD-kaart. Een ontbrekende kaart geeft een melding; de app valt voor downloads niet terug op interne opslag. De bestandsnaam bevat een gespreks-ID en berichtnummer om botsingen te voorkomen. Een bestaand bestand met andere inhoud wordt niet overschreven.
- Downloadbare bestanden blijven beschikbaar na heropenen van de chat. Bij het verwijderen van een chat blijven reeds gedownloade bestanden en tekstexports bestaan. Verwijder die afzonderlijk via de bestandsbeheerder wanneer je ruimte wilt vrijmaken.

## Opslag en limieten

| Onderdeel | Gedrag |
|---|---|
| Tekstbijlage | UTF-8 txt, md, csv of json; maximaal 12 KiB |
| Foto | JPEG of PNG uit gedeelde opslag/SD; bron maximaal 12 MiB en 16 megapixels |
| Verzonden foto | In het geheugen verkleind tot maximaal 1280 pixels aan de langste zijde, JPEG kwaliteit 75, maximaal 512 KiB |
| Gegenereerd bestand | Eén UTF-8-bestand per antwoord, maximaal 16 KiB; txt, md, csv of json |
| Gesprek | Maximaal acht aangeboden bestanden; bestaande limieten van 40 berichten en 64.000 teksttekens blijven gelden |
| Aanvraag | Maximaal 1 MiB; maximaal 100 seconden in de client en 90 seconden in de relay |
| Antwoordstream | Maximaal 256 KiB in de client; tekst maximaal 16.000 tekens |

De app maakt geen permanente kopie van een verzonden foto of tekstbijlage. De chat onthoudt alleen een bijlagenaam. Bijlagen worden uitsluitend met de geselecteerde aanvraag meegestuurd; voeg ze opnieuw toe voor een vervolgvraag die de oorspronkelijke inhoud nodig heeft. Voor een aangeboden download wordt de kleine tekstinhoud wel in het lokale chatbestand bewaard, zodat de opslagknop na heropenen blijft werken. Een opgeslagen download is daarnaast een afzonderlijk SD-bestand.

De relay schrijft uploads of gegenereerde bestanden niet naar schijf. Ze bestaan alleen tijdens de aanvraag in het geheugen. Er is daarom geen tijdelijke download-URL of opruimtaak op de VPS nodig. De oorspronkelijke bestanden op je SD-kaart blijven jouw verantwoordelijkheid. De API-aanvraag gebruikt store:false; de OpenAI-bewaarvoorwaarden blijven van toepassing. Er is geen blijvend server-chatarchief toegevoegd.

De fotovoorbeelden blijven in een begrensde geheugencache van maximaal 32 items. Ze worden niet naar de SD-kaart of interne opslag geschreven. Tijdens streaming verdwijnt het invoerveld tijdelijk en blijft een compacte stopknop zichtbaar. Na de schermopmaak volgt het beeld het nieuwe antwoord. Wie zelf omhoog bladert, kan met de pijl omlaag het volgen hervatten.

## Protocol en beveiliging

Oude clients kunnen dezelfde /v1/chat-endpoint blijven gebruiken en ontvangen één JSON-antwoord. De nieuwe client vraagt stream:true aan en ontvangt newline-delimited JSON: start, delta, status, done of error. Unicode wordt pas na ontvangst van een volledige JSON-regel gedecodeerd. De UI wordt maximaal ongeveer zeven keer per seconde bijgewerkt, zodat het toetsenbord en annuleren responsief blijven.

Alleen een expliciet server-geconfigureerd model en gereedschappen worden gebruikt. De serverprompt kent BlackBerry Classic, BB10, het kleine scherm, de SD-opslag en de beperking dat de app geen opdrachten uitvoert. Het create_text_file-gereedschap biedt uitsluitend tekstinhoud aan; de client bepaalt het opslagpad en schrijft pas na de tik van de gebruiker. Modelnamen, systeeminstructies, tools, upload-URL’s en willekeurige serverpaden worden niet van de client overgenomen. TLS 1.2, CA- en hostnaamcontrole en het aparte toestel-token blijven behouden.

Het eindbericht vervangt de voorlopige tekst door de definitieve tekst met gecontroleerde bronverwijzingen. Een stream zonder eindbericht, een onvolledig modelantwoord of een ongeldig bestand wordt als fout behandeld. Het sluiten van de verbinding annuleert de upstream-aanvraag.

## Controles

Twintig geautomatiseerde servertests slagen, inclusief gesplitste UTF-8/SSE-fragmenten, zichtbare tekst vóór voltooiing, onvolledige streams, annuleren, authenticatie, bestandsnamen, groottelimieten en beide bijlagetypen. De native ARM-build slaagt en de nieuwe app laadt zonder QML-fouten op het toestel.

Live op de VPS zijn tekststreaming, een gegenereerd tekstbestand en het teruglezen van een testcode uit een tekstbijlage bevestigd. Een grotere JPEG-testafbeelding met testtekst is correct herkend. Een eerdere kleine vlakke kleurafbeelding werd inhoudelijk verkeerd benoemd, ook bij een rechtstreekse provider-aanvraag: beeldinterpretatie blijft modeluitvoer en moet gecontroleerd worden.

De echte native browser- en downloadcode is met een wegwerp-testchat op het toestel uitgevoerd. Het opgeslagen bestand kwam byte voor byte overeen. De gebruiker heeft SD-mapnavigatie, de gecorrigeerde opslagknop en zichtbare streaming bevestigd. Daarna zijn de bestandskiezer naar een raster en de ruimte/automatische scroll tijdens streaming verder verfijnd; die laatste visuele aanpassing wordt nog op het toestel nagekeken. Foto’s met EXIF-rotatie en volle/plotseling verwijderde SD-kaarten zijn nog niet afzonderlijk doorlopen.

## Officiële API-bronnen

- https://developers.openai.com/api/docs/guides/streaming-responses
- https://developers.openai.com/api/docs/guides/images-vision
- https://developers.openai.com/api/docs/guides/function-calling
