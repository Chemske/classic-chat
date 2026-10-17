# Verificatie — 17 oktober 2026

Geslaagd op het echte toestel: installatie zonder debug token, uitleesvergelijking van executable en drie QML-bestanden, nul QML-laadfouten, draaiend proces, werkende chat door gebruiker bevestigd en SD-chatbestand met gebruikersbericht plus antwoord teruggelezen. Serveradres en toestel-token zijn ingesteld; het eenmalige setupbestand is verwijderd. Geen OpenAI API-sleutel staat in de appconfiguratie.

Het BAR-pakket is opnieuw op ZIP-CRC, alle manifest-SHA512-digests en ARM ELF32 gecontroleerd. Zie build-result.json, ../build/native-verification.json en huidige-status.md. De installer is op Python-syntaxis gecontroleerd; de handmatige installatie en uitleesvergelijking zijn op dit toestel uitgevoerd.

Historisch slaagden twaalf serverintegratietests met een nagebootste provider en echte lokale/publieke/VPS-API-tests. De servercode is bij deze clientfix niet veranderd.

Nog niet apart getest: TXT-export, chat heropenen/verwijderen via de UI, ontbrekende/volle SD-kaart, mobiel internet en een volledige toestelherstart na de werkende installatie.
