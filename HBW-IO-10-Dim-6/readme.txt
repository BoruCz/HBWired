Homematic Wired Homebrew PWM/0-10V Master Dimmer + 10 digital Eing�nge
======================================================================

*** In Entwicklung! *** Work in progress! ***
// TODO: Implement digital Input (+ key peering)
//       Implement dim peering params: ON_LEVEL_PRIO, OFFDELAY_BLINK, RAMP_START_STEP, OFFDELAY_STEP


Das Modul HBW-IO-10-Dim-6 bietet 6 analoge Ausg�nge, davon 4 mit PWM Ausgang und 10 digitale Eing�nge.
Basis ist ein Arduino NANO mit RS485-Interface.
(Hinweis Hex Dateien: Geeignet f�r Atmel ATMEGA328p! *with_bootloader* ist ebenfalls f�r ATMEGA328p, aber inkl. dem Arduino NANO Bootloader!)

Direktes Peering m�glich ().

Die 6 analogen Ausgangskan�le geben eine Spannung von "0-10V" (alternativ "1-10V", pro Kanal) als Dimmer aus (0-100%). Vier dieser Sechs Ausg�nge k�nnen das PWM Signal direkt ausgeben (12V Ausgangsspannung). Der PWM, bzw. Spannungsbereich kann auf ein Maximalwert von 40-100% pro Kanal konfiguriert werden.
[4..20mA, 0..20mA per Jumper?]

Die 10 Eing�nge sind galvanisch getrennt und k�nnen mit einer Gleichspannung von 4-24V betrieben werden.
[6 mit, 4 ohne galvanische Trennung? Anschluss von PIR-Modul dirket?]

Damit FHEM das Homebrew-Device richtig erkennt, muss die hbw_io-10_dim-6.xml Datei in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x96 aus).

M�gliche Dimmer-"Endstufen":
0-10V: LED Dimmer, Finder Typ 15.11 - Slave Dimmer, etc.
0-90% PWM: Eltako LUD12-230V

Standard-Pinbelegung:
0 - Rx RS485
1 - Tx RS485
2 - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)

