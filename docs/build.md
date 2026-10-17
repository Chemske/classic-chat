# Native build op deze Apple Silicon Mac

De build is geslaagd met:

- Apple Clang 21.0.0, ARMv7 EABI softfp, C++98, PIC, geen exceptions/RTTI.
- BB10 SDK-target 10.3.1.995, echte QNX-, Qt 4- en Cascades-headers en ARM-bibliotheken.
- GNU Binutils 2.45, `arm-unknown-nto-qnx6.5.0eabi`, linkeremulatie `armnto`.
- Lokaal gebouwde host-moc Qt 4.8.7, metacode-revisie 63.
- BlackBerry Tablet OS Elf BAR Packager 1.4.2 uit het oorspronkelijke BBNDK 2.1 Linux-hostpakket.
- Portable Temurin JRE 17.0.20.1 voor alleen de packager. Geen globale Java-installatie of wijziging aan Java-beveiligingsinstellingen.

De opzet is ontleend aan [XIAZY/blackberry10-toolchain](https://github.com/XIAZY/blackberry10-toolchain), commit `0384a8801ffe69162e548d58898d24d8e2a88426`, en aangepast aan deze Mac. De toolchain is geen BlackBerry-certificering van de app. Alleen de appcode en het BAR-bestand worden geleverd; de SDK en ontwikkelgereedschappen hebben hun eigen licenties.

## Bouwscript

`tools/build-native.py` vereist expliciete paden en stopt bij iedere fout. Het bouwt en verpakt uitsluitend; het benadert geen telefoon. Vanaf deze workspace is de uitgevoerde aanroep:

```sh
python3 work/classic-chat/tools/build-native.py \
  --sdk work/bb10-sdk/target_10_3_1_995/qnx6 \
  --override work/bb10-sdk/target_10_3_1_995/target-override \
  --moc work/build-tools/moc/moc \
  --ld work/build-tools/binutils-build/ld/ld-new \
  --java work/build-tools/java/jdk-17.0.20.1+1-jre/Contents/Home/bin/java \
  --packager-libs work/bb10-nativepackager/host/linux/x86/usr/lib
```

Workspace: `<workspace>`.

De traditionele `native/classicchat.pro` staat ook in het project. De geslaagde build hierboven gebruikt het expliciete script, niet qmake/Momentics.

## Herkomstcontroles

Het SDK-archief `bbndk.win32.libraries.10.3.1.995.zip` is via HTTP-range alleen voor de benodigde headers en ARM-bestanden gelezen uit [het gearchiveerde BB-ontwikkelgereedschap](https://archive.org/details/bbdevtools). De uitgepakte ZIP-entry-CRC's zijn gecontroleerd. Het volledige 1,6 GB-archief is niet gedownload of op SHA256 geverifieerd.

De packager-JARs komen uit het gecomprimeerde hostarchief van de [gearchiveerde oorspronkelijke BBNDK-installer](https://web.archive.org/web/20220102083348id_/http://downloads.blackberry.com/upr/developers/downloads/installer-bbndk-2.1.0-linux-1032-201209271809-201209280007.bin). De installer is niet uitgevoerd. Het uitgepakte hostarchief is gecontroleerd tegen de door de toolchain vastgelegde SHA256 `c0ff039ddad3ce4f5e2d962eec7921afd9669e1f285e5a9cf8d4b61b0358860c`.

GNU Binutils 2.45-bronarchief: SHA256 `c50c0e7f9cb188980e2cc97e4537626b1672441815587f1eab69d2a1bfbef5d2`, gecontroleerd. Qt-bron: officiële `qt/qt`, tag v4.8.7, commit `445d29dbb8135944ff3a9283b7622de75d68e747`; alleen de host-moc en bootstrapcode zijn gebouwd. De hostconfiguratie is aangepast voor deze moderne Mac; de target-SDK is niet gepatcht voor deze app.

De JRE-download is tegen de officiële Adoptium-releasechecksum gecontroleerd: `190480874ccceb358cbc840393207f77ac3e63a4c5f8129d0e23e9518b96ad05`.

De packager genereert ontwikkel-identiteiten en een manifest. Dat vormt geen BlackBerry-signature en vervangt geen geldig debug-token.

## Runtime-correctie, 17 oktober 2026

Link ook `bbsystem` en registreer de gebruikte bb.system-QML-types vóór het laden van main.qml. Alleen compileren en verpakken detecteerde de ontbrekende QML-types niet; deze zijn op het echte toestel gecontroleerd.
