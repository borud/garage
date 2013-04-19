/**
 * Embedded webserver for controlling a relay over the local network.
 *
 * @author borud
 */

#include <Ethernet.h>
#include <SPI.h>
#include <Flash.h>
#include <SD.h>
#include <TinyWebServer.h>

// ========================================================================
// Configurables
// ========================================================================

// Which pin to use for controlling the relay
#define RELAY_PIN 9

// The number of milliseconds the switch will be closed
#define BLIP_TIME_MILLISECONDS 200

// MAC address
static uint8_t mac[] = {0xB0, 0x0B, 0x13, 0x50, 0x01, 0x01};

// IP address
static byte ip[] = {10, 1, 0, 99};

// ========================================================================

// Forward declarations
boolean index_get_handler(TinyWebServer& webserver);
boolean garage_door_post_handler(TinyWebServer& webserver);
void blip_relay();

// Declare handlers for the various paths.  
TinyWebServer::PathHandler handlers[] = {
    {"/", TinyWebServer::GET, &index_get_handler},
    // Since the TinyWebServer does not offer any authentication you
    // should probably obfuscate these URLs and make sure that you do
    // not expose them to the outside Internet.
    {"/garageDoor", TinyWebServer::GET, &garage_door_get_handler}
};

const char* headers[] = {
    "Content-Length",
    NULL
};

TinyWebServer web = TinyWebServer(handlers, headers);

/**
 * Blip the relay to toggle garage door.
 */
boolean garage_door_get_handler(TinyWebServer& w) {
    w.send_error_code(200);
    w.send_content_type("application/json");
    w.end_headers();
    
    blip_relay();

    Client& client = w.get_client();
    client << "{\n"
	   << "  status : \"OK\",\n"
	   << "  blipTime : \"" << BLIP_TIME_MILLISECONDS << "\",\n"
	   << "  uptime : \"" << millis() << "\"\n"
	   << "}\n";
}

/**
 * Provice an index page.
 */
boolean index_get_handler(TinyWebServer& w) {
    w.send_error_code(200);
    w.send_content_type("text/html");
    w.end_headers();

    Client& client = w.get_client();
    client 
	<< "<html>\n"
	<< "<head>\n"
	<< " <title>Garage</title>\n"
	<< "</head>\n"
	<< "<body>\n"
	<< "<h1>Garage</h1>\n"
	<< "Uptime : " << millis() << " milliseconds"
	<< "</body>\n"
	<< "</html>\n"
	;
}

/**
 * Pull the RELAY_PIN high for BLIP_TIME_MILLISECONDS.
 */
void blip_relay() {
    digitalWrite(RELAY_PIN, HIGH);
    delay(BLIP_TIME_MILLISECONDS);
    digitalWrite(RELAY_PIN, LOW);
}

/**
 *
 */
void setup() {
    Serial.begin(9600);
    Serial << "*** Setting up IP\n";
    Ethernet.begin(mac, ip);

    // Set up the output pin
    Serial << "*** Initializing pins\n";
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);
    Serial << "*** Init done\n";
}

void loop() {
    web.process();
}

