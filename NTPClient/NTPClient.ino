
// std
#include <array>
#include <cstdint>
#include <tuple>
#include <time.h>

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

constexpr uint16_t read_be16(byte* d)
{
  uint16_t result = uint16_t{ d[0] } << 8;
  result |= d[1];

  return result;
}

constexpr uint32_t read_be32(byte* d)
{
  uint32_t result = uint32_t{ read_be16(d) } << 16;
  d += 2;
  result |= uint32_t{ read_be16(d) };

  return result;
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
  uint32_t secsSince1900 = read_be32(&packetBuffer[40]);
  uint32_t fractional = read_be32(&packetBuffer[44]);

  // now convert NTP time into everyday time:
  // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
  const unsigned long seventyYears = 2208988800UL;
  time_t epoch = secsSince1900 - seventyYears;

  // convert to tm and update current_time
  tm time_;
  localtime_r(&epoch, &time_);
  current_time.h = time_.tm_hour;
  current_time.m = time_.tm_min;
  current_time.s = time_.tm_sec;

  // store fractional part

  Serial.print("packet received, length: ");
  Serial.println(cb);
  Serial.print("Seconds since Jan 1 1900: ");
  Serial.print(secsSince1900);
  Serial.print(".");
  Serial.println(fractional);
  Serial.println(millis());
  Serial.print("Unix time: ");
  Serial.println(epoch);

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

  get_ntp_time();

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

  // update the clock
  int now = millis();

  if (now - last_update_ms > 1000)
  {
    last_update_ms = now;
    current_time.s += 1;
    if ( current_time.s == 60 )
    {
      current_time.m += 1;
      current_time.s = 0;
    }
    if ( current_time.m == 60 )
    {
      current_time.h += 1;
      current_time.m = 0;
    }
    if ( current_time.h == 24 )
    {
      current_time.h = 0;
      current_time.m = 0;
      current_time.s = 0;
    }
  }  
  
  delay(10);
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
