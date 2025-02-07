# RP5ctrl
AV controller for Raspberry Pi 5 power switch and related devices

Raspberry Pi 5 has jumper terminal J2 connected to internal power switch.  This terminal can be used with a switching device.
This concept and PoC figures are reference for schematics and scripts below.
 - concept_diagram.png : concept diagram
 - PoC.png : I used this schematic as a proof of concept.

I made a controller for my RPi5 power switch and related devices like TV and soundbar.
 - rp5etc_cli.py : Python script of the master device with spidev
 - RP5ctrl.ino : Arduino sketch of the slave device
 - RPi5ctrl-sch.png : Schematic of the slave device

I wrote two blog about this in Japanese, please access it if you need more information:<br />
Method : https://pado.tea-nifty.com/top/2025/02/post-e7668f.html<br />
Make : https://pado.tea-nifty.com/top/2025/02/post-96e14b.html<br />
