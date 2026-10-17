#!/usr/bin/env python3
"""Build an unsigned ARMv7 BAR with explicit local tool paths; never deploys."""
from pathlib import Path
import argparse, subprocess, shutil, hashlib
p=argparse.ArgumentParser()
for key in ('sdk','override','moc','ld','java','packager-libs'):
    p.add_argument('--'+key,required=True,type=Path)
a=p.parse_args();root=Path(__file__).resolve().parents[1];native=root/'native';out=root/'build/native';out.mkdir(parents=True,exist_ok=True)
def run(args):subprocess.run([str(x) for x in args],check=True)
resource=subprocess.check_output(['clang','-print-resource-dir'],text=True).strip()
flags=['--target=armv7-none-eabi','-fPIC','-marm','-mcpu=cortex-a9','-mfpu=vfpv3-d16','-mfloat-abi=softfp','-mno-unaligned-access','-D__ARM__','-D__QNX__','-D__QNXNTO__','-D__unix__','-D__unix','-D__LITTLEENDIAN__','-D_QNX_SOURCE','-std=gnu++98','-fno-exceptions','-fno-rtti','-DQT_NO_EXCEPTIONS','-nostdinc','-fno-builtin','-Wno-pragma-pack','-Wno-unknown-attributes']
for x in [Path(resource)/'include',a.override/'usr/include',*[a.sdk/x for x in ('usr/include/cpp','usr/include/cpp/c','usr/include','usr/include/qt4','usr/include/qt4/QtCore','usr/include/qt4/QtDeclarative','usr/include/qt4/QtNetwork')]]:flags+=['-isystem',str(x.resolve())]
flags+=['-I'+str(native/'src')]
run([a.moc,native/'src/ChatController.hpp','-o',out/'moc_ChatController.cpp'])
for f in [native/'src/main.cpp',native/'src/ChatController.cpp',out/'moc_ChatController.cpp']:
    run(['clang++',*flags,'-c',f,'-o',out/(f.stem+'.o')])
l=a.sdk/'armle-v7/lib';u=a.sdk/'armle-v7/usr/lib';qt=u/'qt4/lib'
cmd=[a.ld,'-m','armnto','--dynamic-linker','/usr/lib/ldqnx.so.2','--hash-style=sysv','-z','noexecstack','-o',out/'classicchat',l/'crt1.o',l/'crti.o',*[out/x for x in ['main.o','ChatController.o','moc_ChatController.o']]]
for directory in [l,l/'gcc/4.6.3',u,qt]:cmd+=['-L'+str(directory.resolve()),'-rpath-link',directory]
cmd+=['-l'+x for x in ['bbdata','bbsystem','bbcascades','QtDeclarative','QtScript','QtSvg','QtSql','sqlite3','z','QtXmlPatterns','QtGui','QtNetwork','socket','QtCore','cpp','m','bps','c']]+[l/'crtn.o'];run(cmd)
stage=native/'arm/o.le-v7/classicchat';stage.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(out/'classicchat',stage)
bar=root/'build/ClassicChat-0.1.0-unsigned.bar'
exports=['--add-exports=java.xml/com.sun.org.apache.xerces.internal.'+part+'=ALL-UNNAMED' for part in ('parsers','xni.parser','xni')]
subprocess.run([str(a.java.resolve()),*exports,'-cp',str(a.packager_libs.resolve())+'/*','com.qnx.bbt.nativepackager.BarNativePackager','-devMode','-package',str(bar),'bar-descriptor.xml'],cwd=native,check=True)
print('Unsigned BAR:',bar);print('SHA256:',hashlib.sha256(bar.read_bytes()).hexdigest())
