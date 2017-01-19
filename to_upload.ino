// Temperature Monitoring System
// AUTHORS STANLEY(s151359) C.N, SELVANUR O.(s123636)
// 19/01/2017

/* This program herein is going to monitor an indoor temperature and upload the readings to Thingspeak for visualization*/

// IMPORTANT! TZ_adjust=1;d=$(date +%s);t=$(echo "60*60*$TZ_adjust/1" | bc);echo T$(echo $d+$t | bc ) > /dev/ttyACM0 (PUT THIS SERIAL IN YOUR COMMAND PROMPT)

#include <TimeLib.h>
#include <Ethernet.h>
#include <ThingSpeak.h>
#include <SPI.h>
#include <Ethernet.h>
#include <LiquidCrystal.h>
#include <OneWire.h>
#include <DallasTemperature.h>


#define TEMP_SENSOR 7 //pin for the temperature sensor
#define TIME_HEADER  "T"   // Header tag for serial time sync message
#define TIME_REQUEST  7    // ASCII bell character requests a time sync message


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


const char* server = "api.thingspeak.com";
unsigned long myChannelNumber = 213208; // channel ID fromthingspeak
const char* myWriteAPIKey = "BSJOCU6YWWTGS1HI"; // Write API Key

LiquidCrystal lcd(A1,A2,5,8,3,2); // LCD pins
int rgb[] = {6,9,A3}; // pins for the RGB Led
int fan = A0; // pin for the fan
float room_tempC = 24; // room temperature

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(TEMP_SENSOR);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);


//Define necessary functions

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



void processSyncMessage() {
  unsigned long pctime;
  const unsigned long DEFAULT_TIME = 1357041600; // Jan 1 2013

  if(Serial.find(TIME_HEADER)) {
     pctime = Serial.parseInt();
     if( pctime >= DEFAULT_TIME) { // check the integer is a valid time (greater than Jan 1 2013)
       setTime(pctime); // Sync Arduino clock to the time received on the serial port
     }
  }
}


time_t requestSync()
{
  Serial.write(TIME_REQUEST);  
  return 0; // the time will be sent later in response to serial mesg
}
 
void setColor(int red, int green, int blue)
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

// end of necessary functions

void setup() {
  // put your setup code here, to run once:
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
  //pinMode(fan,OUTPUT); //for the fan

  delay(4000);
  lcd.clear();



  //for date and time
  while (!Serial) ; // Needed for Leonardo only
  setSyncProvider( requestSync);  //set function to call when sync required
  Serial.println("Waiting for sync message");
  lcd.setCursor(2,0);
  lcd.print("Send the sync");
  lcd.setCursor(3,1);
  lcd.print("Message");
  
  if (Ethernet.begin(mac) == 0) {
  // no point in carrying on, so do nothing forevermore:
  while (1) {
    Serial.println("Failed to configure Ethernet using DHCP");
    delay(10000);
    }
  }
  
  ThingSpeak.begin(client);

  // end of date time

  //blink rgb led to show that connectivity was successful
  setColor(255,255,255);
  delay(1000);
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
  delay(1000);
  setColor(0,0,0);

  delay(2000);
  lcd.clear();

}

time_t prevDisplay = 0; // when the digital clock was displayed

void loop() {
  processSyncMessage();
  digitalClockDisplay();

  
  sensors.requestTemperatures(); // Send the command to get temperatures
  float current_TempC = sensors.getTempCByIndex(0); // store the temperature in centigrade
  float current_TempF = sensors.getTempFByIndex(0); // store the temperature in fahreinheit
  ThingSpeak.writeField(myChannelNumber,1,current_TempC,myWriteAPIKey);
  delay(500);
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

  
  if (current_TempC > room_tempC) {
    switch (working_TempC) {
      case 25:
      case 26:
      case 27:
      case 28:
      case 29:
      case 30:
        analogWrite(fan,120);
        setColor(0,0,0);
        setColor(0,0,255); // changing the rgb color to green
        break;
      case 31:
      case 32:
      case 33:
      case 34:
      case 35:
      case 36:
        analogWrite(fan,140);
        setColor(0,0,0);
        setColor(200,200,0); // changing the rgb color to pink
        break;
      case 37:
      case 38:
      case 39:
      case 40:
      case 41:
        analogWrite(fan,160);
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

      default:
      analogWrite(fan,210);
    }
  }
  delay(500);

}
