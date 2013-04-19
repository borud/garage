# Simple garage door opener

This is the software for my garage door opener.  The system consists of:

- An [Arduino Ethernet](http://arduino.cc/en/Main/ArduinoBoardEthernet).
- A relay breakout board of some description.

### Relay

This is the relay I have: http://www.plexishop.it/en/arduino-5v-relay-module.html. I found it in the parts bin at our hackerspace so there is no special reason I chose this relay.  You can find lots of relays at [Sparkfun](http://sparkfun.com/) -- both with breakout boards and without.   If you need to build the circuitry yourself there's lots of resources online.   Just google it.

### Dependencies

In order to use the above code you need to install two extra libraries in your Arduino libraries folder:

- The [TinyWebServer](https://github.com/ovidiucp/TinyWebServer/) library 
- The [Flash](http://arduiniana.org/libraries/flash/) library (look for download link a bit down the page)

If you cannot remember how to install libraries in your Arduino setup there is a handy guide in the official [Arduino documentation on Libraries](http://arduino.cc/en/Guide/Libraries).

