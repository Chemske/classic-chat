#!/usr/bin/env python3
"""Install only Classic Chat on an already prepared BB10 phone. Never flashes firmware.
Requires a verified SSH host key and verified root access.
"""
import argparse, ipaddress, pathlib, shlex, subprocess, uuid, zipfile, re
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--ip',required=True)
p.add_argument('--identity',required=True,type=pathlib.Path)
p.add_argument('--known-hosts',type=pathlib.Path,help='Pinned known_hosts file for the device')
p.add_argument('--legacy-bb10',action='store_true',help='Allow the old RSA SSH algorithms only for this device connection')
p.add_argument('--via-devuser-root-helper',action='store_true',help='Use the already verified /base/bin/__root helper through development SSH')
p.add_argument('--bar',type=pathlib.Path,default=pathlib.Path(__file__).resolve().parents[1]/'build/ClassicChat-0.1.1-unsigned.bar')
a=p.parse_args();ip=str(ipaddress.IPv4Address(a.ip));key=a.identity.expanduser().resolve()
if not key.is_file():p.error('De eigen SSH-private sleutel ontbreekt.')
with zipfile.ZipFile(a.bar) as z:
    manifest=z.read('META-INF/MANIFEST.MF').decode()
    if '\nPackage-Name: com.classicchat.client\n' not in manifest or '\nPackage-Architecture: armle-v7\n' not in manifest:
        p.error('Dit is niet het verwachte Classic Chat ARM-pakket.')
    if z.read('native/classicchat')[:6]!=b'\x7fELF\x01\x01':p.error('Geen ARM ELF32-applicatie.')
    expected_assets={name:z.read(name) for name in ['native/classicchat','native/assets/main.qml','native/assets/SettingsPage.qml','native/assets/HistoryPage.qml','native/assets/certificates/isrgrootx1.pem']}
user='devuser' if a.via_devuser_root_helper else 'root'
ssh=['ssh','-o','BatchMode=yes','-o','ConnectTimeout=10','-o','StrictHostKeyChecking=yes','-i',str(key),user+'@'+ip]
if a.known_hosts:
    known=a.known_hosts.expanduser().resolve()
    if not known.is_file():p.error('Het gepinde SSH-hostbestand ontbreekt.')
    ssh[1:1]=['-o','UserKnownHostsFile='+str(known),'-o','IdentitiesOnly=yes']
if a.legacy_bb10:
    ssh[1:1]=['-o','HostkeyAlgorithms=+ssh-rsa','-o','PubkeyAcceptedAlgorithms=+ssh-rsa',
              '-o','KexAlgorithms=diffie-hellman-group14-sha1','-o','Ciphers=aes128-ctr','-o','MACs=hmac-sha1']
def command(script):
    if a.via_devuser_root_helper:
        return [*ssh,"/base/bin/__root <<'CLASSICCHAT_ROOT_EOF'\n"+script+"\nexit\nCLASSICCHAT_ROOT_EOF"]
    return [*ssh,'/bin/ksh -c '+shlex.quote(script)]
probe=subprocess.run(command('pidin -p $$ -F "CLASSICCHAT_EUID=%W"; uname -s; test -r /base/scripts/sudtools.sh && echo CLASSICCHAT_INSTALLER_PRESENT'),capture_output=True,text=True,timeout=20,check=True)
lines=probe.stdout.splitlines()
if not re.search(r'^CLASSICCHAT_EUID=\s*0\s*$',probe.stdout,re.M) or 'QNX' not in lines or 'CLASSICCHAT_INSTALLER_PRESENT' not in lines:raise SystemExit('Geen bevestigde QNX-rootverbinding met pakketinstaller; gestopt.')
# Multiple SUD workers can process the same transaction and remove each other's staging tree.
workers=subprocess.run(command('pidin -F "%a %A"'),capture_output=True,text=True,timeout=20,check=True)
worker_count=len(re.findall(r'^\s*\d+\s+/base/usr/bin/python3\.2 /base/usr/sbin/sud\.py(?:\s|$)',workers.stdout,re.M))
if worker_count>1:raise SystemExit('Meerdere pakketinstallers actief; gestopt voordat een installatie wordt aangemaakt.')
if worker_count==0:
    subprocess.run(command('PATH=/proc/boot:/base/bin:/base/usr/bin:/bin:/usr/bin:/sbin:/usr/sbin; export PATH; /base/usr/sbin/sud.sh </dev/null >/tmp/classicchat-sud-installer.log 2>&1 &'),capture_output=True,timeout=20,check=True)
remote='/tmp/classicchat-'+uuid.uuid4().hex+'.bar'
with a.bar.open('rb') as source:
    subprocess.run([*ssh,'umask 022; cat > '+remote+'; chmod 644 '+remote],stdin=source,timeout=60,check=True)
print('Alleen Classic Chat is overgedragen. Installatie kan enkele minuten duren.',flush=True)
result=subprocess.run(command('BASEFS=/base; export BASEFS; . /base/scripts/sudtools.sh; sud_install_package_2 -T 600 -D -p '+remote),capture_output=True,text=True,timeout=650)
output=result.stdout+result.stderr
if not any(s in output.lower() for s in ['sucessfully installed','successfully installed']):
    print(output);raise SystemExit('Geen bevestigde installatie. Het eigen tijdelijke BAR-bestand blijft op '+remote+'. Geen automatische retry.')
for name,expected in expected_assets.items():
    actual=subprocess.run([*ssh,'cat /apps/com.classicchat.client.testDev_chat_client793aa0dc/'+name],capture_output=True,timeout=20,check=True).stdout
    if actual!=expected:raise SystemExit('Installatie gemeld, maar uitleescontrole wijkt af voor '+name+'. Gestopt.')
subprocess.run(command('rm -f '+remote),timeout=20,check=True)
print('Classic Chat is geïnstalleerd; programma en schermbestanden zijn teruggelezen en komen exact overeen. Start de app en test HTTPS en SD-opslag op het toestel.')
