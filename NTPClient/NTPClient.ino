#include <MATRIX7219.h>

/*

  Udp NTP Client

  Get the time from a Network Time Protocol (NTP) time server
  Demonstrates use of UDP sendPacket and ReceivePacket
  For more on NTP time servers and the messages needed to communicate with them,
  see http://en.wikipedia.org/wiki/Network_Time_Protocol

  created 4 Sep 2010
  by Michael Margolis
  modified 9 Apr 2012
  by Tom Igoe
  updated for the ESP8266 12 Apr 2015
  by Ivan Grokhotkov

  This code is in the public domain.

*/

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#ifndef STASSID
#define STASSID "BT-Q8AK8R"
#define STAPSK "AgaKBUgL3c@r$4waP"
#endif

const char* ssid = STASSID;  // your network SSID (name)
const char* pass = STAPSK;   // your network password


unsigned int localPort = 2390;  // local port to listen for UDP packets

/* Don't hardwire the IP address or we won't get the benefits of the pool.
    Lookup the IP address for the host name instead */
// IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP;  // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48;  // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[NTP_PACKET_SIZE];  // buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

static const int analogue_pin = A0;
static const int matrix_data_pin = D7;
static const int matrix_select_pin = D8;
static const int matrix_clock_pin = D5;
static const int matrix_count = 4;

MATRIX7219 matrix(
  matrix_data_pin,
  matrix_select_pin,
  matrix_clock_pin,
  matrix_count
  );

// keep track of how long since last NTP query
int ntp_query_time = -1;

// default brightness: 0..15
int brightness = 3;

bool should_poll_ntp(int now)
{
  if ( ntp_query_time == -1 )
    return true;

  // 10s polling time
  if ( (now - ntp_query_time) > 10'000 )
    return true; 

  return false;
}

void setup() 
{
  // setup matrix
  matrix.begin();
  matrix.clear();
  
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  // We start by connecting to a WiFi network
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");

  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());
}

void loop() 
{  
  int a = analogRead(analogue_pin) / 64;
  Serial.print("brightness: ");
  Serial.println(a);

  if ( a != brightness )
  {
    brightness = a;
    matrix.setBrightness(a);
  }
  matrix.setRow(1, B11111111, 1);
  matrix.setRow(2, B10111101, 2);
  matrix.setRow(3, B11011011, 3);
  matrix.setRow(4, B11100111, 4);

  // check if we need to update NTP
  int now = millis();
  if ( should_poll_ntp(now) )
  {
    ntp_query_time = now;
    
    // get a random server from the pool
    WiFi.hostByName(ntpServerName, timeServerIP);
  
    sendNTPpacket(timeServerIP);  // send an NTP packet to a time server
  
    int cb = 0;
    int wait_count = 0;
    while (!cb && wait_count < 20)
    {
      ++wait_count;
      delay(100); 
      cb = udp.parsePacket();
    }
    
    if (!cb) 
    {
      Serial.println("no packet yet");
      delay(2000);
    } 
    else 
    {
      Serial.print("packet received, length=");
      Serial.println(cb);
      // We've received a packet, read the data from it
      udp.read(packetBuffer, NTP_PACKET_SIZE);  // read the packet into the buffer
  
      // the timestamp starts at byte 40 of the received packet and is four bytes,
      //  or two words, long. First, extract the two words:
  
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      // combine the four bytes (two words) into a long integer
      // this is NTP time (seconds since Jan 1 1900):
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      Serial.print("Seconds since Jan 1 1900 = ");
      Serial.println(secsSince1900);
  
      // now convert NTP time into everyday time:
      Serial.print("Unix time = ");
      // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
      const unsigned long seventyYears = 2208988800UL;
      // subtract seventy years:
      unsigned long epoch = secsSince1900 - seventyYears;
      // print Unix time:
      Serial.println(epoch);
  
  
      // print the hour, minute and second:
      Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
      Serial.print((epoch % 86400L) / 3600);  // print the hour (86400 equals secs per day)
      Serial.print(':');
      if (((epoch % 3600) / 60) < 10) {
        // In the first 10 minutes of each hour, we'll want a leading '0'
        Serial.print('0');
      }
      Serial.print((epoch % 3600) / 60);  // print the minute (3600 equals secs per minute)
      Serial.print(':');
      if ((epoch % 60) < 10) {
        // In the first 10 seconds of each minute, we'll want a leading '0'
        Serial.print('0');
      }
      Serial.println(epoch % 60);  // print the second
    }
  }
  
  delay(100);
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress& address) 
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;  // LI, Version, Mode
  packetBuffer[1] = 0;           // Stratum, or type of clock
  packetBuffer[2] = 6;           // Polling Interval
  packetBuffer[3] = 0xEC;        // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123);  // NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
