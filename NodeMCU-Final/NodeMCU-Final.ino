//Libararies
#define BLYNK_PRINT Serial 
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <BlynkSimpleEsp8266.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <ArduinoJson.h>


//Software Serial Decleration
SoftwareSerial mySerial(5 , 4);    // rx -D1  tx -D2 

//LiquidCrystal Display decleration
LiquidCrystal_I2C lcd(0x3F , 16 , 2);

//Wifi SSID and password
const char* ssid = "Kavindu";
const char* pass = "12345678";

//time offset
const long utcOffsetInSeconds = 3600;

//intializing udp client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", utcOffsetInSeconds);

//Blynk Template
#define BLYNK_TEMPLATE_ID "TMPL6Q06PVYmT"
#define BLYNK_TEMPLATE_NAME "IoT Smart Thief Detection System"
#define BLYNK_AUTH_TOKEN "hMN7dYn8yj20jVavrfWnJR0bjx4vMIbF"

//Telegram Bot ID and Token
#define BOTtoken "6257039167:AAE_M6jheVpvR0xKwSCjzTRZWcnt4bfTJn4"
#define CHAT_ID "880413681"

//Start WifiClient
X509List cert(TELEGRAM_CERTIFICATE_ROOT);
WiFiClientSecure client;
UniversalTelegramBot bot(BOTtoken, client);

//Define Pin Numbers
#define LightRelay 14
#define POWERLED 12
#define WIFILED 13

//Intializing parameters
String FullString;
int RelayButton = 0;
int bot_delay = 1000;
unsigned long lastTimeBotRan;


//Read Button from Blynk app
BLYNK_WRITE(V2)  
{
  RelayButton = param.asInt();

  if(RelayButton == 1){

    digitalWrite(LightRelay, HIGH);
    mySerial.write("3");
  }

  else{

    digitalWrite(LightRelay, LOW);
  }
}


//Read and Handle Message from telegram bot
void handleNewMessages(int numNewMessages) {

  for (int i=0; i<numNewMessages; i++) {
    
    String user_text = bot.messages[i].text;
    Serial.println(user_text);

    if (user_text == "/On") {

      bot.sendMessage(CHAT_ID, "Light Bulb set to ON", "");
      RelayButton = 1;
      mySerial.write("3");
      Blynk.virtualWrite(V2 , 1);
      digitalWrite(LightRelay, HIGH);
    }
    
    if (user_text == "/Off") {

      bot.sendMessage(CHAT_ID, "Light Bulb set to OFF", "");
      RelayButton = 0;
      Blynk.virtualWrite(V2 , 0);
      digitalWrite(LightRelay, LOW);
    }
    
    if (user_text == "/Stat") {
      if (digitalRead(LightRelay)){
        bot.sendMessage(CHAT_ID, "Light Bulb is ON", "");
      }
      else{
        bot.sendMessage(CHAT_ID, "Light Bulb is OFF", "");
      }
    }
  }
}


//Initializing Telegram Bot
void bot_setup()
{
  const String commands = F("["
                            "{\"command\":\"On\",  \"description\":\"Turn On the Light Bulb\"},"
                            "{\"command\":\"Off\", \"description\":\"Turn Off the Light Bulb\"},"
                            "{\"command\":\"Stat\",\"description\":\"Check Light Status\"}" 
                            "]");
  bot.setMyCommands(commands);
}



void setup() {

  pinMode(POWERLED , OUTPUT); 
  digitalWrite(POWERLED , HIGH);

  Serial.begin(115200); //Start hardware serial 
  mySerial.begin(9600); //Start Software Serial
  timeClient.begin(); // Time client
  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass); //Start Blynk 

  timeClient.setTimeOffset(19800); // Set timeclient offset to GMT +5:30
  configTime(0, 0, "pool.ntp.org");      
  client.setTrustAnchors(&cert); 

  //Start Wifi Clent
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);

  pinMode(LightRelay , OUTPUT);


  //Check Internet Connection
  int a = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
    a++;
  }


  //Print Wifi Client Details
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  delay(500);

  pinMode(WIFILED , OUTPUT);
  digitalWrite(WIFILED , HIGH);

  //Print Telegram Bot intializing message
  bot.sendMessage(CHAT_ID, "IoT Smart Thief Detection System Started!", "");
  delay(1000);

  Wire.begin(2,0);
  lcd.begin();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("ISTDSS Started");
  delay(2000);
  lcd.clear();

  bot_setup();
}
void loop() {

  timeClient.update(); //Update Current time

  //mySerial.print("3");

  String msg = mySerial.readStringUntil('\r'); //Read Serial Communication

  if(msg != NULL){

    delay(100);
    Serial.println(msg);
    delay(100);
  }


  if (msg == "1"){

    FullString = String(timeClient.getFormattedTime()) + String(" ---> ") + String("A Suspicious Motion Detected!");

    Blynk.virtualWrite(V0 , 1);
    Blynk.virtualWrite(V3 , FullString);
    Blynk.logEvent("motion_detected"); //Trigger Blynk Event
    bot.sendMessage(CHAT_ID, FullString , ""); //Send Telegram message
    lcd.setCursor(0,0);
    lcd.print("Motion Detected"); //Print Lcd Display
    delay(2000);
    lcd.clear();
  }

  else{

    Blynk.virtualWrite(V0 , 0);
  }

    if (msg == "2"){

    FullString = String(timeClient.getFormattedTime()) + String(" ---> ") + String("A Suspicious Sound Detected!");

    Blynk.virtualWrite(V1 , 1);
    Blynk.virtualWrite(V3 , FullString);
    Blynk.logEvent("sound_detected"); //Triger  Blynk Event
    bot.sendMessage(CHAT_ID, FullString , ""); //Send Telegram Bot message
    lcd.print("Sound Detected");
    delay(2000);
    lcd.clear();
  }

  else{

    Blynk.virtualWrite(V1 , 0);
  }


  //Handle commands from Telegram app
  if (millis() > lastTimeBotRan + bot_delay)  {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

    while(numNewMessages) {
      Serial.println("Got Response!");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }

  Blynk.run(); //Run Blynk
}
