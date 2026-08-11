Oui. Le plus important est de **cesser de voir le Device Tree comme une configuration obscure à copier-coller**. Pour un développeur Zephyr, il faut plutôt le voir comme une **description statique du matériel qui sert d'interface entre le matériel, les drivers et le code C**.

Je vais donc te donner une méthode que tu pourras appliquer à **n'importe quel périphérique** : GPIO, UART, I²C, SPI, ADC, PWM, interrupteur, capteur, etc.

Les liens vers la documentation officielle que je donne à la fin de chaque section servent surtout de **références pour retrouver les informations concrètes** : noms de `compatible`, propriétés disponibles, pins, contrôleurs, etc. La logique et la méthode sont expliquées ici.

---

# 1. La vision globale

Le modèle mental que je te recommande est celui-ci :

```text
                    MATÉRIEL RÉEL
                         │
                         │ datasheet / schematic
                         ▼
                 ┌─────────────────┐
                 │  SoC / périph.  │
                 │                 │
                 │ UART / GPIO /   │
                 │ SPI / I2C / ADC │
                 └────────┬────────┘
                          │
                          │ décrit par
                          ▼
                 ┌─────────────────┐
                 │   DEVICETREE    │
                 │                 │
                 │ nodes           │
                 │ properties      │
                 │ phandles        │
                 │ pinctrl         │
                 └────────┬────────┘
                          │
                          │ associé à
                          ▼
                 ┌─────────────────┐
                 │    BINDING      │
                 │     YAML        │
                 │                 │
                 │ compatible      │
                 │ propriétés      │
                 │ types           │
                 │ contraintes     │
                 └────────┬────────┘
                          │
                          │ génère
                          ▼
                 ┌─────────────────┐
                 │ DT generated    │
                 │ macros           │
                 └────────┬────────┘
                          │
                          ▼
                 ┌─────────────────┐
                 │ Driver Zephyr   │
                 │                 │
                 │ DEVICE_DT_*     │
                 │ DT_*             │
                 └────────┬────────┘
                          │
                          ▼
                 ┌─────────────────┐
                 │ Application C   │
                 └─────────────────┘
```

C'est **le lien fondamental** entre les éléments.

Zephyr utilise le Device Tree à la fois pour décrire le matériel au modèle de drivers et pour fournir sa configuration initiale. ([Zephyr Project Documentation][1])

---

# 2. La première chose à comprendre : le Device Tree ne "fait" rien

C'est une distinction extrêmement importante.

Supposons que tu écrives :

```dts
&uart3 {
    status = "okay";
    current-speed = <115200>;
};
```

Tu pourrais avoir l'impression que :

> « Je configure UART3 à 115200 bauds. »

Ce n'est pas exactement ça.

Le Device Tree dit plutôt :

> « Dans le matériel que je décris, UART3 existe, il doit être utilisé et sa configuration initiale est 115200 bauds. »

Ensuite, **le driver UART de Zephyr lit cette information**.

Donc :

```text
DTS
 │
 │ données statiques
 ▼
Device Tree compiler
 │
 ▼
macros C
 │
 ▼
driver
 │
 ▼
initialisation matérielle
```

Le Device Tree est donc essentiellement une **source de données de configuration connue à la compilation**.

C'est une différence importante avec une configuration runtime.

Zephyr ne charge pas un arbre Device Tree binaire à l'exécution comme Linux ; les informations du DT sont transformées en macros C utilisées à la compilation. ([Zephyr Project Documentation][2])

---

# 3. Les quatre concepts qu'il faut absolument maîtriser

Si tu maîtrises :

1. **node**
2. **property**
3. **compatible**
4. **binding**

tu as déjà compris l'essentiel.

Ensuite viennent :

5. phandle
6. specifier
7. pinctrl
8. overlay
9. aliases
10. chosen

---

# 4. Le node

Un **node** représente quelque chose dans le matériel.

Par exemple :

```dts
uart3: serial@40004800 {
    compatible = "st,stm32-uart";
    reg = <0x40004800 0x400>;
    status = "okay";
};
```

Ce node représente un périphérique UART.

Il possède :

* un nom : `serial`
* une adresse : `@40004800`
* éventuellement un label : `uart3`
* des propriétés.

Conceptuellement :

```text
uart3
 │
 ├── compatible
 ├── reg
 ├── status
 ├── interrupts
 ├── pinctrl-0
 └── ...
```

---

# 5. Le label n'est pas le nom du périphérique

C'est une source classique de confusion.

Dans :

```dts
uart3: serial@40004800 {
```

il y a **deux choses différentes** :

```text
uart3                ← label
serial@40004800      ← nom du node
```

Le label sert principalement à pouvoir référencer le node :

```dts
&uart3 {
    status = "okay";
};
```

Dans ton code C :

```c
#define UART_NODE DT_NODELABEL(uart3)
```

Le label est donc une sorte de **nom symbolique du node**.

Zephyr permet aussi d'obtenir un node par chemin, alias, instance de `compatible` ou `/chosen`. ([Zephyr Project Documentation][2])

---

# 6. Les properties

Les propriétés décrivent le node.

Exemple :

```dts
uart3: serial@40004800 {
    compatible = "st,stm32-uart";
    reg = <0x40004800 0x400>;
    current-speed = <115200>;
    status = "okay";
};
```

Ici :

| Property        | Signification                  |
| --------------- | ------------------------------ |
| `compatible`    | quel type de matériel est-ce ? |
| `reg`           | où se trouve le périphérique ? |
| `current-speed` | vitesse de communication       |
| `status`        | est-il utilisé ?               |

Mais attention :

**tu ne peux pas inventer arbitrairement les propriétés.**

C'est précisément là qu'intervient le **binding**.

---

# 7. Le binding : le contrat du node

C'est probablement **le concept le plus important pour créer correctement un overlay**.

Le binding est généralement un fichier :

```text
*.yaml
```

Par exemple, conceptuellement :

```yaml
compatible: "mon-vendeur,mon-peripherique"

properties:
  foo:
    type: int
    required: true

  bar:
    type: boolean
```

Le binding dit :

> « Si un node possède ce `compatible`, voici quelles propriétés il peut avoir et quel est leur type. »

Donc :

```text
                    compatible
                       │
                       ▼
Node ─────────────► Binding YAML
 │                     │
 │                     ├── propriétés valides
 │                     ├── types
 │                     ├── propriétés obligatoires
 │                     ├── valeurs permises
 │                     └── sémantique
```

Zephyr associe les nodes à leurs bindings principalement par la propriété `compatible`. Le binding sert ensuite notamment à valider le contenu du node et à générer les macros nécessaires. ([Zephyr Project Documentation][3])

---

# 8. `compatible` est donc le pivot

Prenons :

```dts
sensor0: sensor@48 {
    compatible = "vendor,foo123";
    reg = <0x48>;
    status = "okay";
};
```

Le système cherche un binding correspondant à :

```text
vendor,foo123
```

Par convention, tu trouveras souvent quelque chose comme :

```text
dts/bindings/.../vendor,foo123.yaml
```

Le binding pourrait définir :

```yaml
compatible: "vendor,foo123"

properties:
  reg:
    ...
    
  interrupt-gpios:
    type: phandle-array
```

Tu dois donc adopter cette règle :

> **Avant d'écrire un node pour un périphérique, trouve d'abord son binding.**

C'est l'une des étapes les plus importantes de la méthode que je vais te donner.

La documentation officielle fournit également un index des bindings disponibles. ([Zephyr Project Documentation][4])

---

# 9. Mais il y a une subtilité : le `compatible` n'identifie pas toujours un périphérique externe

Par exemple :

```dts
&spi2 {
    status = "okay";

    sensor@1 {
        compatible = "vendor,sensor";
        reg = <1>;
    };
};
```

Ici :

```text
spi2
 │
 └── sensor@1
```

`spi2` est le **contrôleur SPI**.

`sensor@1` est le **périphérique connecté au bus SPI**.

C'est extrêmement important.

Le Device Tree décrit donc également les **relations matérielles**.

---

# 10. Le Device Tree est un arbre

Imagine ton MCU :

```text
/
└── soc
    ├── gpioa
    ├── gpiob
    ├── uart1
    ├── uart2
    ├── spi1
    │
    └── i2c1
         │
         ├── sensor@48
         └── eeprom@50
```

Cela permet de représenter naturellement :

```text
MCU
 │
 ├── UART
 │
 ├── GPIO
 │
 └── I2C
      │
      ├── EEPROM
      └── SENSOR
```

La hiérarchie n'est donc pas décorative.

Elle permet de représenter les **bus et leurs périphériques enfants**.

---

# 11. `reg` : attention à son sens

`reg` est une propriété très importante.

Pour un périphérique mémoire :

```dts
reg = <0x40004800 0x400>;
```

peut représenter :

```text
adresse = 0x40004800
taille  = 0x400
```

Pour un périphérique I²C :

```dts
sensor@48 {
    reg = <0x48>;
};
```

`reg` signifie plutôt :

```text
adresse I²C = 0x48
```

Pourquoi ?

Parce que **la signification de `reg` dépend du bus et de son binding**.

Donc ne mémorise pas :

> `reg` = adresse mémoire.

Mémorise plutôt :

> `reg` = identifiant/adresse du node sur son bus, dont l'interprétation dépend du contexte.

La documentation Zephyr décrit également `reg` séparément parce que son accès en C possède des macros particulières. ([Zephyr Project Documentation][2])

---

# 12. Le `status`

Tu rencontreras énormément :

```dts
status = "okay";
```

ou :

```dts
status = "disabled";
```

C'est très simple conceptuellement :

```text
disabled
    │
    └── matériel décrit mais inutilisé

okay
    │
    └── matériel disponible/utilisable
```

Les fichiers `.dtsi` des SoC décrivent souvent énormément de périphériques, mais les laissent désactivés.

Ton board ou ton overlay les active :

```dts
&spi2 {
    status = "okay";
};
```

C'est exactement le mécanisme utilisé par les devicetrees de cartes Zephyr. ([Zephyr Project Documentation][5])

---

# 13. Pourquoi les overlays existent

Supposons que Zephyr fournisse déjà :

```dts
spi2: spi@40003800 {
    ...
    status = "disabled";
};
```

Tu ne veux pas modifier Zephyr.

Tu crées :

```text
app.overlay
```

et tu écris :

```dts
&spi2 {
    status = "okay";
};
```

Cela signifie :

> « Dans le Device Tree final de **mon application**, modifie le node `spi2`. »

C'est exactement ce qu'est un overlay :

```text
Device Tree du SoC
       +
Device Tree de la carte
       +
application.overlay
       ↓
FINAL zephyr.dts
```

Le fichier final est l'un des outils les plus utiles pour comprendre ce qui s'est réellement passé. Zephyr recommande notamment d'examiner `build/zephyr/zephyr.dts`. ([Zephyr Project Documentation][6])

---

# 14. Première règle d'or pour les overlays

Lorsque tu veux utiliser un périphérique :

**ne commence pas par écrire l'overlay.**

Commence par trouver **le node existant** dans le Device Tree.

Par exemple :

```text
Je veux UART3
      ↓
Quel node représente UART3 ?
      ↓
uart3: serial@...
      ↓
Existe-t-il déjà ?
      ↓
Quel est son binding ?
      ↓
Quelles propriétés possède-t-il ?
      ↓
Quels pins sont utilisés ?
      ↓
Overlay
```

---

# 15. La méthode universelle

Voici la méthode que je te recommande de suivre systématiquement.

## Étape 1 — Identifier le matériel

À partir :

* du schéma électrique ;
* de la datasheet ;
* du manuel du MCU ;
* de la documentation de la carte.

Tu détermines :

```text
Périphérique :
    SPI2

Pins :
    SCK  = ...
    MISO = ...
    MOSI = ...
    CS   = ...

Interrupt :
    ...

Adresse :
    ...

Alimentation :
    ...
```

Cette information vient du **hardware**, pas de Zephyr.

---

# 16. Étape 2 — Déterminer quel contrôleur matériel utiliser

Exemple :

```text
Capteur
   │
   │ SPI
   ▼
SPI2 du MCU
```

Tu dois alors trouver dans le Device Tree du SoC :

```dts
spi2: spi@... {
    ...
};
```

Cherche :

```text
spi2:
```

ou :

```text
spi@...
```

ou dans la documentation du SoC.

**Le nom `spi2` n'est pas universel.**

Sur un autre MCU, tu pourrais avoir :

```text
spi1
spi2
lpspi3
flexcomm4
sercom2
```

C'est précisément pour cela qu'il faut apprendre à retrouver l'information plutôt qu'à mémoriser des snippets.

---

# 17. Étape 3 — Trouver le node dans `zephyr.dts`

Après un premier build :

```bash
west build -b <board> .
```

regarde :

```text
build/zephyr/zephyr.dts
```

C'est probablement **le fichier le plus précieux lorsque tu travailles avec Device Tree**.

Tu peux rechercher :

```text
spi
```

puis :

```text
spi2
```

Tu trouveras quelque chose comme :

```dts
spi2: spi@40003800 {
    compatible = "...";
    reg = <...>;
    interrupts = <...>;
    clocks = <...>;
    status = "disabled";
    ...
};
```

À partir de là, tu sais ce que Zephyr connaît réellement.

---

# 18. Étape 4 — Trouver le binding

C'est la deuxième chose que je ferais.

Tu prends :

```dts
compatible = "xxx,yyy";
```

et tu cherches :

```text
xxx,yyy.yaml
```

dans :

```text
zephyr/dts/bindings/
```

Le binding te dit :

```text
compatible
   │
   ├── propriétés
   │
   ├── types
   │
   ├── obligatoire ?
   │
   ├── valeurs
   │
   └── relations avec d'autres nodes
```

Les bindings Zephyr se trouvent notamment dans le dépôt Zephyr, les applications, les boards, les shields et certains modules. ([Zephyr Project Documentation][3])

---

# 19. Étape 5 — Distinguer les propriétés du contrôleur et celles du périphérique

Prenons un bus I²C :

```dts
&i2c1 {
    status = "okay";

    sensor@48 {
        compatible = "vendor,sensor";
        reg = <0x48>;
    };
};
```

Il y a deux bindings potentiels.

### Contrôleur

```text
i2c1
```

Binding :

```text
I²C controller
```

Il possède des propriétés comme :

```text
clock-frequency
pinctrl-0
pinctrl-names
status
```

### Périphérique

```text
sensor@48
```

Binding :

```text
vendor,sensor
```

Il possède ses propres propriétés :

```text
reg
interrupt-gpios
...
```

Donc :

```text
                 I2C controller
                 compatible A
                       │
                       │
                       ▼
                 sensor@48
                 compatible B
```

**Un node enfant n'hérite pas simplement de toutes les propriétés du parent.**

Il faut regarder le binding correspondant à chaque node.

---

# 20. Les phandles : le mécanisme de connexion entre nodes

Voici un concept fondamental.

Supposons :

```dts
sensor0: sensor@48 {
    interrupt-gpios = <&gpioc 5 GPIO_ACTIVE_LOW>;
};
```

Tu viens de créer une relation :

```text
sensor0
   │
   │ interrupt-gpios
   ▼
GPIOC pin 5
```

`&gpioc` est une **référence vers un autre node**.

C'est ce qu'on appelle un **phandle**.

Donc :

```dts
&gpioc
```

ne signifie pas :

> « GPIOC est un périphérique. »

Cela signifie :

> « référence le node identifié par le label `gpioc`. »

Les propriétés de type `phandle`, `phandles` ou `phandle-array` servent précisément à établir ce genre de relation. ([Zephyr Project Documentation][2])

---

# 21. Pourquoi `gpios` est plus compliqué qu'un simple phandle

Regarde :

```dts
reset-gpios = <&gpioa 3 GPIO_ACTIVE_LOW>;
```

Il y a trois informations :

```text
&gpioa
   │
   └── contrôleur GPIO

3
   │
   └── numéro de pin

GPIO_ACTIVE_LOW
   │
   └── flags
```

C'est un **phandle-array**.

Conceptuellement :

```text
reset-gpios
    │
    ├── controller = gpioa
    ├── pin        = 3
    └── flags      = ACTIVE_LOW
```

Le binding indique comment interpréter ces cellules.

C'est pourquoi tu dois toujours regarder :

```text
binding
   ↓
type de propriété
   ↓
specifier cells
```

---

# 22. Les `#*-cells`

C'est une autre notion qui devient essentielle.

Un contrôleur GPIO peut avoir :

```dts
gpio-controller;
#gpio-cells = <2>;
```

Cela signifie grosso modo :

> « Lorsqu'un autre node me référence comme GPIO, il doit fournir deux cellules après mon phandle. »

Donc :

```dts
<&gpioa 3 GPIO_ACTIVE_LOW>
```

se décompose en :

```text
&gpioa
   │
   ├── cell 0 = 3
   └── cell 1 = GPIO_ACTIVE_LOW
```

Les noms exacts de ces cellules sont définis par le binding.

C'est pourquoi **tu ne dois jamais deviner la syntaxe d'un phandle-array**.

---

# 23. La même idée existe pour presque tous les sous-systèmes

Tu retrouveras la même architecture partout :

### GPIO

```text
<controller pin flags>
```

### Interrupt

```text
<interrupt-controller irq flags>
```

### PWM

```text
<pwm-controller channel period flags>
```

### ADC

```text
<adc-controller channel ...>
```

### DMA

```text
<dma-controller channel ...>
```

### Clock

```text
<clock-controller ...>
```

### Reset

```text
<reset-controller ...>
```

Le principe est toujours :

```text
phandle
   +
specifier cells
```

---

# 24. Pinctrl : ne pas le confondre avec GPIO

C'est probablement la prochaine grosse difficulté.

Prenons un UART.

Il faut distinguer :

```text
UART
 │
 ├── périphérique UART
 │
 └── pins physiques
```

Le node UART peut dire :

```dts
pinctrl-0 = <&uart3_tx_pa10 &uart3_rx_pa11>;
pinctrl-names = "default";
```

Cela signifie :

> « Lorsque ce périphérique utilise son état `default`, applique cette configuration de pins. »

Le pinctrl est donc une **couche de multiplexage/configuration des pins du MCU**.

---

# 25. Pourquoi pinctrl existe

Un MCU moderne peut permettre :

```text
PA9  → UART1_TX
PA9  → TIM1_CH2
PA9  → GPIO
PA9  → autre fonction
```

Le Device Tree doit donc décrire :

```text
PA9
 │
 └── fonction sélectionnée = UART1_TX
```

C'est le rôle du pinctrl.

Conceptuellement :

```text
                 UART1
                   │
             pinctrl-0
                   │
                   ▼
            ┌─────────────┐
            │ pin config  │
            └──────┬──────┘
                   │
             ┌─────┴─────┐
             ▼           ▼
           PA9          PA10
           TX            RX
```

Les noms exacts des nodes et propriétés de pinctrl dépendent énormément du SoC.

**C'est donc une des informations que tu dois récupérer dans la documentation du SoC/board.**

---

# 26. Il faut distinguer trois choses

C'est très important pour éviter des erreurs :

```text
GPIO
 │
 └── contrôle logique d'une pin

Pinctrl
 │
 └── multiplexage/configuration électrique de la pin

Périphérique
 │
 └── fonction matérielle : UART/SPI/I2C/etc.
```

Exemple :

```text
PA10
 │
 ├── physiquement une pin
 │
 ├── pinctrl → fonction UART_RX
 │
 └── UART3 → utilise cette entrée
```

---

# 27. Un exemple complet : UART

Supposons que ton matériel possède :

```text
MCU
 └── UART3
      ├── TX → PA10
      └── RX → PB11
```

Ton raisonnement devrait être :

### A. Trouver UART3

Dans le DTS final :

```dts
uart3: serial@... {
    compatible = "...";
    ...
    status = "disabled";
};
```

### B. Trouver son binding

Tu cherches le `compatible`.

### C. Trouver la configuration pinctrl

Dans le `.dtsi` du SoC ou les fichiers associés, tu recherches les configurations correspondant à UART3 TX/RX.

### D. Créer l'overlay

Conceptuellement :

```dts
&uart3 {
    status = "okay";
    pinctrl-0 = <&uart3_default>;
    pinctrl-names = "default";
};
```

Le contenu exact de `uart3_default` dépend du MCU.

**C'est justement une information que tu récupères dans les fichiers DTS/DTSI du SoC.**

---

# 28. Exemple : GPIO

Supposons que tu veuilles simplement :

```text
LED → PA5
```

Le raisonnement est différent.

Il n'y a pas nécessairement un périphérique complexe.

Tu peux créer un node utilisateur :

```dts
led0: led_0 {
    gpios = <&gpioa 5 GPIO_ACTIVE_HIGH>;
};
```

Et dans ton C :

```c
#define LED_NODE DT_NODELABEL(led0)
```

puis récupérer la description GPIO avec les macros Zephyr appropriées.

Le point important est que :

```text
led0
 │
 └── gpios
       │
       ├── gpioa
       ├── pin 5
       └── active high
```

Le node `led0` **ne représente pas nécessairement un périphérique matériel autonome**.

Il peut représenter une abstraction matérielle utile à ton application.

---

# 29. C'est là que les bindings deviennent intéressants

Tu pourrais avoir un node :

```dts
led0: led_0 {
    compatible = "gpio-leds";
    gpios = <&gpioa 5 GPIO_ACTIVE_HIGH>;
};
```

Le `compatible` indique au Device Tree quel binding s'applique.

Le binding connaît :

```text
gpio-leds
   │
   └── gpios
```

Et ton code peut ensuite utiliser les APIs/macros adaptées.

---

# 30. Le Device Tree et le code C

Maintenant arrive le pont entre Device Tree et application.

Supposons :

```dts
led0: led_0 {
    gpios = <&gpioa 5 GPIO_ACTIVE_HIGH>;
};
```

Dans C :

```c
#define LED_NODE DT_NODELABEL(led0)
```

Tu as maintenant :

```text
LED_NODE
   │
   ▼
node du Device Tree
```

Tu peux alors interroger ses propriétés avec les macros `DT_*`.

Par exemple conceptuellement :

```c
DT_PROP(LED_NODE, ...)
```

ou utiliser une macro spécialisée du sous-système GPIO.

Zephyr fournit une API de macros `devicetree.h` permettant d'accéder aux propriétés, phandles, registres, interruptions, etc. ([Zephyr Project Documentation][2])

---

# 31. Une distinction extrêmement importante : Node ID vs `struct device`

Il faut bien distinguer :

```text
Node Device Tree
       │
       ▼
Node ID
       │
       ▼
description statique
```

de :

```text
struct device
       │
       ▼
objet Zephyr représentant le périphérique
```

Le node ID n'est **pas** un pointeur vers un périphérique.

Par exemple :

```c
#define UART_NODE DT_NODELABEL(uart3)
```

`UART_NODE` est un identifiant utilisé par les macros DT.

Pour obtenir le device Zephyr correspondant, on utilise ensuite les mécanismes `DEVICE_DT_*` appropriés.

Conceptuellement :

```text
DT node
   │
   │ DEVICE_DT_...
   ▼
struct device *
```

---

# 32. Pourquoi cette architecture est puissante

Imagine ton application :

```c
uart_configure(...);
```

Elle ne devrait idéalement pas contenir :

```c
#define UART_BASE 0x40004800
#define UART_IRQ  39
#define UART_TX_PIN 10
#define UART_RX_PIN 11
```

Sinon ton application devient dépendante du MCU.

Avec Device Tree :

```text
application
     │
     │ "je veux UART"
     ▼
Device Tree
     │
     ├── quel UART ?
     ├── quelle adresse ?
     ├── quelle IRQ ?
     ├── quels pins ?
     └── quelle configuration ?
```

Tu peux alors changer de board sans réécrire l'application.

---

# 33. Les aliases

Un alias permet de donner un nom **fonctionnel** à un node.

Exemple :

```dts
/ {
    aliases {
        my-uart = &uart3;
    };
};
```

Ton application peut alors utiliser :

```c
DT_ALIAS(my_uart)
```

L'intérêt est énorme.

Ton application dit :

> « Je veux le UART de mon application. »

Elle ne dit pas :

> « Je veux UART3 du STM32. »

Sur une autre carte :

```dts
my-uart = &uart1;
```

et l'application n'a pas besoin de changer.

Les aliases sont simplement des propriétés du node `/aliases`. ([Zephyr Project Documentation][6])

---

# 34. `chosen`

`chosen` fonctionne de manière similaire mais pour des rôles particuliers du système.

Par exemple :

```dts
/chosen {
    zephyr,console = &uart3;
};
```

Cela signifie :

```text
console Zephyr
     │
     └── UART3
```

L'application ou les sous-systèmes qui recherchent :

```text
zephyr,console
```

peuvent alors retrouver ce node.

C'est donc une façon de dire :

> « Ce périphérique joue ce rôle système. »

---

# 35. Alias vs chosen

Je te conseille de retenir ceci :

### Alias

```text
nom fonctionnel de ton choix
```

Exemple :

```text
led0
sensor
my_uart
```

### Chosen

```text
rôle standard défini par Zephyr
```

Exemple :

```text
console
shell
flash
sram
```

---

# 36. Le fichier final est ton meilleur outil de diagnostic

Supposons que tu as écrit :

```text
app.overlay
```

mais que ça ne fonctionne pas.

Ne commence pas immédiatement à modifier le C.

Fais :

```bash
west build -p always
```

puis regarde :

```text
build/zephyr/zephyr.dts
```

Pose-toi ces questions :

```text
1. Mon node existe-t-il ?
2. Est-il au bon endroit ?
3. Son compatible est-il correct ?
4. status = "okay" ?
5. Les propriétés sont-elles présentes ?
6. Les phandles pointent-ils vers les bons nodes ?
7. Le pinctrl est-il présent ?
8. Les pins sont-ils les bons ?
```

C'est beaucoup plus efficace que de déboguer à l'aveugle.

Zephyr documente explicitement cette méthode pour obtenir le Device Tree final et retrouver les bindings utilisés. ([Zephyr Project Documentation][6])

---

# 37. Comment retrouver le binding depuis le fichier généré

Il existe une astuce extrêmement utile.

Le fichier :

```text
build/zephyr/include/generated/zephyr/devicetree_generated.h
```

contient beaucoup de métadonnées.

La documentation indique qu'il contient notamment la liste des nodes et, pour les nodes associés à un binding, le chemin du binding correspondant. ([Zephyr Project Documentation][6])

Donc ton workflow peut devenir :

```text
zephyr.dts
   │
   │ trouver le node
   ▼
compatible
   │
   ▼
devicetree_generated.h
   │
   ▼
binding YAML
```

---

# 38. Le workflow que je te conseille pour n'importe quel périphérique

Voici vraiment la procédure que tu peux garder comme **checklist personnelle**.

## Phase A — Hardware

### 1. Identifier le périphérique

Exemple :

```text
MCPxxx
```

### 2. Identifier son interface

```text
I²C
```

### 3. Identifier son adresse

```text
0x48
```

### 4. Identifier les signaux supplémentaires

```text
INT
RESET
ENABLE
```

### 5. Identifier les pins du MCU

À partir du schematic :

```text
I2C_SCL → PB8
I2C_SDA → PB9
INT     → PC4
RESET   → PA7
```

---

# 39. Phase B — Zephyr

### 6. Vérifier que le SoC est supporté

Si oui, continue.

Sinon, ce n'est plus simplement un problème d'overlay.

---

### 7. Identifier le contrôleur

Exemple :

```text
I2C1
```

---

### 8. Trouver son node

Dans :

```text
.dtsi
```

ou :

```text
build/zephyr/zephyr.dts
```

---

### 9. Trouver son `compatible`

Exemple :

```dts
compatible = "...";
```

---

### 10. Trouver son binding

Recherche le `compatible` dans :

```text
dts/bindings/
```

---

### 11. Vérifier son état

```dts
status = "disabled";
```

→ il faudra probablement :

```dts
status = "okay";
```

---

### 12. Trouver le pinctrl

Chercher les configurations correspondant à :

```text
I2C1 SCL
I2C1 SDA
```

---

# 40. Phase C — Le périphérique externe

Maintenant seulement tu crées l'enfant :

```dts
&i2c1 {

    status = "okay";

    mon_capteur: sensor@48 {
        compatible = "vendor,sensor";
        reg = <0x48>;
    };
};
```

Puis tu ajoutes les propriétés **qui sont réellement définies par son binding** :

```dts
interrupt-gpios = <...>;
reset-gpios = <...>;
...
```

Tu ne devrais pas avoir besoin de deviner.

---

# 41. Phase D — Vérification

Compile.

Puis :

```text
build/zephyr/zephyr.dts
```

et vérifie le résultat final.

Ensuite :

```text
devicetree_generated.h
```

pour comprendre les macros générées si nécessaire.

---

# 42. Phase E — Code C

Dans ton application :

```c
#define SENSOR_NODE DT_NODELABEL(mon_capteur)
```

Puis utilise les APIs/macros correspondant au sous-système.

Tu ne dois généralement pas faire :

```c
#define SENSOR_ADDRESS 0x48
```

car cette information appartient maintenant au Device Tree.

---

# 43. Exemple complet de raisonnement : SPI

Prenons un périphérique SPI.

Matériel :

```text
STM32
 │
 └── SPI2
      │
      ├── SCK
      ├── MISO
      ├── MOSI
      └── CS
            │
            ▼
         capteur
```

Tu dois trouver :

```text
1. node SPI2
2. binding SPI2
3. pinctrl SPI2
4. node du capteur
5. binding du capteur
6. CS
7. propriétés spécifiques du capteur
```

Le résultat conceptuel sera :

```dts
&spi2 {
    status = "okay";

    capteur@0 {
        compatible = "vendor,capteur";
        reg = <0>;
        ...
    };
};
```

Puis éventuellement une configuration CS appropriée au contrôleur SPI.

**Ne copie surtout pas cette structure aveuglément :** `reg`, `cs-gpios`, pinctrl et autres propriétés dépendent du contrôleur et du binding concernés.

---

# 44. Exemple complet : périphérique I²C

Même logique :

```dts
&i2c1 {
    status = "okay";

    capteur@48 {
        compatible = "vendor,capteur";
        reg = <0x48>;

        interrupt-gpios = <&gpioc 4 GPIO_ACTIVE_LOW>;
    };
};
```

On peut lire cet arbre comme une phrase :

> Sur le contrôleur I²C1, il existe un périphérique compatible avec `vendor,capteur`, à l'adresse 0x48, dont la sortie d'interruption est connectée à GPIOC4 active-bas.

C'est exactement le type de raisonnement que tu dois développer.

---

# 45. Et maintenant le point subtil : le binding ne configure pas le driver

Il faut éviter une autre confusion.

Le binding dit essentiellement :

```text
"Voici comment interpréter ce node."
```

Le driver dit :

```text
"Voici comment faire fonctionner ce matériel."
```

Par exemple :

```text
             Device Tree
                  │
                  ▼
             compatible
                  │
                  ▼
              binding
                  │
                  │ valide/interprète
                  ▼
             propriétés
                  │
                  ▼
               driver
                  │
                  ▼
          registres du périphérique
```

Le binding n'est donc **pas le driver**.

---

# 46. Et Kconfig dans tout ça ?

Tu dois aussi séparer :

```text
Device Tree
```

et :

```text
Kconfig
```

Ils ont des responsabilités différentes.

### Device Tree

Répond à :

> Quel matériel existe et comment est-il connecté/configuré ?

### Kconfig

Répond à :

> Quel code/fonctionnalité doit être compilé ?

Par exemple :

```text
DTS
    sensor@48 {
        compatible = "...";
        status = "okay";
    };
```

ne signifie pas nécessairement :

> « Le driver est compilé. »

Il faut aussi que le Kconfig approprié soit activé.

Le modèle mental complet est donc :

```text
                  HARDWARE
                     │
          ┌──────────┴──────────┐
          ▼                     ▼
    Device Tree               Kconfig
          │                     │
          │                     └── compile driver ?
          │
          └── configuration
                    │
                    ▼
                  DRIVER
                    │
                    ▼
               APPLICATION
```

---

# 47. Le piège : « mon overlay compile mais ça ne marche pas »

Il existe plusieurs niveaux de problème.

### Niveau 1 — Syntaxe DTS

```text
Erreur de parsing
```

### Niveau 2 — Binding

```text
property inconnue
type incorrect
property obligatoire absente
```

### Niveau 3 — Device Tree

```text
mauvais node
mauvais phandle
mauvais pin
status disabled
```

### Niveau 4 — Kconfig

```text
driver non compilé
```

### Niveau 5 — Driver

```text
driver incorrect/incompatible
```

### Niveau 6 — Hardware

```text
mauvais câblage
pull-up manquante
mauvaise alimentation
```

C'est important parce qu'un :

```text
west build
```

qui réussit ne prouve absolument pas que ton matériel est correctement configuré.

---

# 48. Comment trouver les informations sans connaître Zephyr par cœur

Voici une méthode beaucoup plus efficace que Google au hasard.

Supposons :

> Je veux utiliser SPI2.

### Recherche 1

Dans :

```text
build/zephyr/zephyr.dts
```

chercher :

```text
spi2
```

Tu obtiens :

```dts
spi2: spi@...
```

---

### Recherche 2

Lire :

```text
compatible = "...";
```

---

### Recherche 3

Chercher ce `compatible` dans :

```text
dts/bindings
```

Tu obtiens le binding.

---

### Recherche 4

Dans le binding, regarder :

```text
properties:
```

---

### Recherche 5

Pour chaque propriété complexe :

```text
pinctrl-0
cs-gpios
interrupts
reset-gpios
clocks
```

suivre les références vers les autres nodes.

---

### Recherche 6

Dans le `.dtsi` du SoC :

```text
spi2
```

pour trouver :

```text
pinctrl
interrupt
clock
```

---

### Recherche 7

Dans le schematic :

```text
SPI2_SCK
SPI2_MISO
SPI2_MOSI
SPI2_CS
```

pour déterminer les pins réelles.

---

# 49. La méthode « remonter et descendre »

Je te recommande même une technique mentale :

## Remonter

Depuis ton périphérique :

```text
sensor
   ↑
   │
SPI2
   ↑
   │
MCU
```

Tu cherches :

```text
Quel bus ?
Quel contrôleur ?
Quel binding ?
```

## Descendre

Depuis le contrôleur :

```text
SPI2
 │
 ├── pinctrl
 ├── clocks
 ├── interrupts
 └── devices enfants
```

Tu détermines :

```text
quels pins ?
quelle IRQ ?
quel clock ?
quels périphériques ?
```

C'est très efficace.

---

# 50. Une règle particulièrement importante : ne pas inventer un node si un node existe déjà

Supposons que le SoC possède :

```dts
uart3: serial@40004800 {
    ...
};
```

Ne fais pas :

```dts
my_uart: serial@40004800 {
    ...
};
```

dans ton overlay.

Tu dois généralement modifier le node existant :

```dts
&uart3 {
    status = "okay";
};
```

Même chose pour :

```text
GPIO
UART
SPI
I2C
ADC
PWM
TIM
USB
CAN
Ethernet
```

Les fichiers `.dtsi` du SoC contiennent déjà les contrôleurs matériels.

Ton overlay vient principalement **personnaliser ce qui existe déjà**.

---

# 51. Quand créer un nouveau node ?

Tu crées typiquement un nouveau node lorsqu'il représente :

### un périphérique externe

```text
I2C
 └── sensor@48
```

### une abstraction de ton board

```text
led0
button0
```

### une configuration spécifique

```text
fixed-partitions
```

etc.

Mais pour un périphérique matériel déjà défini par le SoC :

```text
UART3
SPI2
I2C1
GPIOA
```

tu modifies généralement le node existant.

---

# 52. Comment savoir quelles propriétés écrire ?

C'est ici que le binding devient ton **manuel de référence immédiat**.

Tu as :

```dts
compatible = "foo,bar";
```

Tu trouves :

```text
foo,bar.yaml
```

et regardes :

```yaml
properties:
```

Par exemple, conceptuellement :

```yaml
properties:

    foo:
        type: int
        required: true

    reset-gpios:
        type: phandle-array

    mode:
        type: string
        enum:
            - ...
            - ...
```

Tu sais alors :

```text
foo             → entier
reset-gpios     → référence GPIO
mode             → chaîne avec valeurs limitées
```

Le binding est donc ton **contrat syntaxique**.

La documentation Zephyr décrit notamment `type`, `required`, `default`, `enum`, `min/max`, `phandle-array`, etc. ([Zephyr Project Documentation][7])

---

# 53. Le binding peut lui-même dépendre d'autres bindings

Tu peux rencontrer :

```text
include:
```

ou :

```text
child-binding:
```

ou :

```text
bus:
on-bus:
```

Ne te laisse pas intimider.

Le raisonnement reste :

```text
Mon node
   │
   ▼
compatible
   │
   ▼
binding
   │
   ├── propriétés propres
   │
   └── propriétés héritées/incluses
```

Si une propriété que tu cherches n'est pas immédiatement visible, regarde les `include`.

---

# 54. Le concept de `bus`

Un node peut représenter un contrôleur de bus :

```text
I2C
SPI
UART
```

et ses enfants.

Par exemple :

```text
i2c1
 │
 ├── sensor@48
 └── eeprom@50
```

Le binding de `sensor@48` peut également tenir compte du fait qu'il se trouve sur un bus I²C.

C'est une des raisons pour lesquelles le **contexte du node dans l'arbre est important**.

---

# 55. Les macros C ne sont que la dernière étape

Une erreur fréquente est de commencer par :

```c
DT_NODELABEL(...)
DT_PROP(...)
DT_GPIO(...)
```

alors que tu ne comprends pas encore le node.

Je te recommande l'ordre inverse :

```text
1. Hardware
2. Node
3. Compatible
4. Binding
5. Properties
6. Phandles
7. Pinctrl
8. Overlay
9. zephyr.dts
10. macros C
```

Les macros C deviennent alors presque triviales.

---

# 56. Comment lire une erreur Device Tree

Supposons une erreur :

```text
DT_N_S_soc_S_spi_40003800_P_cs_gpios_IDX_0_VAL_pin
```

Ça paraît horrible.

Mais tu peux la lire conceptuellement :

```text
DT
 │
 └── node
      │
      └── /soc/spi@40003800
            │
            └── property cs-gpios
                  │
                  └── index 0
                        │
                        └── cell pin
```

Les macros générées ont effectivement une nomenclature assez complexe, mais Zephyr fournit les macros `DT_*` pour éviter d'avoir à les manipuler directement. ([Zephyr Project Documentation][2])

---

# 57. Ton outil de référence principal : `zephyr.dts`

Je te conseille presque de prendre cette habitude :

```bash
west build ...
```

puis :

```bash
less build/zephyr/zephyr.dts
```

et de considérer ce fichier comme :

> **« Voilà ce que Zephyr pense que mon matériel est. »**

C'est extrêmement puissant.

---

# 58. Ton deuxième outil : rechercher le binding

Une fois que tu connais :

```dts
compatible = "xxx,yyy";
```

cherche :

```bash
grep -R '"xxx,yyy"' zephyr/dts/bindings
```

ou :

```bash
grep -R 'compatible:.*xxx,yyy' zephyr/dts/bindings
```

Cela te donne le binding.

---

# 59. Ton troisième outil : les fichiers `.dtsi`

Pour un MCU donné, tu vas principalement naviguer dans :

```text
soc/
dts/
boards/
```

Tu cherches :

```text
GPIO
UART
SPI
I2C
ADC
PWM
pinctrl
```

Le `.dtsi` du SoC te donne généralement la **description générique du silicium**.

Le `.dts` de la board ajoute :

```text
ce qui est particulier à la carte.
```

Et ton `.overlay` ajoute :

```text
ce qui est particulier à ton application.
```

---

# 60. La hiérarchie des fichiers

Une représentation utile :

```text
               SoC
                │
                ▼
             soc.dtsi
                │
                │ décrit
                ▼
        périphériques du MCU
                │
                ▼
            board.dts
                │
                │ décrit
                ▼
       câblage de la carte
                │
                ▼
          app.overlay
                │
                │ personnalise
                ▼
          zephyr.dts final
```

Puis :

```text
zephyr.dts
    │
    ├── bindings
    │
    ▼
devicetree_generated.h
    │
    ▼
drivers
    │
    ▼
application
```

---

# 61. Comment concevoir un overlay « propre »

Je te recommande cette structure mentale :

```dts
/* 1. Activer le contrôleur */

&spi2 {
    status = "okay";

    /* 2. Configuration du bus */

    pinctrl-0 = <&spi2_default>;
    pinctrl-names = "default";

    /* 3. Périphérique externe */

    mon_capteur: sensor@0 {
        compatible = "vendor,sensor";
        reg = <0>;

        /* 4. Connexions supplémentaires */

        interrupt-gpios = <&gpioc 4 GPIO_ACTIVE_LOW>;
    };
};

/* 5. Éventuellement alias/chosen */

 / {
    aliases {
        sensor0 = &mon_capteur;
    };
};
```

Ce n'est pas un template universel à copier. C'est plutôt une **structure logique** :

```text
contrôleur
   ↓
configuration
   ↓
périphérique
   ↓
connexions
   ↓
interface application
```

---

# 62. Une distinction très importante : propriété vs connexion

Dans :

```dts
current-speed = <115200>;
```

tu as une **valeur**.

Dans :

```dts
interrupt-gpios = <&gpioa 5 GPIO_ACTIVE_LOW>;
```

tu as une **relation**.

Donc :

```text
Properties
   │
   ├── valeurs simples
   │      └── 115200
   │
   └── références
          └── &gpioa
```

Cette distinction devient essentielle lorsque tu lis les bindings.

---

# 63. Les trois catégories de propriétés à reconnaître

Quand tu lis un binding, classe mentalement les propriétés en trois catégories.

### A. Valeur

```dts
clock-frequency = <100000>;
```

### B. Référence

```dts
reset-gpios = <&gpioa ...>;
```

### C. Liste de références

```dts
clocks = <&rcc ...>, <&...>;
```

Cela te permet immédiatement de comprendre la structure.

---

# 64. Et maintenant : comment créer un overlay à partir de zéro

Voici le processus complet que je te recommande de suivre.

---

## Phase 1 — Décrire le matériel sur papier

Écris :

```text
Périphérique :
Interface :
Contrôleur MCU :
Adresse :
Pins :
Interrupt :
Reset :
Enable :
CS :
```

Ne touche pas encore au DTS.

---

## Phase 2 — Trouver le contrôleur dans Zephyr

Cherche :

```text
UARTx
SPIx
I2Cx
ADCx
PWMx
```

dans le `.dtsi`.

---

## Phase 3 — Examiner son node

Note :

```text
label
compatible
status
reg
interrupts
pinctrl
clocks
```

---

## Phase 4 — Trouver le binding

Avec :

```text
compatible
```

trouve :

```text
*.yaml
```

---

## Phase 5 — Faire la liste des propriétés

Pour chaque propriété potentiellement nécessaire :

```text
nom
type
obligatoire ?
valeur par défaut ?
enum ?
phandle ?
phandle-array ?
```

---

## Phase 6 — Résoudre les références

Pour chaque :

```text
&xxx
```

trouve :

```text
xxx:
```

puis examine ce node.

---

## Phase 7 — Vérifier pinctrl

À partir du schematic :

```text
fonction périphérique
       ↓
pin physique
       ↓
configuration pinctrl Zephyr
```

---

## Phase 8 — Écrire l'overlay minimal

Commence avec le strict nécessaire :

```dts
&xxx {
    status = "okay";
};
```

Compile.

Puis ajoute progressivement :

```text
pinctrl
device
interrupt
reset
etc.
```

C'est beaucoup plus facile à déboguer qu'un gros overlay écrit d'un seul coup.

---

# 65. La stratégie « minimal → enrichissement »

Par exemple :

### Étape 1

```dts
&i2c1 {
    status = "okay";
};
```

Build.

### Étape 2

Ajouter pinctrl.

Build.

### Étape 3

Ajouter le périphérique :

```dts
sensor@48 {
    compatible = "...";
    reg = <0x48>;
};
```

Build.

### Étape 4

Ajouter l'interruption.

Build.

### Étape 5

Ajouter reset.

Build.

### Étape 6

Écrire le C.

Cette méthode permet de déterminer **exactement quelle modification introduit une erreur**.

---

# 66. Une chose à ne jamais faire

Ne fais pas :

> « J'ai trouvé un exemple Internet avec un `spi2`, donc je vais copier. »

Parce que tu ignores :

```text
quel SoC ?
quel board ?
quel binding ?
quelle version Zephyr ?
quels pins ?
quel contrôleur ?
quelle pinctrl ?
```

Utilise plutôt un exemple pour découvrir **où chercher**.

Puis valide chaque élément contre :

```text
ton SoC
ton board
ton schematic
ton binding
ton Zephyr
```

---

# 67. Ce qu'il faut chercher dans la documentation externe

Puisque tu souhaites utiliser les sources externes uniquement pour récupérer les données nécessaires, voici précisément **ce que tu dois chercher**.

## Datasheet MCU

Tu cherches :

```text
Peripheral instances
Register address
Alternate functions
GPIO ports
GPIO pin numbers
Interrupt numbers
Clock sources
```

---

## Reference manual MCU

Tu cherches :

```text
UART instance
SPI instance
I2C instance
GPIO controller
pin multiplexing
clock configuration
interrupt mapping
```

---

## Schematic de la carte

Tu cherches :

```text
MCU pin
    ↓
connector/component
```

---

## Zephyr DTS/DTSI

Tu cherches :

```text
node label
compatible
status
reg
interrupts
pinctrl
```

---

## Binding Zephyr

Tu cherches :

```text
properties
type
required
default
enum
phandle
phandle-array
*-cells
```

---

# 68. Les sources officielles à garder dans tes favoris

Voici les pages que je te conseille d'utiliser comme **références**, plutôt que comme tutoriels :

* [Introduction au Device Tree Zephyr](https://docs.zephyrproject.org/latest/build/dts/intro.html?utm_source=chatgpt.com) — syntaxe et structure générales. ([Zephyr Project Documentation][1])
* [Bindings Zephyr](https://docs.zephyrproject.org/latest/build/dts/bindings.html?utm_source=chatgpt.com) — fonctionnement des bindings. ([Zephyr Project Documentation][8])
* [Syntaxe des bindings](https://docs.zephyrproject.org/latest/build/dts/bindings-syntax.html?utm_source=chatgpt.com) — propriétés, types, `required`, `enum`, `phandle-array`, etc. ([Zephyr Project Documentation][7])
* [HOWTO Device Tree](https://docs.zephyrproject.org/latest/build/dts/howtos.html?utm_source=chatgpt.com) — retrouver le DT final, les bindings et gérer les overlays. ([Zephyr Project Documentation][6])
* [Accès au Device Tree depuis C/C++](https://docs.zephyrproject.org/latest/build/dts/api-usage.html?utm_source=chatgpt.com) — passage du node DT aux macros C. ([Zephyr Project Documentation][2])
* [API Device Tree](https://docs.zephyrproject.org/latest/build/dts/api/api.html?utm_source=chatgpt.com) — référence des macros `DT_*`. ([Zephyr Project Documentation][9])
* [Index des bindings](https://docs.zephyrproject.org/latest/build/dts/api/bindings.html?utm_source=chatgpt.com) — recherche d'un binding par `compatible`. ([Zephyr Project Documentation][4])

---

# 69. Le modèle mental final à retenir

Si tu ne devais retenir qu'un seul diagramme, ce serait celui-ci :

```text
                         MATÉRIEL
                            │
              ┌─────────────┴─────────────┐
              │                           │
           Datasheet                  Schematic
              │                           │
              └─────────────┬─────────────┘
                            │
                     informations
                     matérielles
                            │
                            ▼
                    ┌──────────────┐
                    │   DTS/DTSI   │
                    │              │
                    │    NODE      │
                    │      │       │
                    │      ├─ compatible
                    │      ├─ status
                    │      ├─ reg
                    │      ├─ pinctrl
                    │      ├─ gpios
                    │      ├─ clocks
                    │      └─ ...
                    └──────┬───────┘
                           │
                     compatible
                           │
                           ▼
                    ┌──────────────┐
                    │   BINDING    │
                    │    YAML      │
                    │              │
                    │ propriétés   │
                    │ types        │
                    │ contraintes  │
                    │ cells        │
                    └──────┬───────┘
                           │
                           ▼
                 Device Tree Compiler
                           │
                           ▼
                 ┌──────────────────┐
                 │  zephyr.dts      │
                 │  generated.h     │
                 └────────┬─────────┘
                          │
                     DT_* macros
                          │
                          ▼
                 ┌──────────────────┐
                 │  Zephyr driver   │
                 └────────┬─────────┘
                          │
                          ▼
                    struct device
                          │
                          ▼
                    Application C
```

Et surtout :

```text
                 NODE
                  │
       ┌──────────┼───────────┐
       │          │           │
       ▼          ▼           ▼
 compatible    properties   phandles
       │          │           │
       │          │           └──────► autre NODE
       │          │
       │          └──────────────► valeurs/configuration
       │
       └──────────────► BINDING
                           │
                           └──► définit comment
                                interpréter le node
```

**C'est cette relation qu'il faut comprendre.**

---

# 70. La recette universelle en 12 questions

Lorsque tu dois créer un overlay pour **n'importe quel périphérique**, pose-toi simplement ces questions dans cet ordre :

1. **Quel est le matériel réel ?**
2. **Sur quel bus/interface est-il connecté ?**
3. **Quel contrôleur du MCU utilise-t-il ?**
4. **Quel node Zephyr représente ce contrôleur ?**
5. **Quel est son `compatible` ?**
6. **Quel binding correspond à ce `compatible` ?**
7. **Quelles propriétés ce binding autorise-t-il ?**
8. **Quelles propriétés sont obligatoires ?**
9. **Quels phandles doivent pointer vers quels autres nodes ?**
10. **Comment les pins physiques sont-elles représentées par le pinctrl/GPIO ?**
11. **Quel node dois-je modifier ou créer dans l'overlay ?**
12. **Est-ce que `build/zephyr/zephyr.dts` correspond maintenant exactement à mon matériel ?**

Si tu peux répondre correctement à ces 12 questions, **tu n'as plus besoin de mémoriser les overlays Zephyr**. Tu peux en construire un pour un nouveau MCU ou un nouveau périphérique en remontant systématiquement des données matérielles vers le binding puis vers le node.

Et c'est précisément cette façon de travailler qui transforme le Device Tree de « magie noire Zephyr » en une procédure d'ingénierie assez mécanique.

[1]: https://docs.zephyrproject.org/latest/build/dts/intro.html?utm_source=chatgpt.com "Introduction to devicetree — Zephyr Project Documentation"
[2]: https://docs.zephyrproject.org/latest/build/dts/api-usage.html?utm_source=chatgpt.com "Devicetree access from C/C++ — Zephyr Project Documentation"
[3]: https://docs.zephyrproject.org/latest/build/dts/bindings-intro.html?utm_source=chatgpt.com "Introduction to Devicetree Bindings — Zephyr Project Documentation"
[4]: https://docs.zephyrproject.org/latest/build/dts/api/bindings.html?utm_source=chatgpt.com "Bindings index — Zephyr Project Documentation"
[5]: https://docs.zephyrproject.org/latest/hardware/porting/board_porting.html?utm_source=chatgpt.com "Board Porting Guide — Zephyr Project Documentation"
[6]: https://docs.zephyrproject.org/latest/build/dts/howtos.html?utm_source=chatgpt.com "Devicetree HOWTOs — Zephyr Project Documentation"
[7]: https://docs.zephyrproject.org/latest/build/dts/bindings-syntax.html?utm_source=chatgpt.com "Devicetree bindings syntax — Zephyr Project Documentation"
[8]: https://docs.zephyrproject.org/latest/build/dts/bindings.html?utm_source=chatgpt.com "Devicetree bindings — Zephyr Project Documentation"
[9]: https://docs.zephyrproject.org/latest/build/dts/api/api.html?utm_source=chatgpt.com "Devicetree API — Zephyr Project Documentation"
