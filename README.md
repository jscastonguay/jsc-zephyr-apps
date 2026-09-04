# jsc-zephyr-apps

## Références

Voir la série de tuto Zephyr de digikey:
- https://www.youtube.com/watch?v=mTJ_vKlMS_4&list=PLEBQazB0HUyTmK2zdwhaf8bLwuEaDH-52

Pour un exemple d'un projet de type workspace:
- https://github.com/zephyrproject-rtos/example-application/tree/main

Pour Kconfig:
- Practical Zephyr - Kconfig (Part 2): https://interrupt.memfault.com/blog/practical_zephyr_kconfig

Pour zephyr en général (et autres porribilités):
- [Practical Zephyr - Zephyr Basics (Part 1)](https://interrupt.memfault.com/blog/practical_zephyr_basics)
- [Practical Zephyr - Kconfig (Part 2)](https://interrupt.memfault.com/blog/)
- [practical_zephyr_kconfig Practical Zephyr - Devicetree basics (Part 3)](https://interrupt.memfault.com/blog/practical_zephyr_dt)
- [Practical Zephyr - Devicetree semantics (Part 4)](https://interrupt.memfault.com/blog/practical_zephyr_dt_semantics)
- [Practical Zephyr - Devicetree practice (Part 5)](https://interrupt.memfault.com/blog/practical_zephyr_05_dt_practice)
- [Practical Zephyr - West workspaces (Part 6)](https://interrupt.memfault.com/blog/practical_zephyr_west)

Pour le devicetree:

- https://www.devicetree.org/
- https://github.com/devicetree-org/devicetree-specification/releases/tag/v0.4
- https://docs.zephyrproject.org/latest/build/dts/index.html
- https://www.raspberrypi.com/documentation/computers/configuration.html#device-trees-overlays-and-parameters
- Pour la syntaxe:
    - https://docs.zephyrproject.org/latest/build/dts/intro-syntax-structure.html
    - https://docs.zephyrproject.org/latest/build/dts/bindings-syntax.html#type
- Pour la convention de nommage des devicetree:
    - https://docs.kernel.org/devicetree/bindings/dts-coding-style.html

Pour les device drivers:

- https://github.com/ShawnHymel/workshop-zephyr-device-driver

## Commandes

Pour commencer:
```bash
cd ~/zephyrproject
source .venv/bin/activate
```

Pour bâtir une application:
```bash
west build -b nucleo_h753zi
```

Pour bâtir une application mais en rebâtissant le tout:
```bash
west build -b nucleo_h753zi -p always
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
ou pour s'assurer de tout rebâtir:
```bash
west build -b nucleo_h753zi -p always -- -DEXTRA_CONF_FILE=boards/nucleo_h753zi.conf
```

Pour télécharger l'application:
```bash
west flash
```bash
west build -b reel_board samples/basic/blinky -- -DZEPHYR_SCA_VARIANT=dtdoctor
```

## Autres possibilités durant la comstruction

Pour utiliser le `DT doctor`, faire:


## Devicetree

Voir le résultat de l'explication de chatgpt [ici](doc/devicetree.md).

Pour voir le dts résultant après compilation:
```
build/zephyr/zephyr.dts
```