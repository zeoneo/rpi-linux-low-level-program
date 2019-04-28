# Low Level Coding in Raspberry Pi with Raspbian.

This is not bare metal tutorial but I try to directly access physical memory using memory maps. It needs sudo permissions.
I mostly use this code to debug problems in my raspberry pi bare metal programs.

Here I have attached working code for serial communication usign rpi 3 uart.

I have attached my cmdline.txt and config.txt files so that it is easy for everyone to run this code and see proper output.




Compile the code in Raspberry pi using

    gcc -o uart.out uart.c

Connect your serial cable or usb to ttl connector to your computer/laptop.

Open serail terminal programs like hyperterminal/putty on windows
or coolterm, minicom in MacOsx.

Configurations are
* Buad Rate: 115200
* 8 bits
* No parity
* No flow control 

Run the executable using super user permissions

    sudo ./uart.out


Code is referred from following sites.
1. http://wiki.osdev.org/ARM_RaspberryPi_Tutorial_C
2. http://www.pieter-jan.com/node/15
3. https://www.iot-programmer.com/index.php/books/22-raspberry-pi-and-the-iot-in-c/chapters-raspberry-pi-and-the-iot-in-c/59-raspberry-pi-and-the-iot-in-c-memory-mapped-gpio
