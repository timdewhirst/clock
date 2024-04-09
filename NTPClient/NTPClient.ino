
// std
#include <array>
#include <cstdint>
#include <tuple>
#include <time.h>

// arduino
#include <MATRIX7219.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

// local
#include "Display.hpp"
#include "NTP.hpp"

#ifndef STASSID
#define STASSID "BT-Q8AK8R"
#define STAPSK "AgaKBUgL3c@r$4waP"
#endif

const char* ssid = STASSID;  // your network SSID (name)
const char* pass = STAPSK;   // your network password

// keep track of time
uint64_t ntp_time_ms {};
uint64_t last_update_ms {};

unsigned int localPort = 2390;  // local port to listen for UDP packets
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48;  // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE];  // buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

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
  packetBuffer[12] = 't';
  packetBuffer[13] = 'i';
  packetBuffer[14] = 'm';
  packetBuffer[15] = 'e';

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123);  // NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

bool get_ntp_time()
{
  IPAddress timeServerIP;
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP);
  
  sendNTPpacket(timeServerIP);  // send an NTP packet to a time server
  
  int cb = 0;
  int wait_count = 0;
  while (!cb && wait_count < 1000)
  {
    ++wait_count;
    delay(10); 
    cb = udp.parsePacket();
  }
  
  if (!cb) 
    return false;
  
  // We've received a packet, read the data from it
  udp.read(packetBuffer, NTP_PACKET_SIZE);  // read the packet into the buffer

  // decode the packet: see https://www.rfc-editor.org/rfc/rfc5905.html
  uint64_t secsSince1900 = read_be32(&packetBuffer[40]);
  uint64_t fractional = read_be32(&packetBuffer[44]);

  // now convert NTP time into everyday time:
  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
  const uint64_t seventyYears = 2208988800UL;
  ntp_time_ms = (secsSince1900 - seventyYears) * 1000;
  ntp_time_ms += fractional / 1'000'000;
  last_update_ms = millis();

  Serial.print("packet received, length: ");
  Serial.println(cb);
  Serial.print("Seconds since Jan 1 1900: ");
  Serial.print(secsSince1900);
  Serial.print(".");
  Serial.println(fractional);
  Serial.print("NTP time ms: ");
  Serial.println(ntp_time_ms);

  return true;
}

static const int analogue_pin = A0;
static const int matrix_data_pin = D7;
static const int matrix_select_pin = D8;
static const int matrix_clock_pin = D5;
static const int matrix_count = 5;
static const int switch_pin = D1;

// monitor for switch press
struct Button {
  uint32_t numberKeyPresses;
  bool pressed;
};

volatile Button button = {0, false};

void ICACHE_RAM_ATTR isr() 
{
  if ( button.pressed )
    return;
    
  button.numberKeyPresses++;
  button.pressed = true;
}

// matrix interface
MATRIX7219 matrix(
  matrix_data_pin,
  matrix_select_pin,
  matrix_clock_pin,
  matrix_count
  );

// display abstraction over matrix
Display display(matrix);

// keep track of how long since last NTP query
int ntp_query_time = -1;

// default brightness: 0..15
int brightness = 3;

/// check to see if this is either:
/// - the first call, or
/// - 10s has elapsed
bool should_poll_ntp(int now)
{
  if ( ntp_query_time == -1 )
    return true;

  // 100s polling time
  if ( (now - ntp_query_time) > 100'000 )
    return true; 

  return false;
}

/// split number into tens, units
constexpr std::tuple<uint8_t, uint8_t> split(uint8_t n)
{
  return { n/10, n%10 };
}

///
void setup() 
{ 
  pinMode(switch_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(switch_pin), isr, FALLING);
  
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
    display.add_char(s_fullstop);
    display.render();
  }
  Serial.println("");

  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());

  get_ntp_time();
}

///
void loop() 
{  
  if (button.pressed) 
  {
      Serial.printf("Button has been pressed %u times\n", button.numberKeyPresses);
      button.pressed = false;
  }
  
  // update the clock
  int now = millis();

  // convert to tm and update current_time
  time_t epoch = (ntp_time_ms + now - last_update_ms)/1000;
  tm time_;
  localtime_r(&epoch, &time_);

  // adjust for daylight savings
  if (time_.tm_isdst == 0)
    time_.tm_hour += 1;

  // read & set brightness - maps from analogue_pin to 0..15
  int a = analogRead(analogue_pin) / 64;
  if ( a != brightness )
  {
    brightness = a;
    matrix.setBrightness(a);
  }

  // build string - right to left
  display.clear();
  auto [st, su] = split(time_.tm_sec);
  display.add_char(digits[su]);
  display.add_char(digits[st]);
  display.add_char(s_colon);
  auto [mt, mu] = split(time_.tm_min);
  display.add_char(digits[mu]);
  display.add_char(digits[mt]);
  display.add_char(s_colon);
  auto [ht, hu] = split(time_.tm_hour);
  display.add_char(digits[hu]);
  display.add_char(digits[ht]);
  display.render();

  delay(10);
}
