#piRefly
piRefly is a crossplattform (Raspberry Pi / Arduino) implementation of the kuramoto ossczilator, derived from the work of [1000fireflies.net]. It will be used on [PiAndMore] for an world record trial. You can find the teaser presentation [here].


### Tech
piRefly uses a number of open source projects to work properly:
##### Raspberry Pi
* [OpenWRT] - Debian derivat for Routers
* [wiringPi] - Framework for using Raspberry Pi GPIOs

##### Arduino
* [Arduino] - Bootloaders, Scripts and IDE for developing AVR solutions


### Installation
##### Raspberry Pi
Download openwrt-brcm2708-bcm2708-sdcard-vfat-ext4.7z, extract it using i.e. [7zip] and flash it to an sdcard with i.e. [Win32 Disk Imager] or dd.

##### Arduino
Download piRefly.ino, open it using [Arduino] IDE and upload it to your i.e. Arduino Uno.


### Usage
##### Basic Idea
After you installed piRefly on your RPi or Arduino and do power it on, it will start to blink by using its OnBoard LED(s). Additionally, you can add your own LED or other stuff to the so called LED Port, which will just mirror the blink of the OnBoard LED(s). piRefly will try, by using the Kuramoto Oscillator, to snychronize with other nodes (i.e. RPis or Arduinos) using its COMM Ports. 

##### Setup your piRefly network
Just connect each network node with another network node via the COMM Ports (i.e. GPIO18 on one RPi to GPIO23 on another RPi) AND connect GND Connections from node to node. After that, power the nodes and see the Kuramoto Oscillator synchronizing the LEDs.

##### Raspberry Pi
The RPi Version works for all non RPi2 Models: B pre 2.0, B 2.0, A, B+, A+.
It will use following outputs of the RPi:
* On not + Models: ACT LED
* On + Models: ACT and PWR LED
* On B Models: LAN LEDs
If you use i.e. an B+, it will use ACT, PWR and LAN LEDs.

Additionally, RPi will use:
* GPIO4 as LED Port
* GPIO18, GPIO23, GPIO24, GPIO25 as COMM Ports

##### Arduino
The Arduino Version works for nearly all Arduino Models. It has been designed with the Arduin Uno in mind.
It will use following outputs of the Uno:
* PIN 13 as OnBoard LED

If your Arduino got an OnBoard LED, but on a different pin, you need to change that

Additionally, Arduino will use:
* PIN 12 as LED Port
* PIN 4, PIN 5, PIN 6, PIN 7 as COMM Ports


### Development
Want to contribute? Great!
Just clone and make a pull request.
You can also come to one of the [PiAndMore] Seasons and bring with you your preconfigured Raspberry Pis or Arduinos :)!
If you want to compile the RPi Version yourself, you need to install wiringPi as well as libusbdev - you can then use the Makefile to build it.


### Todo's
* Reallife field testing

### Author
* Nico Maas

[1000fireflies.net]: http://1000fireflies.net/
[PiAndMore]: http://piandmore.de/
[here]: http://www.nico-maas.de/wordpress/?p=1103
[OpenWRT]: http://openwrt.org
[wiringPi]: http://wiringpi.com/
[Arduino]: http://www.arduino.cc/
[7zip]: http://www.7-zip.de/
[Win32 Disk Imager]: http://sourceforge.net/projects/win32diskimager/