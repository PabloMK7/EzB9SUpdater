# EzB9SUpdater - Easy Boot9Strap Updater
A 3DS tool to update B9S to the latest version (currently v1.4) without the need of a computer or SD card reader.

This tool uses [SafeB9SInstaller](https://github.com/d0k3/SafeB9SInstaller) and [Boot9Strap](https://github.com/SciresM/boot9strap).

## Usage
1. Install the EzB9SUpdater cia file from the [Releases](https://github.com/PabloMK7/EzB9SUpdater/releases/latest) page. Alternatively use [this QR code](https://user-images.githubusercontent.com/10946643/170048287-b54c9e0f-374b-4117-a567-2a30c23b5e29.png) with FBI remote install.
2. Launch the EzB9SUpdtaer app from the Home Menu.
3. Follow the instructions in the app. At some point, you will be asked to press and hold the START button to reboot into SafeB9SInstaller. It is important that you keep holding the button until you see the SafeB9SInstaller screen. Otherwise, the console will just reboot into EzB9SUpdater again and no update will be performed.
4. Once you finish the B9S update, you can exit the app and uninstall it from FBI.
5. In order to check if you updated B9S from 1.3 to 1.4 do the following steps:
    1. Power off your console.
    2. Press and hold the following button combination: `X + START + SELECT`.
    3. Without releasing those buttons, power on your device.
    4. Your notification LED should lit up for a second ([status codes](https://github.com/PabloMK7/boot9strap/tree/patch-1#led-status-codes)). If it doesn't, the update wasn't installed properly.

## Credits
- [@Rinnegatamante](https://github.com/Rinnegatamante) : Zip extracting code from [lpp-3ds](https://github.com/Rinnegatamante/lpp-3ds).
- [@Krzysztof Gabis](https://github.com/kgabis) : JSON parsing utility [parson](https://github.com/kgabis/parson).
- [@Steveice10](https://github.com/Steveice10) : buildtools
- [@Nanquitas](https://github.com/Nanquitas) : BootNTRSelector, from which this app is based.
- [@Kartik](https://github.com/hax0kartik) : Little help with reboot code.
