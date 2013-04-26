/** -*- mode: c; c-basic-offset: 4 -*-
 * 
 * Simple webserver based interface for garage door.
 *
 * @author borud
 * 
 */

// This needs to be defined before WebServer.h is included
#define WEBDUINO_AUTH_REALM "Garage"

#include <SPI.h>
#include <Ethernet.h>
#include <WebServer.h>

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

// URL path prefix
#define PREFIX ""

// ========================================================================

// Username and password is admin:admin. Change this to something else
#define USER_CREDENTIALS_1 "YWRtaW46YWRtaW4="

WebServer webserver(PREFIX, 80);

/**
 * Pull the RELAY_PIN high for BLIP_TIME_MILLISECONDS.
 */
void blip_relay() {
    digitalWrite(RELAY_PIN, HIGH);
    delay(BLIP_TIME_MILLISECONDS);
    digitalWrite(RELAY_PIN, LOW);
}

/**
 * Default handler for webserver.
 */
void default_cmd(WebServer &server, WebServer::ConnectionType type, char *, bool) {
    server.httpSuccess();
    if (type != WebServer::HEAD) {
	server.print("<h1>Garage</h1>\n");
	server.print("Uptime: ");
	server.print(millis());
	server.print("\n");
	server.print("<p><a href=\"/garageDoor\">Garage door</a>");
    }
}

/**
 * /garageDoor handler
 */
void garage_door_cmd(WebServer& server, WebServer::ConnectionType type, char *, bool) {
    // If the credentials are not correct we should bail
    if (! server.checkCredentials(USER_CREDENTIALS_1)) {
	server.httpUnauthorized();
	return;
    }

    server.httpSuccess("application/json;charset=utf-8", NULL);
    if (type != WebServer::HEAD) {
	blip_relay();
	server.print("{\n"
		     "  status : \"OK\",\n"
		     "  blipTime : \"");
	server.print(BLIP_TIME_MILLISECONDS);
	server.print("\",\n"
		     "  uptime : \"");
	server.print(millis());
	server.print("\"\n}\n");
    }
}

/**
 * Set up ethernet and webserver.
 */
void setup() {
    // Conservative setting
    Serial.begin(9600);

    // Set up the output pin
    Serial.println("*** Initializing pins");
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    // Configure ethernet
    Serial.println("*** Initializing Ethernet");
    Ethernet.begin(mac, ip);

    // Configure webserver
    Serial.println("*** Initializing Webserver");
    webserver.setDefaultCommand(&default_cmd);
    webserver.addCommand("garageDoor", &garage_door_cmd);
    webserver.begin();

    Serial.println("*** Done initializing");
}

/**
 * Main loop.
 */
void loop() {
    int len = 128;
    char buff[len];

    webserver.processConnection(buff, &len);
}
