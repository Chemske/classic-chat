#!/bin/bash
set -euo pipefail
test "$(uname -m)" = aarch64
test -s runtime-provenance.json
test ! -e /opt/classicchat
if ! getent passwd classicchat >/dev/null; then
    useradd --system --home-dir /var/lib/classicchat --create-home --shell /usr/sbin/nologin classicchat
fi
install -d -m 755 /opt/classicchat /opt/classicchat/bin /opt/classicchat/server /opt/classicchat/node
install -d -m 750 -o root -g classicchat /etc/classicchat
install -d -m 700 -o classicchat -g classicchat /var/lib/classicchat
tar -xJf node-v24.22.0-linux-arm64.tar.xz --strip-components=1 -C /opt/classicchat/node
chown -R root:root /opt/classicchat/node
install -m 644 server.mjs /opt/classicchat/server/
install -m 644 runtime-provenance.json /opt/classicchat/
install -m 644 classicchat.service /etc/systemd/system/classicchat.service
/opt/classicchat/node/bin/node --check /opt/classicchat/server/server.mjs
systemd-analyze verify /etc/systemd/system/classicchat.service
systemctl daemon-reload
echo 'Installed; service not started until private configuration is transferred.'
