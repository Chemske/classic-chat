#!/usr/bin/env python3
"""Download pinned official ARM64 runtimes and verify published SHA256."""
import pathlib,urllib.request,hashlib,json,argparse
p=argparse.ArgumentParser(description=__doc__)
p.add_argument("--with-tunnel",action="store_true",help="Also download the optional historical Cloudflare development transport")
a=p.parse_args()
artifacts=[
    ('node-v24.22.0-linux-arm64.tar.xz','https://nodejs.org/dist/v24.22.0/node-v24.22.0-linux-arm64.tar.xz','fef00bb4d36e1c50104a9775a10359beee98f162b4104dd12472c75065c6a143'),
    ('cloudflared-linux-arm64','https://github.com/cloudflare/cloudflared/releases/download/2026.10.1/cloudflared-linux-arm64','2c65dfabf616c40efc7545d107d7a57ad5e21377e2964b7e88af19a238074a46')
]
if not a.with_tunnel: artifacts=[entry for entry in artifacts if not entry[0].startswith("cloudflared")]
result=[]
for name,url,expected in artifacts:
    h=hashlib.sha256();dest=pathlib.Path(name)
    if dest.exists():raise SystemExit('Refusing to overwrite '+name)
    with urllib.request.urlopen(urllib.request.Request(url,headers={'User-Agent':'ClassicChat-deployment'}),timeout=30) as source, dest.open('xb') as output:
        while True:
            block=source.read(1024*1024)
            if not block:break
            output.write(block);h.update(block)
    if h.hexdigest()!=expected:raise SystemExit('SHA256 mismatch: '+name)
    result.append({'file':name,'url':url,'sha256':expected,'bytes':dest.stat().st_size})
    print('Verified:',name,flush=True)
pathlib.Path('runtime-provenance.json').write_text(json.dumps(result,indent=2)+'\n')
