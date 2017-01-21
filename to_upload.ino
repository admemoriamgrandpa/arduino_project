// Temperature Monitoring System
// AUTHORS STANLEY(s151359) C.N, SELVANUR O.(s123636)
// 19/01/2017

/* This program herein is going to monitor an indoor temperature and upload the readings to Thingspeak for visualization*/

#include <TimeLib.h>
#include <Ethernet.h>
#include <ThingSpeak.h>
#include <SPI.h>
#include <EthernetUdp.h>
#include <Ethernet.h>
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>






#define TEMP_SENSOR 7 //pin for the temperature sensor






//for the date and time
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; //mac address
byte customChar[8] = {
  0b00110,
  0b01001,
  0b01001,
  0b00110,
  0b00000,
  0b00000,
  0b00000,
  0b00000
}; // for the degree symbol







EthernetClient client;
EthernetUDP Udp;




// NTP Servers:
IPAddress timeServer(132, 163, 4, 101); // time-a.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 102); // time-b.timefreq.bldrdoc.gov
// IPAddress timeServer(132, 163, 4, 103); // time-c.timefreq.bldrdoc.gov


const int timeZone = 1;     // Central European Time
//const int timeZone = -5;  // Eastern Standard Time (USA)
//const int timeZone = -4;  // Eastern Daylight Time (USA)
//const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)



unsigned int localPort = 8888;  // local port to listen for UDP packets


const char* server = "https://api.thingspeak.com/update?Q1Z8JPVXPX5THS9E&field1=1";
unsigned long myChannelNumber = 217178; // channel ID fromthingspeak
const char* myWriteAPIKey = "Q1Z8JPVXPX5THS9E"; // Write API Key



LiquidCrystal lcd(A1,A2,5,8,3,2); // LCD pins
int rgb[] = {6,A0,A3}; // pins for the RGB Led
int fan = 9; // pin for the fan
int room_tempC = 24; // room temperature
int interval = 60000; //we shall upload to thingspeak every minute
unsigned long previousMillis = 0; // keep track of the millis func





// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(TEMP_SENSOR);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);



/*-------- NTP code ----------*/

const int NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets


time_t getNtpTime()
{
  while (Udp.parsePacket() > 0) ; // discard any previously received packets
  //Serial.println("Transmit NTP Request");
  sendNTPpacket(timeServer);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = Udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      //Serial.println("Receive NTP Response");
      Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  //Serial.println("No NTP Response :-(");
  return 0; // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void sendNTPpacket(IPAddress &address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:                 
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
}


void digitalClockDisplay(){
  // digital clock display of the time
  //Serial.print(hour());
  lcd.home();
  lcd.print("TIME:");
  lcd.setCursor(5,0);
  if (hour() < 10) {
    lcd.print(0);
    lcd.setCursor(6,0);
    lcd.print(hour());
  }
  else{
    lcd.print(hour());
  }
  
  lcd.setCursor(7,0);
  lcd.print(":");
  lcd.setCursor(8,0);
  if (minute() < 10) {
    lcd.print(0);
    lcd.setCursor(9,0);
    lcd.print(minute());
  }
  else {
    lcd.print(minute());
  } 
}
/* end of NTP CODE */



void setColor(int red, int green, int blue) // set RGB led
{
  #ifdef COMMON_ANODE
    red = 255 - red;
    green = 255 - green;
    blue = 255 - blue;
  #endif
  analogWrite(rgb[0], red);
  analogWrite(rgb[1], green);
  analogWrite(rgb[2], blue);  
}





void setup() {
  Serial.begin(9600); // print to serial monitor
  sensors.begin(); // start the sensor
  lcd.createChar(0, customChar); //create the custom char defined above
  lcd.begin(16,2); // activate lcd
  
  lcd.print("PROJECT BY:");
  lcd.setCursor(0,1);// place the cursor at the second row
  lcd.print("STANLEY&SELVANUR");
  pinMode(rgb[0], OUTPUT); // blue
  pinMode(rgb[1], OUTPUT); // green
  pinMode(rgb[2], OUTPUT); // red
  pinMode(fan,OUTPUT); //for the fan
  analogWrite(fan,0);

  delay(4000);
  lcd.clear();



  //for date and time
  while (!Serial) ; // Needed for Leonardo only
  delay(250);
  //uncomment below to view on serial monitor
  //Serial.println("TimeNTP Example");
  lcd.print("TimeNTP Example");
  delay(2000);
  lcd.clear();
  
  if (Ethernet.begin(mac) == 0) {
  // no point in carrying on, so do nothing forevermore:
  while (1) {
    // uncomment the following to view on serial monitor
    //Serial.println("Failed to configure Ethernet using DHCP");
    lcd.setCursor(1,0);
    lcd.print("No internet");
    lcd.setCursor(2,1);
    lcd.print("Connection");
    delay(10000);
    lcd.clear();
    }
  }


  Udp.begin(localPort);
  setSyncProvider(getNtpTime);
  
  ThingSpeak.begin(client);


  // blinking the LED to show successful connection;
  setColor(200,200,0); //pink
  delay(1000);
  setColor(0,0,0);
  setColor(0,255,0); //red
  delay(1000);
  setColor(0,0,0);
  setColor(255,0,0); //blue
  delay(1000);
  setColor(0,0,0);
  setColor(0,0,255); //green
  delay(1000);
  setColor(0,0,0);
  setColor(0,190,150); //yellow
  delay(1000);
  setColor(0,0,0);
  setColor(255,255,255); //white
  delay(1000);

}

time_t prevDisplay = 0; // when the digital clock was displayed

void loop() {

  unsigned long currentMillis = millis();
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) { //update the display only if time has changed
      prevDisplay = now();
      digitalClockDisplay();  
    }
  }

  
  sensors.requestTemperatures(); // Send the command to get temperatures
  float current_TempC = sensors.getTempCByIndex(0); // store the temperature in centigrade
  float current_TempF = sensors.getTempFByIndex(0); // store the temperature in fahreinheit

  

  int working_TempC = int(current_TempC); // will be used to adjust the fan
  setColor(255,255,255);
  lcd.setCursor(0,1);
  lcd.print("TEMP:");
  lcd.setCursor(5,1);
  lcd.print(current_TempC);
  lcd.setCursor(10,1);
  lcd.write((uint8_t)0);
  lcd.setCursor(11,1);
  lcd.print("C");

  
  if (working_TempC > room_tempC) {
    switch (working_TempC) {
      case 25:
      case 26:
      case 27:
      case 28:
      case 29:
      case 30:
        analogWrite(fan,80);
        setColor(0,0,0);
        setColor(0,0,255); // changing the rgb color to green
        break;
      case 31:
      case 32:
      case 33:
      case 34:
      case 35:
      case 36:
        analogWrite(fan,100);
        setColor(0,0,0);
        setColor(200,200,0); // changing the rgb color to pink
        break;
      case 37:
      case 38:
      case 39:
      case 40:
      case 41:
        analogWrite(fan,120);
        setColor(0,0,0);
        setColor(255,0,0); // changing the rgb color to blue
        break;
      case 42:
      case 43:
      case 44:
      case 45:
      case 46:
        analogWrite(fan,170);
        setColor(0,0,0);
        setColor(0,0,255); // changing the rgb color to yellow
        break;
      case 47:
      case 48:
      case 49:
      case 50:
      case 51:
        analogWrite(fan,190);
        setColor(0,0,0);
        setColor(0,255,0); // changing the rgb color to red
        break;
      case 52:
      case 53:
      case 54:
      case 55:
      case 56:
      case 57:
      case 58:
      case 59:
      case 60:
        analogWrite(fan,210);
        setColor(0,0,0);  // keep blinking!
        setColor(200,200,0); //pink
        delay(1000);
        setColor(0,0,0);
        setColor(0,255,0); //red
        delay(1000);
        setColor(0,0,0);
        setColor(255,0,0); //blue
        delay(1000);
        setColor(0,0,0);
        setColor(0,0,255); //green
        delay(1000);
        setColor(0,0,0);
        setColor(0,50,70); //yellow
        delay(1000);
        setColor(0,0,0);
        setColor(255,255,255); //white
        break;

      case 61:
      case 62:
      case 63:
      case 64:
      case 65:
      case 66:
      case 67:
      case 68:
      case 69:
      case 70:
      case 71:
      case 72:
      case 73:
      case 74:
      case 75:
      case 76:
      case 77:
      case 78:
      case 79:
      case 80:
      case 81:
      case 82:
      case 83:
      case 84:
      case 85:
      case 86:
      case 87:
      case 88:
      case 89:
      case 90:
      case 91:
      case 92:
      case 93:
      case 94:
      case 95:
      case 96:
      case 97:
      case 98:
      case 99:
      case 100:
        setColor(0,0,0);  
        setColor(0,255,0); //switch on the red line to indicate danger
        break;
        

      default:
      analogWrite(fan,0);
    }
  }

  else {
    analogWrite(fan,0);
  }

  if ((unsigned long)(currentMillis - previousMillis) >= interval) { // upload to thingspeak every minute
    ThingSpeak.writeField(myChannelNumber,1,current_TempC,myWriteAPIKey);
    //track next event
    previousMillis = currentMillis;
  }
  
}
