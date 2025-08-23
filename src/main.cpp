// Créé le 07/02/25

//------------- Library and settings --------------

//#include <ArduinoOTA.h>
#include <WiFi.h>
//#include <Wire.h>
#include <OneWire.h>
#include "ThingSpeak.h"
#include <DallasTemperature.h>
#include <TimeLib.h>
#include "TimeLord.h"
#include <SolarPosition.h>
#include <NTPClient.h>
#include <Tides.h>

unsigned long myChannelNumber = 134567;
const char* myWriteAPIKey = "OCOFBXF7R5DMCY6P";

constexpr int daysToCalculate = 4;
WiFiClient client;
TideStack tideStack(daysToCalculate);
TideInfo tideInfo;


//--- SETTINGS ------------------------------------------------
//const char* ssid     = "SFR_1198";    // Your ssid
const char* ssid = "SFR_5360";
const char* password = "0123456789";  // Your Password
WiFiServer server(8002);              // Default Virtuino Server port

//-------------VirtuinoCM  Library and settings --------------

#include "VirtuinoCM.h"
VirtuinoCM virtuino;
#define V_memory_count 250  // the size of V memory. You can change it to a number <=255)
float V[V_memory_count];    // This array is synchronized with Virtuino V memory. You can change the type to int, long etc.
String Text_1 = "";         // This text variable is synchronized with the Virtuino pin Text_1
String Text_2 = "";
String Text_3 = "";
String Text_4 = "";
String Text_5 = "";
String Text_6 = "";
String Text_7 = "";
String Text_8 = "";
String Text_9 = "";
String Text_10 = "";
String Text_11 = "";
String Text_12 = "";
String Text_13 = "";
String Text_14 = "";
String Text_15 = "";
String Text_16 = "";
String Text_17 = "";
String Text_18 = "";
String Text_19 = "";
String Text_20 = "";
String Text_21 = "";
String Text_22 = "";

#define ONE_WIRE_BUS 4

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 86400000);  // UTC time with 24h refresh
OneWire oneWire(ONE_WIRE_BUS);                              // R à ajouter
DallasTemperature sensors(&oneWire);
DeviceAddress sensor_in = { 0x28, 0xFF, 0xD7, 0x1B, 0x84, 0x16, 0x04, 0x30 };   //sonde intérieure
DeviceAddress sensor_out = { 0x28, 0xFF, 0x63, 0x44, 0x81, 0x16, 0x04, 0x17 };  //sonde extérieure
TimeLord myLord;


//int decal_TU = 1;                // Heure hiver
int decal_TU = 2;  // Heure été
int timeZone = decal_TU;
int LONGITUDE = -3.16908;
int LATITUDE = 47.32275;

const int numReadings = 5;
int indice = 0;

float outTemp_reading[numReadings];
float total_outTemp_reading;
float average_outTemp = 0;

float inTemp_reading[numReadings];
float total_inTemp_reading;
float average_inTemp = 0;

String SunRise_time, SunSet_time, SunNoon_time, Noon, h, m;

const char* joursSemaine[] = { "Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi" };
const char* joursSuivants[3];

SolarPosition Bangor(LATITUDE, LONGITUDE);  // Bangor
int elevation;
int azimut;

unsigned long currentMillis;       // chrono
unsigned long previousMillis = 0;  // last updated
const long interval = 10000;       // interval between update (10s)

int h_TU, SunSet_h, SunSet_m, SunSet_mn, azimut_Set, SunRise_h, SunRise_m, SunRise_mn, SunNoon_h, SunNoon_mn, elev_Noon, azimut_Rise, PresentTime_mn;

bool PassageMaJ = false;

unsigned long currentEpoch;
unsigned long lastMillis;
int lastDay;
bool useNtpTime = true;  // Set this to true to use NTP time, false to use the manually set time

//============================================================== virtuinoRun
void virtuinoRun() {
  WiFiClient client = server.available();
  if (client) {
    virtuino.readBuffer = "";  // clear Virtuino input buffer. The inputBuffer stores the incoming characters
    if (client.connected()) {
      while (client.available() > 0) {
        char c = client.read();    // read the incoming data
        virtuino.readBuffer += c;  // add the incoming character to Virtuino input buffer
      }
      String* response = virtuino.getResponse();  // get the text that has to be sent to Virtuino as reply. The library will check the inptuBuffer and it will create the response text
      if (response->length() > 0) {
        client.print(*response);
      }
      client.stop();
    }
  }
}


//============================================================== vDelay
void vDelay(int delayInMillis) {
  long t = millis() + delayInMillis;
  while (millis() < t) virtuinoRun();
}



void onReceived(char variableType, uint8_t variableIndex, String valueAsText) {
  if (variableType == 'V') {
    float value = valueAsText.toFloat();                           // convert the value to float. The valueAsText have to be numerical
    if (variableIndex < V_memory_count) V[variableIndex] = value;  // copy the received value to arduino V memory array
  } else if (variableType == 'T') {
    if (variableIndex == 1) Text_1 = valueAsText;  // Store the text to the text variable Text_1
    else if (variableIndex == 2) Text_2 = valueAsText;
    else if (variableIndex == 3) Text_3 = valueAsText;
    else if (variableIndex == 4) Text_4 = valueAsText;
    else if (variableIndex == 5) Text_5 = valueAsText;
    else if (variableIndex == 6) Text_6 = valueAsText;
    else if (variableIndex == 7) Text_7 = valueAsText;
    else if (variableIndex == 8) Text_8 = valueAsText;
    else if (variableIndex == 9) Text_9 = valueAsText;
    else if (variableIndex == 10) Text_10 = valueAsText;
    else if (variableIndex == 11) Text_11 = valueAsText;
    else if (variableIndex == 12) Text_12 = valueAsText;
    else if (variableIndex == 13) Text_13 = valueAsText;
    else if (variableIndex == 14) Text_14 = valueAsText;
    else if (variableIndex == 15) Text_15 = valueAsText;
    else if (variableIndex == 16) Text_16 = valueAsText;
    else if (variableIndex == 17) Text_17 = valueAsText;
    else if (variableIndex == 18) Text_18 = valueAsText;
    else if (variableIndex == 19) Text_19 = valueAsText;
    else if (variableIndex == 20) Text_20 = valueAsText;
    else if (variableIndex == 21) Text_21 = valueAsText;
    else if (variableIndex == 22) Text_22 = valueAsText;
  }
}


String onRequested(char variableType, uint8_t variableIndex) {
  if (variableType == 'V') {
    if (variableIndex < V_memory_count) return String(V[variableIndex]);  // return the value of the arduino V memory array
  } else if (variableType == 'T') {
    if (variableIndex == 1) return Text_1;
    else if (variableIndex == 2) return Text_2;
    else if (variableIndex == 3) return Text_3;
    else if (variableIndex == 4) return Text_4;
    else if (variableIndex == 5) return Text_5;
    else if (variableIndex == 6) return Text_6;
    else if (variableIndex == 7) return Text_7;
    else if (variableIndex == 8) return Text_8;
    else if (variableIndex == 9) return Text_9;
    else if (variableIndex == 10) return Text_10;
    else if (variableIndex == 11) return Text_11;
    else if (variableIndex == 12) return Text_12;
    else if (variableIndex == 13) return Text_13;
    else if (variableIndex == 14) return Text_14;
    else if (variableIndex == 15) return Text_15;
    else if (variableIndex == 16) return Text_16;
    else if (variableIndex == 17) return Text_17;
    else if (variableIndex == 18) return Text_18;
    else if (variableIndex == 19) return Text_19;
    else if (variableIndex == 20) return Text_20;
    else if (variableIndex == 21) return Text_21;
    else if (variableIndex == 22) return Text_22;
  }

  return "";
}


//************** Calcul heure lever - heure coucher soleil ***************************

void Lever_Coucher() {
  byte Today[] = { 0, 0, 12, day(), month(), year() };

  myLord.SunRise(Today);
  h = Today[tl_hour];
  m = Today[tl_minute];
  if (m.toInt() < 10) {
    SunRise_time = "Lever du soleil : " + h + "h0" + m;
  } else {
    SunRise_time = "Lever du soleil : " + h + "h" + m;
  }
  Text_1 = SunRise_time;
  SunRise_h = h.toInt();
  SunRise_m = m.toInt();
  SunRise_mn = h.toInt() * 60 + m.toInt();

  myLord.SunSet(Today);
  h = Today[tl_hour];
  m = Today[tl_minute];
  if (m.toInt() < 10) {
    SunSet_time = "Coucher du soleil : " + h + "h0" + m;
  } else {
    SunSet_time = "Coucher du soleil : " + h + "h" + m;
  }
  Text_2 = SunSet_time;
  SunSet_h = h.toInt();
  SunSet_m = m.toInt();
  SunSet_mn = h.toInt() * 60 + m.toInt();

  SunNoon_mn = round((SunRise_mn + SunSet_mn) / 2.);  //Affichage heure midi soleil
  SunNoon_h = SunNoon_mn / 60;
  SunNoon_mn = SunNoon_mn - SunNoon_h * 60;

  //************** Calcul azimut lever  ***********************************

  tmElements_t someTimeLever = { 0, SunRise_m, SunRise_h - decal_TU, 0, day(), month(), CalendarYrToTm(year()) };
  time_t someEpochTimeLever = makeTime(someTimeLever);
  azimut_Rise = round(Bangor.getSolarAzimuth(someEpochTimeLever));
  V[5] = azimut_Rise;

  //************** Calcul azimut coucher  ***********************************

  tmElements_t someTimeCoucher = { 0, SunSet_m, SunSet_h - decal_TU, 0, day(), month(), CalendarYrToTm(year()) };
  time_t someEpochTimeCoucher = makeTime(someTimeCoucher);

  azimut_Set = round(Bangor.getSolarAzimuth(someEpochTimeCoucher));
  V[6] = azimut_Set;
}

//************** Calcul azimut - élévation midi  ***********************************

void Elevation_Noon() {
  if (SunNoon_mn < 10) {
    SunNoon_time = String(SunNoon_h) + "h0" + String(SunNoon_mn) + " / ";
  } else {
    SunNoon_time = String(SunNoon_h) + "h" + String(SunNoon_mn) + " / ";
  }
  tmElements_t someTime = { 0, SunNoon_mn, SunNoon_h - decal_TU, 0, day(), month(), CalendarYrToTm(year()) };
  time_t someEpochTime = makeTime(someTime);

  elev_Noon = round(Bangor.getSolarElevation(someEpochTime));
  Noon = SunNoon_time + elev_Noon;
  Text_3 = Noon;
}

//************** 3 jours suivants  ***********************************

void J_Semaine() {
  int today = weekday();  // Stocke le jour actuel (1 = Dimanche, ..., 7 = Samedi)

  for (int i = 0; i < 3; i++) {
    int nextDay = today + (i + 1);                 // Ajoute 1, 2, 3 jours
    if (nextDay > 7) nextDay -= 7;                 // Corrige dépassement de la semaine
    joursSuivants[i] = joursSemaine[nextDay - 1];  // Stocke dans le tableau
  }
  Text_4 = joursSuivants[0];
  Text_5 = joursSuivants[1];
  Text_6 = joursSuivants[2];
}

//************** Fonction de conversion avec décalage horaire ***********************************

String convertDecimalTimeToHM_WithOffset(float decimalTime, int offsetHours) {
  const float epsilon = 0.0001;
  if (fabs(decimalTime) < epsilon) {
    return " ";
  }
  decimalTime += offsetHours;

  // Gérer le passage de minuit
  if (decimalTime >= 24.0) {
    decimalTime -= 24.0;
  } else if (decimalTime < 0.0) {
    decimalTime += 24.0;
  }

  int hours = (int)decimalTime;
  int minutes = (int)((decimalTime - hours) * 60);

  String result = "";
  if (hours < 10) result += "0";
  result += String(hours);
  result += "h";
  if (minutes < 10) result += "0";
  result += String(minutes);

  return result;
}

//**************** Marées avec correction horaire ***************************************

void Marees() {
  for (int i = 0; i <= tideStack.getTop(); i++) {
    const TideInfo& tideInfo = tideStack.peek(i);
    Serial.println(joursSemaine[weekday(tideInfo.epoch) - 1]);

    for (int j = 0; j < tideInfo.numEvents; ++j) {
      const TideEvent& event = tideInfo.events[j];
      Serial.print(event.isPeak ? "Marée Haute: " : "Marée Basse: ");
      Serial.print(convertDecimalTimeToHM_WithOffset(event.time, 0));  // CHANGÉ : 0 au lieu de decal_TU
      Serial.print(" (");
      Serial.print(event.amplitude);
      Serial.print("m");
      Serial.println(")");
    }
    Serial.print("Coefficient matin: ");
    Serial.println(tideInfo.morningCoefficient);
    Serial.print("Coefficient aprem: ");
    Serial.println(tideInfo.afternoonCoefficient);
    Serial.println("---");
  }

  //**************** Jour J - TOUS CHANGÉS ***********************************************
  V[7] = tideStack.peek(0).events[0].amplitude;
  V[107] = tideStack.peek(0).morningCoefficient;
  Text_7 = convertDecimalTimeToHM_WithOffset(tideStack.peek(0).getEventTime(0), 0);  // CHANGÉ

  V[8] = tideStack.peek(0).events[1].amplitude;
  V[108] = 0;
  Text_8 = convertDecimalTimeToHM_WithOffset(tideStack.peek(0).getEventTime(1), 0);  // CHANGÉ

  V[9] = tideStack.peek(0).events[2].amplitude;
  V[109] = tideStack.peek(0).afternoonCoefficient;
  Text_9 = convertDecimalTimeToHM_WithOffset(tideStack.peek(0).getEventTime(2), 0);  // CHANGÉ

  V[10] = tideStack.peek(0).events[3].amplitude;
  V[110] = 0;
  Text_10 = convertDecimalTimeToHM_WithOffset(tideStack.peek(0).getEventTime(3), 0);

  //**************** Jour J+1 - TOUS CHANGÉS ***********************************************
  V[11] = tideStack.peek(1).events[0].amplitude;
  V[111] = tideStack.peek(1).morningCoefficient;
  Text_11 = convertDecimalTimeToHM_WithOffset(tideStack.peek(1).getEventTime(0), 0);  // CHANGÉ

  V[12] = tideStack.peek(1).events[1].amplitude;
  V[112] = 0;
  Text_12 = convertDecimalTimeToHM_WithOffset(tideStack.peek(1).getEventTime(1), 0);  // CHANGÉ

  V[13] = tideStack.peek(1).events[2].amplitude;
  V[113] = tideStack.peek(1).afternoonCoefficient;
  Text_13 = convertDecimalTimeToHM_WithOffset(tideStack.peek(1).getEventTime(2), 0);  // CHANGÉ

  V[14] = tideStack.peek(1).events[3].amplitude;
  V[114] = 0;
  Text_14 = convertDecimalTimeToHM_WithOffset(tideStack.peek(1).getEventTime(3), 0);  // CHANGÉ

  //**************** Jour J+2 - TOUS CHANGÉS ***********************************************
  V[15] = tideStack.peek(2).events[0].amplitude;
  V[115] = tideStack.peek(2).morningCoefficient;
  Text_15 = convertDecimalTimeToHM_WithOffset(tideStack.peek(2).getEventTime(0), 0);  // CHANGÉ

  V[16] = tideStack.peek(2).events[1].amplitude;
  V[116] = 0;
  Text_16 = convertDecimalTimeToHM_WithOffset(tideStack.peek(2).getEventTime(1), 0);  // CHANGÉ

  V[17] = tideStack.peek(2).events[2].amplitude;
  V[117] = tideStack.peek(2).afternoonCoefficient;
  Text_17 = convertDecimalTimeToHM_WithOffset(tideStack.peek(2).getEventTime(2), 0);  // CHANGÉ

  V[18] = tideStack.peek(2).events[3].amplitude;
  V[118] = 0;
  Text_18 = convertDecimalTimeToHM_WithOffset(tideStack.peek(2).getEventTime(3), 0);  // CHANGÉ

  //**************** Jour J+3 - TOUS CHANGÉS ***********************************************
  V[19] = tideStack.peek(3).events[0].amplitude;
  V[119] = tideStack.peek(3).morningCoefficient;
  Text_19 = convertDecimalTimeToHM_WithOffset(tideStack.peek(3).getEventTime(0), 0);  // CHANGÉ

  V[20] = tideStack.peek(3).events[1].amplitude;
  V[120] = 0;
  Text_20 = convertDecimalTimeToHM_WithOffset(tideStack.peek(3).getEventTime(1), 0);  // CHANGÉ

  V[21] = tideStack.peek(3).events[2].amplitude;
  V[121] = tideStack.peek(3).afternoonCoefficient;
  Text_21 = convertDecimalTimeToHM_WithOffset(tideStack.peek(3).getEventTime(2), 0);  // CHANGÉ

  V[22] = tideStack.peek(3).events[3].amplitude;
  V[122] = 0;
  Text_22 = convertDecimalTimeToHM_WithOffset(tideStack.peek(3).getEventTime(3), 0);  // CHANGÉ
  // Print all Text_7 to Text_22 values (which are set using convertDecimalTimeToHM_WithOffset)

}
//************** M à J données journalières ********************************************

void MaJ() {
  Lever_Coucher();
  Elevation_Noon();
  J_Semaine();
  Marees();
}

//**************************************************************************************

void setup() {
  Serial.begin(9600);
  while (!Serial)
    ;
  if (strlen(password) > 0) {
    WiFi.begin(ssid, password);
  } else {
    WiFi.begin(ssid);
  }
  //WiFi.setMinSecurity(WIFI_AUTH_WPA_PSK);  // Set minimum security to WPA2
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected");
  timeClient.begin();
  if (useNtpTime) {
    timeClient.update();
    currentEpoch = timeClient.getEpochTime();
  } else {
    currentEpoch = 1739577500;  // Manually set epoch time for my tests (to test midnight transition)
  }
  setTime(currentEpoch);
  lastMillis = millis();
  lastDay = day();

  for (int i = 0; i < daysToCalculate; i++) {
    time_t futureEpoch = currentEpoch + (i * SECS_PER_DAY);  // Add i days in seconds
    tideInfo = run_calculations(futureEpoch);
    tideStack.push(tideInfo);
  }

  server.begin();

  // Initialisation Virtuino
  virtuino.begin(onReceived, onRequested, 256);

  // Initialisation capteurs OneWire
  sensors.begin();
  vDelay(500);
  sensors.setResolution(sensor_in, 10);
  sensors.setResolution(sensor_out, 10);

  // Initialisation Thingspeak
  ThingSpeak.begin(client);

  // Configurations supplémentaires
  myLord.TimeZone(decal_TU * 60);
  myLord.Position(LATITUDE, LONGITUDE);

  // Initialisation des tableaux
  for (int i = 0; i < numReadings; i++) {
    outTemp_reading[i] = 0;
    inTemp_reading[i] = 0;
  }
  MaJ();
}

void loop() {

  unsigned long currentMillis = millis();

  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval)) {
    Serial.print(millis());
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
  }

  virtuinoRun();

  //**************************** Lecture paramètres sondes toutes les xx secondes ***************************

  PresentTime_mn = hour() * 60 + minute();

  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    int currentDay = day();
    if (!useNtpTime) {
      // Update the currentEpoch based on the elapsed time
      currentEpoch += (currentMillis - lastMillis) / 1000;
      lastMillis = currentMillis;
    }
    if (currentDay != lastDay) {
      Serial.println("Midnight transition detected!");
      lastDay = currentDay;
      time_t newEpoch = timeClient.getEpochTime() + (daysToCalculate - 1) * SECS_PER_DAY;
      tideInfo = run_calculations(newEpoch);
      tideStack.push(tideInfo);
      MaJ();
    }

    total_outTemp_reading = total_outTemp_reading - outTemp_reading[indice];
    sensors.requestTemperaturesByAddress(sensor_out);
    outTemp_reading[indice] = sensors.getTempC(sensor_out);
    total_outTemp_reading = total_outTemp_reading + outTemp_reading[indice];

    total_inTemp_reading = total_inTemp_reading - inTemp_reading[indice];
    sensors.requestTemperaturesByAddress(sensor_in);
    inTemp_reading[indice] = sensors.getTempC(sensor_in);
    total_inTemp_reading = total_inTemp_reading + inTemp_reading[indice];

    vDelay(500);

    indice = indice + 1;

    //************** Ecriture moyenne paramètres toutes les n lectures ***************************

    if (indice >= numReadings) {
      indice = 0;

      average_outTemp = (total_outTemp_reading / numReadings) + 0;  //correction de 0.0°
      average_inTemp = (total_inTemp_reading / numReadings) + 0;    //correction de 0.0°

      h_TU = hour() - decal_TU;  // pour l'été
      if (h_TU == -1) {
        h_TU = 23;
      }

      //tmElements_t someTime = {second(), minute(), h_TU, 0, day(), month(), CalendarYrToTm(year()) };
      tmElements_t someTime = { second(), minute(), hour(), 0, day(), month(), CalendarYrToTm(year()) };
      time_t someEpochTime = makeTime(someTime);

      elevation = round(Bangor.getSolarElevation(someEpochTime));
      azimut = round(Bangor.getSolarAzimuth(someEpochTime));

      //************** Ecriture Virtuino ***************************

      V[1] = average_outTemp;  // write temp extérieure to virtual pin V1. On Virtuino panel add a value display or an analog instrument to pin V1
      V[2] = average_inTemp;
      V[4] = elevation;
      if (elevation > 0) {
        V[3] = azimut;
      } else {
        V[3] = 360;
      }

    }  //************************fin de la boucle de l'indice ****************************************
  }    //************************fin de la boucle de l'interval **************************************


  //************** Thingspeak ***************************

  ThingSpeak.setField(1, average_outTemp);
  ThingSpeak.setField(2, average_inTemp);
  ThingSpeak.writeFields(myChannelNumber, myWriteAPIKey);
}

