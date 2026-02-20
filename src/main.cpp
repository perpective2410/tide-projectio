// Station météo-marée de Bangor — Créé le 07/02/25, réécrit pour maintenabilité

#include <ArduinoOTA.h>
#include <WiFi.h>
#include <OneWire.h>
#include "ThingSpeak.h"
#include <DallasTemperature.h>
#include <TimeLib.h>
#include "TimeLord.h"
#include <SolarPosition.h>
#include <NTPClient.h>
#include <Tides.h>
#include "VirtuinoCM.h"

// ─── Configuration ────────────────────────────────────────────────────────────

const char* WIFI_SSID     = "Wokwi-GUEST";
const char* WIFI_PASSWORD = "";

const float LATITUDE  =  47.32275f;   // Bangor, Belle-Île-en-Mer
const float LONGITUDE =  -3.16908f;

const unsigned long THINGSPEAK_CHANNEL_ID = 1275039;
const char*         THINGSPEAK_WRITE_KEY  = "D9S73UER1TAUP4O";

constexpr int  DAYS_FORECAST        = 4;
constexpr int  VIRTUINO_PORT        = 8002;
constexpr int  ONEWIRE_PIN          = 4;
constexpr long SENSOR_PERIOD_MS     = 10000;   // lecture capteurs toutes les 10s
constexpr long THINGSPEAK_PERIOD_MS = 60000;   // envoi ThingSpeak toutes les 60s
constexpr int  TEMP_BUFFER_SIZE     = 5;       // taille du buffer de moyenne glissante

// Adresses des sondes DS18B20 sur le bus OneWire
DeviceAddress SENSOR_OUT = { 0x28, 0xFF, 0x63, 0x44, 0x81, 0x16, 0x04, 0x17 };
DeviceAddress SENSOR_IN  = { 0x28, 0xFF, 0xD7, 0x1B, 0x84, 0x16, 0x04, 0x30 };

const char* JOURS[] = {
    "Dimanche", "Lundi", "Mardi", "Mercredi", "Jeudi", "Vendredi", "Samedi"
};

// ─── Mémoire Virtuino ─────────────────────────────────────────────────────────
// V[1]   = température extérieure (°C)
// V[2]   = température intérieure (°C)
// V[3]   = azimut solaire courant (360 si sous l'horizon)
// V[4]   = élévation solaire courante (°)
// V[5]   = azimut lever du soleil (°)
// V[6]   = azimut coucher du soleil (°)
// V[7..22]   = amplitudes marées (4 jours × 4 événements)
// V[107..122] = coefficients de marée
// V[200..204] = flèches marée basse
// V[210..214] = flèches marée haute
// Text[1] = lever du soleil     Text[2] = coucher du soleil
// Text[3] = midi solaire / élévation
// Text[4..6]  = noms J+1, J+2, J+3
// Text[7..22] = heures de marée (4 jours × 4 événements)

#define V_COUNT 250
float  V[V_COUNT];
String Text[23];  // index 1..22 ; index 0 inutilisé

// ─── Matériel ─────────────────────────────────────────────────────────────────

WiFiClient        wifiClient;
WiFiServer        server(VIRTUINO_PORT);
WiFiUDP           ntpUDP;
NTPClient         timeClient(ntpUDP, "pool.ntp.org", 0, 86400000);
OneWire           oneWire(ONEWIRE_PIN);
DallasTemperature sensors(&oneWire);
TimeLord          myLord;
SolarPosition     solarBangor(LATITUDE, LONGITUDE);
VirtuinoCM        virtuino;
TideStack         tideStack(DAYS_FORECAST);

// ─── Moyenne glissante températures ──────────────────────────────────────────

float outBuf[TEMP_BUFFER_SIZE] = {};
float inBuf[TEMP_BUFFER_SIZE]  = {};
float outSum = 0, inSum = 0;
float avgOut = 0, avgIn  = 0;
int   tempIdx = 0;

// ─── Chronomètres ─────────────────────────────────────────────────────────────

unsigned long prevSensor     = 0;
unsigned long prevThingSpeak = 0;
int lastDay = -1;

// ─── Fuseau horaire (mis à jour automatiquement, été/hiver) ───────────────────

int utcOffsetMin = 60;  // 60 = heure d'hiver, 120 = heure d'été

// ─── Virtuino : callbacks ─────────────────────────────────────────────────────

void onReceived(char type, uint8_t idx, String val) {
    if      (type == 'V' && idx < V_COUNT)           V[idx]   = val.toFloat();
    else if (type == 'T' && idx >= 1 && idx <= 22)   Text[idx] = val;
}

String onRequested(char type, uint8_t idx) {
    if (type == 'V' && idx < V_COUNT)          return String(V[idx]);
    if (type == 'T' && idx >= 1 && idx <= 22)  return Text[idx];
    return "";
}

void virtuinoRun() {
    WiFiClient cl = server.available();
    if (!cl) return;
    virtuino.readBuffer = "";
    if (cl.connected()) {
        while (cl.available()) virtuino.readBuffer += (char)cl.read();
        String* resp = virtuino.getResponse();
        if (resp->length() > 0) cl.print(*resp);
        cl.stop();
    }
}

// Attente non-bloquante : continue de servir Virtuino pendant la pause.
void vDelay(int ms) {
    long t = millis() + ms;
    while (millis() < t) virtuinoRun();
}

// ─── Utilitaires ──────────────────────────────────────────────────────────────

// Formate un temps décimal (ex : 10.5) en "10h30". Retourne " " pour zéro.
String formatHM(float h) {
    if (fabsf(h) < 0.0001f) return " ";
    while (h >= 24.0f) h -= 24.0f;
    while (h <   0.0f) h += 24.0f;
    int hh = (int)h;
    int mm = (int)((h - hh) * 60.0f + 0.5f);
    if (mm == 60) { hh++; mm = 0; }
    char buf[8];
    snprintf(buf, sizeof(buf), "%02dh%02d", hh, mm);
    return String(buf);
}

// Heure locale courante en heures décimales (hour() est UTC car setTime() = UTC).
float localHourNow() {
    float h = hour() + (utcOffsetMin / 60.0f) + (minute() / 60.0f) + (second() / 3600.0f);
    if (h >= 24.0f) h -= 24.0f;
    return h;
}

// ─── Soleil ───────────────────────────────────────────────────────────────────

void updateSolar() {
    int offsetH = utcOffsetMin / 60;

    // TimeLord travaille en heure locale et retourne l'heure locale.
    byte date[] = { 0, 0, 12, (byte)day(), (byte)month(), (byte)year() };

    myLord.SunRise(date);
    int rH = date[tl_hour], rM = date[tl_minute];
    char buf[10];
    snprintf(buf, sizeof(buf), "%dh%02d", rH, rM);
    Text[1] = "Lever du soleil : " + String(buf);

    myLord.SunSet(date);
    int sH = date[tl_hour], sM = date[tl_minute];
    snprintf(buf, sizeof(buf), "%dh%02d", sH, sM);
    Text[2] = "Coucher du soleil : " + String(buf);

    // Midi solaire = milieu entre lever et coucher
    int riseMin = rH * 60 + rM;
    int setMin  = sH * 60 + sM;
    int noonMin = (riseMin + setMin) / 2;
    int nH = noonMin / 60, nM = noonMin % 60;

    // SolarPosition attend du temps UTC ; on soustrait le décalage horaire.
    auto epochUTC = [&](int lh, int lm) -> time_t {
        tmElements_t t = { 0, (byte)lm, (byte)(lh - offsetH), 0,
                           (byte)day(), (byte)month(), (byte)CalendarYrToTm(year()) };
        return makeTime(t);
    };

    V[5] = roundf(solarBangor.getSolarAzimuth(epochUTC(rH, rM)));   // azimut lever
    V[6] = roundf(solarBangor.getSolarAzimuth(epochUTC(sH, sM)));   // azimut coucher

    int elev = roundf(solarBangor.getSolarElevation(epochUTC(nH, nM)));
    snprintf(buf, sizeof(buf), "%dh%02d", nH, nM);
    Text[3] = String(buf) + " / " + String(elev);
}

// ─── Noms des jours suivants (J+1, J+2, J+3) ─────────────────────────────────

void updateDayNames() {
    int today = weekday();  // 1=Dimanche, 7=Samedi
    for (int i = 0; i < 3; i++) {
        // (today + i) % 7 donne l'indice 0-basé du jour suivant grâce au décalage
        // introduit par l'encodage 1-basé de weekday().
        Text[4 + i] = JOURS[(today + i) % 7];
    }
}

// ─── Marées ───────────────────────────────────────────────────────────────────
// Mapping Virtuino :
//   V[7  + d*4 + e] = amplitude (m)         pour le jour d (0..3), événement e (0..3)
//   Text[7 + d*4 + e] = heure formatée       même indices
//   V[107 + d*4 + 0] = coefficient matin
//   V[107 + d*4 + 2] = coefficient après-midi
//   V[107 + d*4 + 1] = V[107 + d*4 + 3] = 0

void updateTides() {
    for (int d = 0; d < DAYS_FORECAST; d++) {
        const TideInfo& ti = tideStack.peek(d);

        Serial.print(JOURS[weekday(ti.epoch) - 1]);
        Serial.print(" — coef matin: "); Serial.print(ti.morningCoefficient);
        Serial.print("  aprèm: ");       Serial.println(ti.afternoonCoefficient);

        for (int e = 0; e < 4; e++) {
            int base = 7 + d * 4 + e;
            V[base]    = ti.events[e].amplitude;
            Text[base] = formatHM(ti.getEventTime(e));

            int coefBase = 107 + d * 4 + e;
            if      (e == 0) V[coefBase] = ti.morningCoefficient;
            else if (e == 2) V[coefBase] = ti.afternoonCoefficient;
            else             V[coefBase] = 0;

            Serial.print(ti.events[e].isPeak ? "  HM " : "  BM ");
            Serial.print(Text[base]);
            Serial.print("  "); Serial.print(ti.events[e].amplitude, 2);
            Serial.println("m");
        }
    }
}

// ─── Flèches : prochain événement de marée ────────────────────────────────────
// V[200+i] = 1 → flèche marée basse à l'événement i du jour J
// V[210+i] = 1 → flèche marée haute à l'événement i du jour J
// V[204] / V[214] = premier événement du lendemain (débordement de la journée)

void updateArrows() {
    float now = localHourNow();

    for (int i = 0; i <= 4; i++) { V[200 + i] = 0; V[210 + i] = 0; }

    const TideInfo& today    = tideStack.peek(0);
    const TideInfo& tomorrow = tideStack.peek(1);

    for (int i = 0; i < 4; i++) {
        float t = today.getEventTime(i);
        if (t > 0.0f && t > now) {
            (today.events[i].isPeak ? V[210 + i] : V[200 + i]) = 1;
            return;
        }
    }

    // Plus d'événement aujourd'hui → premier événement de demain (slot 4)
    if (tomorrow.events[0].time >= 0) {
        (tomorrow.events[0].isPeak ? V[214] : V[204]) = 1;
    }
}

// ─── Mise à jour quotidienne ──────────────────────────────────────────────────

void dailyUpdate() {
    timeClient.update();
    utcOffsetMin = getFranceTimezoneOffset(timeClient.getEpochTime());
    myLord.TimeZone(utcOffsetMin);
    updateSolar();
    updateDayNames();
    updateTides();
}

// ─── setup ────────────────────────────────────────────────────────────────────

void setup() {
    Serial.begin(9600);
    while (!Serial);

    // Connexion WiFi
    if (strlen(WIFI_PASSWORD) > 0) WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    else                            WiFi.begin(WIFI_SSID);
    while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print('.'); }
    Serial.println("\nWiFi connecté : " + WiFi.localIP().toString());

    ArduinoOTA.begin();

    // Synchronisation NTP (heure UTC)
    timeClient.begin();
    timeClient.update();
    time_t epoch = timeClient.getEpochTime();
    setTime(epoch);
    lastDay = day();

    // Fuseau horaire (gestion automatique été/hiver)
    utcOffsetMin = getFranceTimezoneOffset(epoch);
    myLord.TimeZone(utcOffsetMin);
    myLord.Position(LATITUDE, LONGITUDE);

    // Pré-calcul des marées sur DAYS_FORECAST jours
    for (int i = 0; i < DAYS_FORECAST; i++) {
        TideInfo ti = run_calculations(epoch + (time_t)i * SECS_PER_DAY);
        tideStack.push(ti);
    }

    // Virtuino & serveur TCP
    server.begin();
    virtuino.begin(onReceived, onRequested, 256);

    // Sondes de température
    sensors.begin();
    vDelay(500);
    sensors.setResolution(SENSOR_IN,  10);
    sensors.setResolution(SENSOR_OUT, 10);

    // ThingSpeak
    ThingSpeak.begin(wifiClient);

    dailyUpdate();
}

// ─── loop ─────────────────────────────────────────────────────────────────────

void loop() {
    ArduinoOTA.handle();
    virtuinoRun();

    unsigned long now = millis();

    // Reconnexion WiFi si nécessaire
    if (WiFi.status() != WL_CONNECTED && now - prevSensor >= SENSOR_PERIOD_MS) {
        Serial.println("Reconnexion WiFi...");
        WiFi.disconnect();
        WiFi.reconnect();
    }

    if (now - prevSensor < SENSOR_PERIOD_MS) return;
    prevSensor = now;

    // ── Passage minuit ───────────────────────────────────────────────────────
    if (day() != lastDay) {
        Serial.println("Passage minuit détecté");
        lastDay = day();
        time_t newEpoch = timeClient.getEpochTime() + (time_t)(DAYS_FORECAST - 1) * SECS_PER_DAY;
        TideInfo ti = run_calculations(newEpoch);
        tideStack.push(ti);
        dailyUpdate();
    }

    // ── Lecture températures (moyenne glissante) ──────────────────────────────
    outSum -= outBuf[tempIdx];
    sensors.requestTemperaturesByAddress(SENSOR_OUT);
    outBuf[tempIdx] = sensors.getTempC(SENSOR_OUT);
    outSum += outBuf[tempIdx];

    inSum -= inBuf[tempIdx];
    sensors.requestTemperaturesByAddress(SENSOR_IN);
    inBuf[tempIdx] = sensors.getTempC(SENSOR_IN);
    inSum += inBuf[tempIdx];

    vDelay(500);
    tempIdx = (tempIdx + 1) % TEMP_BUFFER_SIZE;

    avgOut = outSum / TEMP_BUFFER_SIZE;
    avgIn  = inSum  / TEMP_BUFFER_SIZE;

    // ── Position solaire courante (hour() = UTC car setTime() avec epoch UTC) ─
    {
        tmElements_t t = { (byte)second(), (byte)minute(), (byte)hour(), 0,
                           (byte)day(), (byte)month(), (byte)CalendarYrToTm(year()) };
        time_t et = makeTime(t);
        int elev = roundf(solarBangor.getSolarElevation(et));
        int azim = roundf(solarBangor.getSolarAzimuth(et));

        V[1] = avgOut;
        V[2] = avgIn;
        V[4] = elev;
        V[3] = (elev > 0) ? azim : 360;
    }

    // ── Flèches marée ────────────────────────────────────────────────────────
    updateArrows();

    // ── ThingSpeak (limité à 1 envoi par minute) ─────────────────────────────
    if (now - prevThingSpeak >= THINGSPEAK_PERIOD_MS) {
        prevThingSpeak = now;
        ThingSpeak.setField(4, avgOut);
        ThingSpeak.setField(5, avgIn);
        ThingSpeak.writeFields(THINGSPEAK_CHANNEL_ID, THINGSPEAK_WRITE_KEY);
    }
}
