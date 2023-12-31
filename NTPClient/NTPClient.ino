
// std
#include <array>
#include <cstdint>
#include <tuple>

// arduino
#include <MATRIX7219.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>

#ifndef STASSID
#define STASSID "BT-Q8AK8R"
#define STAPSK "AgaKBUgL3c@r$4waP"
#endif

const char* ssid = STASSID;  // your network SSID (name)
const char* pass = STAPSK;   // your network password

// keep track of time
struct Time
{
  uint8_t h {};
  uint8_t m {};
  uint8_t s {};
} 
current_time;
int last_update_ms = -1;

unsigned int localPort = 2390;  // local port to listen for UDP packets
const char* ntpServerName = "time.nist.gov";

const int NTP_PACKET_SIZE = 48;  // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[NTP_PACKET_SIZE];  // buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

bool get_ntp_time()
{
  IPAddress timeServerIP;
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP);
  
  sendNTPpacket(timeServerIP);  // send an NTP packet to a time server
  
  int cb = 0;
  int wait_count = 0;
  while (!cb && wait_count < 10)
  {
    ++wait_count;
    delay(100); 
    cb = udp.parsePacket();
  }
  
  if (!cb) 
    return false;
  
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
  return true;
}

static const int analogue_pin = A0;
static const int matrix_data_pin = D7;
static const int matrix_select_pin = D8;
static const int matrix_clock_pin = D5;
static const int matrix_count = 5;

/// masks: index is the mask width i.e.
/// - index 0 -> 0b00000000
/// - index 2 -> 0b00000011
static const uint8_t masks[] = { 
  0b00000000,
  0b00000001,
  0b00000011,
  0b00000111,
  0b00001111,
  0b00011111,
  0b00111111,
  0b01111111,
  0b11111111
};

static const uint8_t display_height = 8;

/// representation of a char; max size is 8x8
struct Char
{
  using data_t = std::array<uint8_t, display_height>;
  
  Char(uint8_t width_, data_t&& data_)
    : width(width_)
    , data(data_)
  {}

  // returns data for row masked according to width
  uint8_t row(uint8_t r) const
  {
    return data[r] & masks[width];  
  }

  const uint8_t width;
  const data_t data;
};

const static Char s_colon( 
  2, 
  {0b00000000,
   0b00000000,
   0b00000001,
   0b00000001,
   0b00000000,
   0b00000001,
   0b00000001,
   0b00000000});
const static Char s_fullstop( 
  1, 
  {0b00000000,
   0b00000000,
   0b00000000,
   0b00000000,
   0b00000000,
   0b00000000,
   0b00000001,
   0b00000000}); 
const static Char s_one( 
  5, 
  {0b00000000,
   0b00000010,
   0b00000110,
   0b00000010,
   0b00000010,
   0b00000010,
   0b00000111,
   0b00000000}); 
const static Char s_two( 
  5, 
  {0b00000000,
   0b00000110,
   0b00001001,
   0b00000001,
   0b00000010,
   0b00000100,
   0b00001111,
   0b00000000}); 
const static Char s_three( 
  5, 
  {0b00000000,
   0b00000110,
   0b00001001,
   0b00000010,
   0b00000001,
   0b00001001,
   0b00000110,
   0b00000000}); 
const static Char s_four( 
  5, 
  {0b00000000,
   0b00000010,
   0b00000110,
   0b00001010,
   0b00001111,
   0b00000010,
   0b00000010,
   0b00000000}); 
const static Char s_five( 
  5, 
  {0b00000000,
   0b00001111,
   0b00001000,
   0b00001110,
   0b00000001,
   0b00001001,
   0b00000110,
   0b00000000}); 
const static Char s_six( 
  5, 
  {0b00000000,
   0b00000110,
   0b00001000,
   0b00001110,
   0b00001001,
   0b00001001,
   0b00000110,
   0b00000000}); 
const static Char s_seven( 
  5, 
  {0b00000000,
   0b00001111,
   0b00000001,
   0b00000010,
   0b00000010,
   0b00000100,
   0b00000100,
   0b00000000}); 
const static Char s_eight( 
  5, 
  {0b00000000,
   0b00000110,
   0b00001001,
   0b00000110,
   0b00001001,
   0b00001001,
   0b00000110,
   0b00000000}); 
const static Char s_nine( 
  5, 
  {0b00000000,
   0b00000110,
   0b00001001,
   0b00001001,
   0b00000111,
   0b00000001,
   0b00000110,
   0b00000000}); 
const static Char s_zero( 
  5, 
  {0b00000000,
   0b00000110,
   0b00001001,
   0b00001001,
   0b00001001,
   0b00001001,
   0b00000110,
   0b00000000}); 

static const Char digits[] = {
  s_zero,
  s_one,
  s_two,
  s_three,
  s_four,
  s_five,
  s_six,
  s_seven,
  s_eight,
  s_nine
};

/// store displayable string as a series of rows,
/// left most is MSB
struct Display
{
  using data_t = std::array<uint64_t, display_height>;

  Display( MATRIX7219& m )
    : matrix(m)
  {
    matrix.begin();
    matrix.clear();  
  }
     
  /// extract data for one row in one matrix; matrix is
  /// zero indexed
  uint8_t get_matrix_row(uint8_t matrix, uint8_t row) const
  {
    uint64_t data = rows[row];
    data >>= (matrix * 8);
    return data;
  }

  void add_char(const Char& c)
  {
    for ( uint8_t r_index=0; r_index<display_height; ++r_index )
    {
      uint64_t r = c.row(r_index);
      r <<= column_offset;
      rows[r_index] |= r;
    }

    column_offset += c.width;
  }

  void clear()
  {
    rows = {};
    column_offset = 0;
  }

  void render()
  {
    for ( uint8_t m = 0; m < matrix_count; ++m )
      for ( uint8_t r = 0; r < display_height; ++r )
      {
        matrix.setRow(1 + r, get_matrix_row(m, r), 1 + m);
      }
  }

  MATRIX7219& matrix;
  data_t rows {};
  uint8_t column_offset {};
};


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

  // 10s polling time
  if ( (now - ntp_query_time) > 10'000 )
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
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  Serial.println("Starting UDP");
  udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(udp.localPort());

  last_update_ms = millis();
}

///
void loop() 
{  
  // read & set brightness - maps from analogue_pin to 0..15
  int a = analogRead(analogue_pin) / 64;
  if ( a != brightness )
  {
    brightness = a;
    matrix.setBrightness(a);
  }

  // build string - right to left
  display.clear();
  auto [st, su] = split(current_time.s);
  display.add_char(digits[su]);
  display.add_char(digits[st]);
  display.add_char(s_colon);
  auto [mt, mu] = split(current_time.m);
  display.add_char(digits[mu]);
  display.add_char(digits[mt]);
  display.add_char(s_colon);
  
  auto [ht, hu] = split(current_time.h);
  display.add_char(digits[hu]);
  display.add_char(digits[ht]);
  display.render();

  // check if we need to update NTP
  int now = millis();

  if (now - last_update_ms > 1000)
  {
    last_update_ms = now;
    current_time.s += 1;
    if ( current_time.s == 100 )
    {
      current_time.m += 1;
      current_time.s = 0;
    }
    if ( current_time.m == 100 )
    {
      current_time.h += 1;
      current_time.m = 0;
    }
  }  
  if ( should_poll_ntp(now) )
  {
    ntp_query_time = now;
    
    // get a random server from the pool
    IPAddress timeServerIP;
    WiFi.hostByName(ntpServerName, timeServerIP);
  
    sendNTPpacket(timeServerIP);  // send an NTP packet to a time server
  
    int cb = 0;
    int wait_count = 0;
    while (!cb && wait_count < 10)
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
