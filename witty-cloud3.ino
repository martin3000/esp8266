// in arduino IDE, select NodeMCU 1.0 (ESP12E) module
// siehe https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/
// https://arduino-esp8266.readthedocs.io/en/latest/libraries.html#esp-specific-apis
// Wifi -> https://www.bakke.online/index.php/2017/05/21/reducing-wifi-power-consumption-on-esp8266-part-2/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <time.h>
#include <TZ.h>

#define TEST !true
#define RED_LED D8
#define GREEN_LED D6
#define BLUE_LED D7
#define CONTROL_LED D4 //Blaue SMD LED am ESP Modul
#define deepSleepSeconds 3000    //max. 12000s
#define MYTZ TZ_Europe_Berlin
#define MUELL_GRUEN 88
#define MUELL_SCHWARZ 99
#define MUELL_BRAUN 101
#define ABHOLTAG 3          /* 1=Montag, 2=Dienstag....Sonntag geht nicht */

const char compile_date[] = __DATE__ " " __TIME__;
//const char *WIFI_SSID = "Wohnzimmer"; // Change to your wifi network name
//const char *WIFI_PWD = "";
const char *WIFI_SSID = "UPC993324"; // Change to your wifi network name
const char *WIFI_PWD = "der hase hoppelt auf dem kartoffelfeld";      // Change to your wifi password

const char *TIME_SERVER = "pool.ntp.org";
const int fotoZellePin = A0; //Bestimmt das der Fotowiderstand an den analogen PIN A0 angeschlossen ist
const int EPOCH_1_1_2019 = 1546300800; //1546300800 =  01/01/2019 @ 12:00am (UTC)
const int SECONDS_PER_WEEK = 3600 * 24 * 7;
const int BUTTON = 4;

int muellFarbe = 0;
boolean buttonPressed = false;  //stop-knopf gedrückt oder nicht

void listNetworks() {
  // scan for nearby networks:
  Serial.println("** Scan Networks **");
  int numSsid = WiFi.scanNetworks();

  // print the list of networks seen:
  Serial.print("number of available networks:");
  Serial.println(numSsid);

  // print the network number and name for each network found:
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    Serial.print(thisNet);
    Serial.print(") ");
    Serial.print(WiFi.SSID(thisNet));
    Serial.print("\tSignal: ");
    Serial.print(WiFi.RSSI(thisNet));
    Serial.print(" dBm");
    Serial.print("\tEncryption: ");
    Serial.println(WiFi.encryptionType(thisNet));
  }
}

String findOpenNetwork() {
  int numSsid = WiFi.scanNetworks();
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    if (WiFi.encryptionType(thisNet)==7) return WiFi.SSID(thisNet);
  }
  return "";
}


String uint64ToString(uint64_t input) {
  String result = "";
  uint8_t base = 10;

  do {
    char c = input % base;
    input /= base;

    if (c < 10)
      c += '0';
    else
      c += 'A' - 10;
    result = c + result;
  } while (input);
  return result;
}

void sleep() {
  saveButtonState( buttonPressed );

  //analogWrite(GREEN_LED, 0);
  //analogWrite(BLUE_LED, 0);
  //analogWrite(RED_LED, 255);
  //ESP.deepSleep(deepSleepSeconds*1e6,WAKE_RF_DISABLED);
  //ESP.deepSleep(deepSleepSeconds*1e6);
  //Serial.print("ds-max:"); Serial.println(uint64ToString(ESP.deepSleepMax())); -> 0x03'37'D7'FF'F9 -> dezimal 13'821'804'537
  if (TEST) ESP.deepSleep(5 * 60 * 1e6);
  //else ESP.deepSleep(ESP.deepSleepMax());
  else ESP.deepSleep(0xffffffff);
}

boolean restoreButtonState() {
  int offset_check = 0;
  int offset_data = 4;
  uint32_t data;
  uint32_t check;

  ESP.rtcUserMemoryRead(offset_check, &check, sizeof(check));
  ESP.rtcUserMemoryRead(offset_data,  &data,  sizeof(data));
  if (check == 0x44444444 and data == 1) return true; //alread initialized
  else return false;
}

/* save the button state, together with a check value of 0x44444444   */
void saveButtonState(boolean state) {
  int offset_check = 0;
  int offset_data = 4;
  uint32_t data;
  uint32_t check;
  check = 0x44444444;
  if (state == true) data = 1;
  else data = 0;
  ESP.rtcUserMemoryWrite(offset_check, &check, sizeof(check));
  ESP.rtcUserMemoryWrite(offset_data,  &data,  sizeof(data));
}

int checkMuellabfuhr(time_t tstamp) {
  struct tm *tmp ;                      // NOTE: structure tm is defined in time.h
  tmp = localtime(&tstamp);                // convert to local time and break down
  int weekNumber = (tstamp - 4 * 86400) / SECONDS_PER_WEEK; // montag, 5.1.1970 auf 0-wert setzen
  int day = tmp->tm_wday;
  int hour = tmp->tm_hour;
  int minute = tmp->tm_min;
  if ((day == ABHOLTAG and hour <= 8) or (day + 1 == ABHOLTAG and hour > 10 /*vorsicht:geht nicht mit Sonntag als Abholtag*/) or (TEST and minute < 20)) {
    if (weekNumber % 2 == 1) return MUELL_GRUEN;
    else                     return MUELL_SCHWARZ;
  } else return -1;
}

void setup() {
  // letzten zustand des buttons wiederherstellen
  buttonPressed = restoreButtonState();

  pinMode(CONTROL_LED, OUTPUT);  /* Setzt den PIN für die kleine blaue LED als Ausgabe */

  // initialize serial debug output
  Serial.begin(115200);
  Serial.setTimeout(2000);
  Serial.println();

  // set up wifi connection
  // if there is an open wifi, use this
  WiFi.persistent( false );
  listNetworks();
  String openSSID=findOpenNetwork();
  if (openSSID=="") WiFi.begin(WIFI_SSID, WIFI_PWD);
  else WiFi.begin(openSSID);
  //WiFi.begin(WIFI_SSID);
  delay(3000);

  int tries = 0;
  while (WiFi.status() != WL_CONNECTED) {
    Serial.println("wait wifi");
    digitalWrite(CONTROL_LED, LOW);
    delay(1000);
    digitalWrite(CONTROL_LED, HIGH);
    delay(1000);
    if (tries++ > 20) sleep();
  }
  Serial.println("Wifi connected to "+WiFi.SSID());


  // fetch time from time server
  configTime(MYTZ, TIME_SERVER);

  // wait for time to be fetched
  time_t now;
  while (now < EPOCH_1_1_2019)  {
    Serial.println("wait for time");
    delay(2000);
    now = time(nullptr);
    if (tries++ > 20) {
      Serial.println("time not set->sleeping");
      sleep();
    }
  }
  WiFi.disconnect();                    // save energy
  WiFi.mode( WIFI_OFF );

  WiFi.forceSleepBegin();
  delay( 1 );

  Serial.print("timestamp:"); Serial.println(now);
  Serial.print("Time:");
  Serial.println(ctime(&now));          // -> http://www.cplusplus.com/reference/ctime/time/
  struct tm *tmp ;                      // NOTE: structure tm is defined in time.h
  tmp = localtime(&now);                // convert to local time and break down
  int weekNumber = (now - 4 * 86400) / SECONDS_PER_WEEK; // montag, 5.1.1970 auf 0-wert setzen
  int day = tmp->tm_wday;
  int hour = tmp->tm_hour;
  int minute = tmp->tm_min;
  Serial.print("wday:");
  Serial.println(day);
  Serial.print("week:");
  Serial.println(weekNumber);
  Serial.print("hour:");
  Serial.println(hour);
  Serial.print("minute:");
  Serial.println(minute);

  int muellAlarm = checkMuellabfuhr(now);
  Serial.print("check:");
  Serial.println(muellAlarm);
  if (muellAlarm > 0 and not buttonPressed) {
    if (muellAlarm == MUELL_GRUEN) muellFarbe = GREEN_LED;
    else if (muellAlarm == MUELL_SCHWARZ) muellFarbe = BLUE_LED;
    else if (muellAlarm == MUELL_BRAUN) muellFarbe = RED_LED;
  }

  /* wenn die müllabfuhr vorbei ist und bevor die nächste kommt, buttonState zurücksetzen */
  if (muellAlarm < 0) buttonPressed = false;

  if (muellAlarm < 0 or buttonPressed) {
    if (muellAlarm < 0) Serial.println("müllabfuhr steht nicht an->sleep");
    if (buttonPressed)  Serial.println("blinken wurde schon bestätigt->sleep");
    sleep();
  }

  /*
    int offset_check=0;
    int offset_data=4;
    uint32_t data;
    uint32_t check;
    /* test if rtcUserMemory have already been written or if we have to initialize it *
    /* for this test, use a check value of 0x44444444                                 *
    ESP.rtcUserMemoryRead(offset_check, &check, sizeof(check));
    if (check==0x44444444 and hour>5) {    //alread initialized
    ESP.rtcUserMemoryRead(offset_data, &data, sizeof(data));
    if (data==1) buttonPressed=true;
    else buttonPressed=false;
    } else {  // we have to initialize the rtcMemory if come here for the first time or if it is between 0:00 and 4:59
    check=0x44444444;
    ESP.rtcUserMemoryWrite(offset_check, &check, sizeof(check));
    data=0;
    ESP.rtcUserMemoryWrite(offset_data, &data, sizeof(data));
    buttonPressed=false;
    }
  */


  //analogWrite(GREEN_LED,0);
  //analogWrite(BLUE_LED,0);
  //analogWrite(RED_LED,0);
  //digitalWrite(CONTROL_LED, HIGH);

  //int lightValue = analogRead(fotoZellePin); // 1024->sehr hell, 166->Zimmerhelligkeit
  //int buttonValue = digitalRead(BUTTON);
  //if (hour<5) buttonPressed=false;      // stop-button wieder zurücksetzen

  /*
    if (day==2 and hour>5 and not buttonPressed)  {       //tuesday
    pinMode(BLUE_LED, OUTPUT);   /* Setzt den PIN für die blaue LED als Ausgabe *
    pinMode(RED_LED, OUTPUT);    /* Setzt den PIN für die rote  LED als Ausgabe *
    pinMode(GREEN_LED, OUTPUT);  /* Setzt den PIN für die grüne LED als Ausgabe *
    if (weekNumber % 2 == 1) muellFarbe = GREEN_LED;
    else                     muellFarbe = BLUE_LED;
    } else if (TEST and minute>5 and minute<30 and not buttonPressed) {
    pinMode(BLUE_LED, OUTPUT);   /* Setzt den PIN für die blaue LED als Ausgabe *
    pinMode(RED_LED, OUTPUT);    /* Setzt den PIN für die rote  LED als Ausgabe *
    pinMode(GREEN_LED, OUTPUT);  /* Setzt den PIN für die grüne LED als Ausgabe *
    if (day % 2 == 1) muellFarbe = GREEN_LED;
    else              muellFarbe = BLUE_LED;
    } else {
    sleep();
    }
  */
  pinMode(BLUE_LED, OUTPUT);   /* Setzt den PIN für die blaue LED als Ausgabe */
  pinMode(RED_LED, OUTPUT);    /* Setzt den PIN für die rote  LED als Ausgabe */
  pinMode(GREEN_LED, OUTPUT);  /* Setzt den PIN für die grüne LED als Ausgabe */

  Serial.println("setup done");
}

void loop() {
  /* prüfe stop-knopf und schreibe zustand in rtcMemory */
  int buttonValue = digitalRead(BUTTON);
  if (buttonValue == LOW) {
    //saveButtonState(true);
    buttonPressed = true;
    Serial.println("button pressed->sleep");
    sleep();
  }

  /* prüfe ob müllabfuhr schon vorbei ist -> sleep */
  if (checkMuellabfuhr(time(nullptr)) < 0) sleep();

  digitalWrite(muellFarbe,HIGH);
  delay(100);
  digitalWrite(muellFarbe,LOW);
  delay(800);
}
