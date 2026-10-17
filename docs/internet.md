# Internetverbinding

De native app gebruikt het vaste HTTPS-adres https://classicchat.130-61-37-71.sslip.io. De Oracle VPS verzorgt de relay en Caddy verzorgt HTTPS. Wifi en mobiele data kunnen dezelfde URL gebruiken; de Mac hoeft niet aan te staan. Een afzonderlijke test met wifi uit is na deze update nog niet uitgevoerd.

Gebruik versie 0.1.1 of nieuwer voor de TLS 1.2-verbinding. Het serveradres en het aparte toestel-token staan in de appinstellingen. De OpenAI API-sleutel hoort uitsluitend op de server. Start-Classic-Chat.command opent het publieke adres uit het lokale Internetadres.txt; het start geen lokale server.

Het oude tijdelijke Cloudflare-adres is buiten gebruik. Zie VPS-handleiding.md voor installatie, poorten, certificaten, herstarttests en de afhankelijkheid van sslip.io en het publieke IP. Voor lokale relay-ontwikkeling blijft server/.env.example beschikbaar.
