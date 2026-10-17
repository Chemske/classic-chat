# Classic Chat — donker ontwerp en internetzoeken, 17 oktober 2026

De bijgewerkte native app is geïnstalleerd zonder debug token. Programma en QML-bestanden zijn na installatie uitgelezen en komen exact overeen. Het hoofdscherm opent zonder QML-laadfouten.

De app gebruikt nu overal het native donkere thema, met antraciete achtergrond, afzonderlijke berichtvlakken, gedempte groene accenten, vaste schermmarges, expliciete berichtbreedte voor regelafbreking en een compacte invoerbalk. Gesprekken en instellingen krijgen dezelfde achtergrond en meer tussenruimte. Gebruikers- en assistenttekst zijn visueel gescheiden.

De bestaande gpt-4.1-mini op de VPS heeft de OpenAI-web_search-tool beschikbaar en kiest die bij actuele vragen. De instructies vragen om de plaats als een weervraag die mist en geven expliciet de Belgische datum mee, ook rond middernacht. Er zijn twee echte betaalde zoektests gedaan; de laatste gebruikte aantoonbaar web_search en gaf een weerantwoord voor zaterdag 17 oktober 2026 met een bronlink. Dertien servertests slagen, inclusief bronverwijzingen en toegangsbeveiliging.

Citaties worden als aanklikbare inline nummers weergegeven. De client accepteert daarvoor alleen HTTP(S)-links en ontsnapt alle overige model- en gebruikersinhoud als gewone tekst. Het gespreksarchief op SD blijft leesbare tekst met bronlinks. De telefoon ontvangt geen OpenAI API-sleutel. Zoeken kan extra API-kosten meebrengen.

Actueel tijdelijk serveradres: https://pts-bracelets-beginners-organizational.trycloudflare.com
Het nieuwe adres is privé in de app ingesteld; hetzelfde toestel-token blijft geldig. De Mac hoeft niet aan te staan. Een tunnelherstart kan dit tijdelijke adres wijzigen.

De vorige werkende versie blijft lokaal als herstelkopie bewaard. Er is bij deze update geen firmware geflasht en geen andere app verwijderd. De visuele controle door de gebruiker, bronlinks openen in de browser en een volledige toestelherstart zijn nog niet bevestigd.

Gebruikte officiële documentatie: https://developers.openai.com/api/docs/guides/tools-web-search
