/** -*- mode: c; c-basic-offset: 4 -*-
 * 
 * Simple webserver based interface for garage door.
 *
 * This sketch contains some code which updates the internal clock of
 * the Arduino.  For now that functionality isn't terribly useful
 * since it only uses the clock to display the (UNIX) time.  However
 * the plan is to add some sensors and log data from them, so I added
 * the NTP code now so I can test it over time to see that it works
 * well.
 *
 * @author borud
 * 
 */

// This needs to be defined before WebServer.h is included
#define WEBDUINO_AUTH_REALM "Garage"

#include <Time.h>
#include <SPI.h>
#include <Ethernet.h>
#include <WebServer.h>
#include <EthernetUdp.h>

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

// Username and password is admin:admin. Change this to something else
#define USER_CREDENTIALS_1 "YWRtaW46YWRtaW4="

// NTP server 158.38.0.168 (oliven.uninett.no).  You should change this to
// something appropriate for where you are in the world.
static IPAddress timeServer(158,38,0,168);

// Number of milliseconds between each time we poll the NTP server.
// (we assume that 15 minutes is probably a good value)
const unsigned long NTP_POLL_INTERVAL = (15 * 60 * 1000);

// ========================================================================

const int NTP_UDP_PORT = 9988;
const int NTP_PACKET_SIZE = 48;
const int timezone = 1;
static byte packetBuffer[ NTP_PACKET_SIZE];
static EthernetUDP Udp;

// Milliseconds since last NTP poll.  Initialized to
// -NTP_POLL_INTERVAL so that we poll right away after reboot.
static unsigned long last_ntp_poll_millis = -NTP_POLL_INTERVAL;

// Keep track of how much clock drift there was the last time we set the clock
static time_t last_poll_clock_drift = 0; 

// Stream operator
template<class T>
inline Print &operator <<(Print &obj, T arg)
{ obj.print(arg); return obj; }

static WebServer webserver(PREFIX, 80);

/**
 * Pull the RELAY_PIN high for BLIP_TIME_MILLISECONDS.
 */
void blip_relay() {
    digitalWrite(RELAY_PIN, HIGH);
    delay(BLIP_TIME_MILLISECONDS);
    digitalWrite(RELAY_PIN, LOW);
}

/**
 * Send NTP query if NTP_POLL_INTERVAL milliseconds since last poll
 */
void possibly_send_ntp_poll() {
    unsigned long now_millis = millis();
    if (now_millis > (last_ntp_poll_millis + NTP_POLL_INTERVAL)) {
        memset(packetBuffer, 0, NTP_PACKET_SIZE);
        packetBuffer[0] = 0b11100011;
        packetBuffer[1] = 0;
        packetBuffer[2] = 6;
        packetBuffer[3] = 0xEC;
        packetBuffer[12]  = 49; 
        packetBuffer[13]  = 0x4E;
        packetBuffer[14]  = 49;
        packetBuffer[15]  = 52;

        Udp.beginPacket(timeServer, 123); //NTP requests are to port 123
        Udp.write(packetBuffer,NTP_PACKET_SIZE);
        Udp.endPacket(); 
        last_ntp_poll_millis = now_millis;
        Serial << "*** Sent request to NTP server\n";
    }
}

/**
 * Check if we have received an UDP packet.  If so we try to parse it
 * as an NTP packet.
 */
void possibly_parse_ntp_packet() {
    int size = Udp.parsePacket();
    if (size < NTP_PACKET_SIZE) {
        return;
    }

    Udp.read(packetBuffer, NTP_PACKET_SIZE);
    unsigned long seconds_since_1900;
    seconds_since_1900 =  (unsigned long)packetBuffer[40] << 24;
    seconds_since_1900 |= (unsigned long)packetBuffer[41] << 16;
    seconds_since_1900 |= (unsigned long)packetBuffer[42] << 8;
    seconds_since_1900 |= (unsigned long)packetBuffer[43];
    time_t t = seconds_since_1900 - 2208988800UL * SECS_PER_HOUR;

    last_poll_clock_drift = now() - t;
    setTime(t);
      
    Serial << "*** Set time to "
           << t
           << ", drift was "
           << last_poll_clock_drift
           << "\n";
}

/**
 * Default handler for webserver.
 */
void default_cmd(WebServer &server, WebServer::ConnectionType type, char *, bool) {
    server.httpSuccess();
    if (type != WebServer::HEAD) {
        server << "<h1>Garage</h1>\n"
               << "<p><b>Uptime</b>: " << millis() << "\n"
               << "<p><b>Time</b>: " << now() << "\n"
               << "<p><b>Last Clock Drift</b>: " << last_poll_clock_drift << "\n"
               << "<p><a href=\"/garageDoor\">Garage door</a>"
            ;
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
	server << "{\n"
               << "  status : \"OK\",\n"
               << "  time: \"" << now() << "\",\n"
               << "  blipTime : \"" << BLIP_TIME_MILLISECONDS << "\",\n"
               << "  uptime : \"" << millis() << "\"\n"
               << "}\n"
            ;
    }
}

/**
 * Set up ethernet and webserver.
 */
void setup() {
    // Conservative setting
    Serial.begin(9600);

    // Set up the output pin
    Serial << "*** Initializing pins\n";
    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW);

    // Configure ethernet
    Serial << "*** Initializing Ethernet\n";
    Ethernet.begin(mac, ip);

    // Configure webserver
    Serial << "*** Initializing Webserver\n";
    webserver.setDefaultCommand(&default_cmd);
    webserver.addCommand("garageDoor", &garage_door_cmd);
    webserver.begin();

    // Initialize UDP socket
    Serial << "*** Setting up NTP socket, listening to "
           << NTP_UDP_PORT
           << ", poll interval "
           << NTP_POLL_INTERVAL
           << "\n";
    Udp.begin(NTP_UDP_PORT);

    Serial << "*** Done initializing\n";
}

/**
 * Main loop.
 */
void loop() {
    int len = 128;
    char buff[len];

    webserver.processConnection(buff, &len);
    possibly_send_ntp_poll();
    possibly_parse_ntp_packet();
}
