"""Render the project's simple geometric app icon without external dependencies."""
from pathlib import Path
import struct, zlib
N=114;S=4
pixels=bytearray()
def rounded(x,y,l,t,r,b,radius):
    cx=max(l+radius,min(r-radius,x));cy=max(t+radius,min(b-radius,y))
    return l<=x<=r and t<=y<=b and (x-cx)**2+(y-cy)**2<=radius**2
for y in range(N):
    pixels.append(0)
    for x in range(N):
        rgb=[0,0,0]
        for sy in range(S):
            for sx in range(S):
                xx=x+(sx+.5)/S;yy=y+(sy+.5)/S
                color=(24,39,52)
                bubble=rounded(xx,yy,21,24,93,79,12) or (36<=xx<=51 and 76<=yy<=91 and yy<=112-xx)
                if bubble:color=(243,247,250)
                line=(rounded(xx,yy,35,40,79,45,2.5) or rounded(xx,yy,35,55,68,60,2.5))
                if line:color=(17,131,170)
                rgb=[a+b for a,b in zip(rgb,color)]
        pixels.extend(round(v/(S*S)) for v in rgb)
def chunk(kind,data):return struct.pack('>I',len(data))+kind+data+struct.pack('>I',zlib.crc32(kind+data)&0xffffffff)
png=b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('>IIBBBBB',N,N,8,2,0,0,0))+chunk(b'IDAT',zlib.compress(pixels,9))+chunk(b'IEND',b'')
target=Path(__file__).resolve().parents[1]/'native/assets/icon.png';target.write_bytes(png)
