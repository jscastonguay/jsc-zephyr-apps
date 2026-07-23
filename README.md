# jsc-zephyr-apps

Voir la série de tuto Zephyr de digikey:
https://www.youtube.com/watch?v=mTJ_vKlMS_4&list=PLEBQazB0HUyTmK2zdwhaf8bLwuEaDH-52

Pour un exemple d'un projet de type workspace:
https://github.com/zephyrproject-rtos/example-application/tree/main


Pour commencer:
```bash
cd ~/zephyrproject
source .venv/bin/activate
```

Pour bâtir une application:
```bash
west build -b nucleo_h753zi
```

Avec menuconfig (kconfig):
```bash
west build -b nucleo_h753zi -t menuconfig
```

Pour comparer les fichiers de configuration de zephry
```bash
diff build/zephyr/.config build/zephyr/.config.old 
```

Par la suite, modifier le fichier `prj.conf`.

Si le fichier de configuration kconfig est créé à un autre endroit, par exemple `boards/nucleo_h753zi.conf`, faire le build de la façon suivante:
```bash
west build -b nucleo_h753zi -- -DEXTRA_CONF_FILE=boards/nucleo_h753zi.conf
```
