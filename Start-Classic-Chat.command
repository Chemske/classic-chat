#!/bin/zsh
set -eu
address_file="${0:A:h}/Internetadres.txt"
if [[ ! -f "$address_file" ]]; then
  print 'Het actuele VPS-adres ontbreekt. Zie docs/VPS-handleiding.md.'
  exit 1
fi
server_url="$(/usr/bin/awk '/^https:\/\/[A-Za-z0-9.-]+\/?$/ { print; exit }' "$address_file")"
if [[ -z "$server_url" ]]; then
  print 'Geen geldig actueel VPS-adres gevonden. Zie docs/VPS-handleiding.md.'
  exit 1
fi
exec /usr/bin/open "$server_url"
