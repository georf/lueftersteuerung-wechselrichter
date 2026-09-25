# Externe Kühlung für Deye 12K Wechselrichter

ESP8266-basierte Lüftersteuerung zur externen Kühlung eines Deye 12K Wechselrichters.

## Funktion

Das Projekt steuert **3 × 14-cm-Lüfter**, die zusätzliche Kühlluft über die Kühlrippen des Wechselrichters führen.

Ein **12-kΩ-NTC** ist direkt zwischen den Kühlrippen des Wechselrichters angebracht und misst deren Temperatur.

Die Lüfter werden abhängig von der gemessenen Temperatur per PWM geregelt. Ziel ist es, die Kühlrippen aktiv zu kühlen und dadurch möglichst lange zu verhindern, dass der interne Lüfter des Wechselrichters anlaufen muss.

Zusätzlich werden die Betriebsdaten per **MQTT** an Home Assistant übertragen.

<img width="337" height="483" alt="image" src="https://github.com/user-attachments/assets/fa573bab-2795-4de2-bbff-6dba887be3d4" />

## Hardware

- ESP8266 / NodeMCU
- 3 × 14-cm-12-V-Lüfter
- IRLZ44N MOSFET zur Lüftersteuerung
- 12-kΩ-NTC
- 12-V-Versorgung
- 12-V → 3,3-V Buck-Converter für den ESP8266

<img width="771" height="602" alt="image" src="https://github.com/user-attachments/assets/c8a07791-e3e3-49d3-ad8f-b8796060ad18" />

Aufgebaut ist die Hardware auf einer Lochrasterplatine. Die Anschlüsse wurden mit Klemmblöcken realisiert.

![Ohne ESP](/Bilder/ohne-ESP.jpg) ![Mit ESP](/Bilder/mit-ESP.jpg)

## Messwerte

Folgende Werte werden per MQTT veröffentlicht:

- Kühlrippentemperatur
- Lüfter-PWM in %
- IP-Adresse

Die MQTT Discovery ermöglicht die automatische Integration der Sensoren in Home Assistant.

## Temperaturregelung

Die Temperatur wird über den NTC gemessen und für die Regelung kalibriert.

Die Lüfterleistung wird abhängig von der Kühlrippentemperatur erhöht. Dadurch laufen die externen Lüfter nur so stark wie notwendig.

## Sicherheit

Die externe Kühlung ist eine **zusätzliche Kühlmaßnahme** und ersetzt nicht die interne Kühlung oder die Schutzfunktionen des Wechselrichters.
