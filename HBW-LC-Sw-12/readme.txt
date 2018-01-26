Homematic Wired Homebrew Relaismodul 12fach
==============================================

Das Modul HBW-LC-Sw-12 schaltet bis zu 12 angeschlossene bistabile Relais.
Basis ist ein Arduino NANO mit RS485-Interface.

Direktes Peering m�glich (HBWLinkSwitchSimple).

Damit FHEM das Homebrew-Device richtig erkennt, muss die HBW-LC-Sw-12.xml Datei in den Ordner FHEM/lib/HM485/Devices/xml kopiert werden (Das Device gibt sich als HW-Typ 0x93 aus).

Standard-Pinbelegung:
4 - Rx RS485
2 - Tx RS485
3 - RS485 Enable
13 - Status LED
0 - Rx Debug
1 - Tx Debug
8 - Bedientaster (Reset)
6 - shiftRegOne_Data   (Drei Shiftregister 74HC595 f�r LEDs und Relais)
7 - shiftRegOne_Clock
5 - shiftRegOne_Latch
9 - shiftRegTwo_Data   (Drei weitere Shiftregister 74HC595 f�r LEDs und Relais, in eigenem 6 TE Geh�use)
10 - shiftRegTwo_Clock
11 - shiftRegTwo_Latch


Altenative M�glichkeit, per "#define USE_HARDWARE_SERIAL" aktivieren:
Hier wird Hardware Serial (USART) statt "HBWSoftwareSerial" genutzt! Der Bedientaster (Reset) ist ein Analogeingang!

Pinbelegung:
0 - Rx RS485
1 - Tx RS485
2 - RS485 Enable
13 - Status LED
A6 - Bedientaster (Reset)
6 - shiftRegOne_Data   (Drei Shiftregister 74HC595 f�r LEDs und Relais)
7 - shiftRegOne_Clock
5 - shiftRegOne_Latch
9 - shiftRegTwo_Data   (Drei weitere Shiftregister 74HC595 f�r LEDs und Relais, in eigenem 6 TE Geh�use)
10 - shiftRegTwo_Clock
11 - shiftRegTwo_Latch
