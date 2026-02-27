#include <Arduino.h>
#include <TimeLib.h>
#include <math.h>
#include "Tides.h"
#include "stations/StationDef.h"

// ── Astronomical variables (used by TideHarmonics.cpp via extern) ─────────
double T, s, ls, p, M, p1, N;

// ── Internal data structures ──────────────────────────────────────────────

struct Harmonic {
    const char* name;
    double amplitude;
    double phase;
};

// ── Table2NC lookup (linear search over the compile-time array) ───────────

class Table2NCDef {
private:
    Table2NC* array;
    int       count;
public:
    Table2NCDef(Table2NC* arr, int cnt) : array(arr), count(cnt) {}

    Table2NC get_constituent(const char* name) const {
        for (int i = 0; i < count; i++)
            if (strcmp(array[i].name, name) == 0) return array[i];
        return Table2NC{nullptr, 0, 0, 0, 0, 0, 0, 0, nullptr, nullptr};
    }
};

// ── HarmonicModel ─────────────────────────────────────────────────────────

class HarmonicModel {
private:
    Harmonic* harmonics;
    Harmonic  z0Harmonic;
    int       harmonicCount;
public:
    // Copies station data from compile-time StationDef into heap-allocated Harmonics.
    HarmonicModel(const StationDef* def) : harmonicCount(def->harmonicCount) {
        harmonics = new Harmonic[harmonicCount];
        for (int i = 0; i < harmonicCount; i++) {
            harmonics[i] = {
                def->harmonics[i].name,
                def->harmonics[i].amplitude,
                def->harmonics[i].phase
            };
        }
        z0Harmonic = {"Z0", def->z0, 0.0};
    }
    ~HarmonicModel() { delete[] harmonics; }

    Harmonic* get_harmonics()            { return harmonics; }
    Harmonic  get_z0_harmonic()          { return z0Harmonic; }
    int       get_harmonic_count() const { return harmonicCount; }
};

// ── HarmonicCalculator ────────────────────────────────────────────────────

class HarmonicCalculator {
private:
    HarmonicModel& model;
    Table2NCDef&   tableDef;
    double*        equilbrm;      // equilibrium argument in degrees (kept for equi_tide())
    double*        nodefctr;      // nodal amplitude correction factor
    Table2NC*      constituents;  // pre-resolved once at construction
    // ── Per-harmonic cached constants (updated by equi_tide()) ────────────
    double*        speed_rad;     // c.speed in radians/hour — set at construction
    double*        base_phase_rad;// (equilbrm[i] - h.phase) in radians — updated in equi_tide()
    double*        amp_f;         // nodefctr[i] * h.amplitude        — for amplitude()
    double*        amp_d;         // amp_f[i] * speed_rad[i]          — for derivative()
    double         minAmplitude;  // harmonics below this threshold are skipped

public:
    HarmonicCalculator(HarmonicModel& m, Table2NCDef& t, double minAmp = 0.0)
        : model(m), tableDef(t), minAmplitude(minAmp) {
        int n          = model.get_harmonic_count();
        equilbrm       = new double[n]();
        nodefctr       = new double[n]();
        constituents   = new Table2NC[n];
        speed_rad      = new double[n]();
        base_phase_rad = new double[n]();
        amp_f          = new double[n]();
        amp_d          = new double[n]();
        for (int i = 0; i < n; i++) {
            constituents[i] = t.get_constituent(m.get_harmonics()[i].name);
            speed_rad[i]    = radians(constituents[i].speed);
        }
    }
    ~HarmonicCalculator() {
        delete[] equilbrm;
        delete[] nodefctr;
        delete[] constituents;
        delete[] speed_rad;
        delete[] base_phase_rad;
        delete[] amp_f;
        delete[] amp_d;
    }

    // Called once per day — computes equilibrium arguments, node factors,
    // and derived per-harmonic constants used by amplitude() / derivative().
    void equi_tide() {
        static const double Thalf = 180.0;
        for (int i = 0; i < model.get_harmonic_count(); i++) {
            const Table2NC& c = constituents[i];
            if (c.u_func == nullptr) continue;
            Harmonic& h = model.get_harmonics()[i];
            equilbrm[i]       = reduc360(
                c.T * Thalf + c.s * s + c.ls * ls +
                c.p * p + c.p1 * p1 + c.deg + c.u_func());
            nodefctr[i]       = c.f_func();
            base_phase_rad[i] = radians(equilbrm[i] - h.phase);
            if (h.amplitude < minAmplitude) {
                amp_f[i] = amp_d[i] = 0.0;
            } else {
                amp_f[i] = nodefctr[i] * h.amplitude;
                amp_d[i] = amp_f[i] * speed_rad[i];
            }
        }
    }

    // Hot path — sin/cos called once per harmonic; all other factors pre-cached.
    double derivative(double t) {
        double total = 0.0;
        for (int i = 0; i < model.get_harmonic_count(); i++) {
            if (amp_d[i] == 0.0) continue;
            total -= amp_d[i] * sin(speed_rad[i] * t + base_phase_rad[i]);
        }
        return total;
    }

    double amplitude(double t) {
        double total = 0.0;
        Harmonic z0 = model.get_z0_harmonic();
        for (int i = 0; i < model.get_harmonic_count(); i++) {
            if (amp_f[i] == 0.0) continue;
            total += amp_f[i] * cos(speed_rad[i] * t + base_phase_rad[i]);
        }
        return z0.amplitude + total;
    }

    TideInfo findTidesAndTimes(int utcOffsetMinutes, double coeffDivisor = 6.1) {
        TideInfo tideInfo;
        TideEvent tideEvents[4];
        double extremumUTCTimes[4];  // parallel array: UTC time of each extremum
        int eventCount = 0;

        const double COARSE_STEP  = 20.0 / 60.0;
        const int    BISECT_STEPS = 14;

        double utcStart = -(utcOffsetMinutes / 60.0);
        double utcEnd   = utcStart + 24.0;
        double prevT    = utcStart;
        double prevD    = derivative(prevT);

        for (double t = utcStart + COARSE_STEP;
             t <= utcEnd + COARSE_STEP * 0.5 && eventCount < 4;
             t += COARSE_STEP) {

            double curD = derivative(t);
            if (prevD * curD < 0.0) {
                double a = prevT, da = prevD;
                double b = t,     db = curD;
                for (int k = 0; k < BISECT_STEPS; k++) {
                    double mid = (a + b) * 0.5;
                    double dm  = derivative(mid);
                    if (da * dm <= 0.0) { b = mid; db = dm; }
                    else                { a = mid; da = dm; }
                }
                double extremumT = (a + b) * 0.5;
                double extremumH = amplitude(extremumT);
                bool   isPeak    = (prevD > 0.0);
                float  localT    = (float)(extremumT + utcOffsetMinutes / 60.0);
                while (localT <  0.0f) localT += 24.0f;
                while (localT >= 24.0f) localT -= 24.0f;
                tideEvents[eventCount] = { extremumH, localT, isPeak };
                extremumUTCTimes[eventCount] = extremumT;  // store UTC for amplitude lookup
                eventCount++;
            }
            prevT = t;
            prevD = curD;
        }

        for (int i = 0; i < eventCount; i++)
            for (int j = i + 1; j < eventCount; j++)
                if (tideEvents[i].time > tideEvents[j].time) {
                    TideEvent tmp = tideEvents[i];
                    tideEvents[i] = tideEvents[j];
                    tideEvents[j] = tmp;
                    double tmpUTC = extremumUTCTimes[i];
                    extremumUTCTimes[i] = extremumUTCTimes[j];
                    extremumUTCTimes[j] = tmpUTC;
                }

        tideInfo.numEvents = eventCount;
        for (int i = 0; i < eventCount; i++) tideInfo.events[i] = tideEvents[i];

        if (tideInfo.numEvents >= 2) {
            if (tideInfo.events[0].isPeak != tideInfo.events[1].isPeak)
                tideInfo.morningCoefficient = abs(round(
                    (tideInfo.events[0].amplitude - tideInfo.events[1].amplitude)
                    / coeffDivisor * 100));
            if (tideInfo.events[tideInfo.numEvents - 2].isPeak !=
                tideInfo.events[tideInfo.numEvents - 1].isPeak)
                tideInfo.afternoonCoefficient = abs(round(
                    (tideInfo.events[tideInfo.numEvents - 2].amplitude -
                     tideInfo.events[tideInfo.numEvents - 1].amplitude)
                    / coeffDivisor * 100));
        }

        // Calculate amplitude points at regular intervals (for sinusoid display)
        // Reuse pre-calculated extremum amplitudes where sample falls within 0.6 min
        for (int i = 0; i < TIDE_AMPLITUDE_SAMPLES; i++) {
            double localHours = i * (24.0 / TIDE_AMPLITUDE_SAMPLES);
            double tUTC = localHours - (utcOffsetMinutes / 60.0);
            bool found = false;
            for (int j = 0; j < eventCount; j++) {
                if (fabs(tUTC - extremumUTCTimes[j]) < 0.01) {
                    tideInfo.amplitudePoints[i] = tideEvents[j].amplitude;
                    found = true;
                    break;
                }
            }
            if (!found) tideInfo.amplitudePoints[i] = amplitude(tUTC);
        }
        tideInfo.amplitudeCalculated = true;

        return tideInfo;
    }
};

// ── TideStack ─────────────────────────────────────────────────────────────

TideStack::TideStack(int daysToCalculate) : stackSize(daysToCalculate), count(0) {
    stack = new TideInfo[stackSize];
}
TideStack::~TideStack() { delete[] stack; }

void TideStack::push(TideInfo& tideInfo) {
    for (int i = 0; i < stackSize - 1; i++) stack[i] = stack[i + 1];
    stack[stackSize - 1] = tideInfo;
    if (count < stackSize) count++;
}
TideInfo* TideStack::getStack()      { return stack; }
int       TideStack::getCount()      { return count; }
int       TideStack::getTop()        { return count - 1; }
TideInfo& TideStack::peek(int index) { return stack[index]; }

// ── Astronomical calculations ─────────────────────────────────────────────

static double dateToJulianDay(int year, int month, int day) {
    if (month <= 2) { year -= 1; month += 12; }
    int A = year / 100;
    int B = 2 - A + A / 4;
    return int(365.25 * (year + 4716)) + int(30.6001 * (month + 1)) + day + B - 1524.5;
}

static double julianDateToCentury(double julianDay) {
    return (julianDay - 2451545.0) / 36525.0;
}

static void astroCalculations(time_t epoch) {
    tmElements_t tm;
    breakTime(epoch, tm);
    double julianDay = dateToJulianDay(tmYearToCalendar(tm.Year), tm.Month, tm.Day);
    T  = julianDateToCentury(julianDay);
    // Apply ΔT = 65 s correction (7.523148148148148e-4 days).
    // All astronomical arguments (s, ls, p, N, p1) use the corrected century Tdt.
    double Tdt = julianDateToCentury(julianDay + 7.523148148148148e-4);
    s  = reduc360(218.3164591 + 481267.88134236 * Tdt - 0.0013268 * Tdt * Tdt
                  + Tdt * Tdt * Tdt / 538841.0 - Tdt * Tdt * Tdt * Tdt / 65194000.0);
    ls = reduc360(280.46645 + 36000.76983 * Tdt + 0.0003032 * Tdt * Tdt);
    p  = reduc360(83.3532430 + 4069.0137111 * Tdt - 0.0103238 * Tdt * Tdt
                  - Tdt * Tdt * Tdt / 80053.0 + Tdt * Tdt * Tdt * Tdt / 18999000.0);
    M  = 357.52910 + 35999.05030 * Tdt - 0.0001559 * Tdt * Tdt - 0.00000048 * Tdt * Tdt * Tdt;
    p1 = reduc360(ls - M);
    N  = reduc360(125.04452 - 1934.136261 * Tdt + 0.0020708 * Tdt * Tdt + Tdt * Tdt * Tdt / 450000.0);
}

int getFranceTimezoneOffset(time_t epoch) {
    int yr = year(epoch);  // calendar year via TimeLib (AVR-compatible)

    // Last Sunday of March at 1:00 UTC
    tmElements_t tm;
    tm.Year   = yr - 1970; tm.Month = 3; tm.Day = 31;
    tm.Hour   = 1; tm.Minute = 0; tm.Second = 0;
    time_t startDSTEpoch = makeTime(tm);
    while (weekday(startDSTEpoch) != 1) startDSTEpoch -= 86400;  // 1 = Sunday

    // Last Sunday of October at 1:00 UTC
    tm.Month = 10; tm.Day = 31;
    time_t endDSTEpoch = makeTime(tm);
    while (weekday(endDSTEpoch) != 1) endDSTEpoch -= 86400;

    return (epoch >= startDSTEpoch && epoch < endDSTEpoch) ? 120 : 60;
}

// ── Global state ──────────────────────────────────────────────────────────

// Lazy singleton — initialised on first call, after all global constructors
// have run, which guarantees table2NcDefArray is fully constructed.
static Table2NCDef& getTableDef() {
    static Table2NCDef instance(table2NcDefArray, TABLE2NC_COUNT);
    return instance;
}

static HarmonicModel*      g_model         = nullptr;
static HarmonicCalculator* g_calculator    = nullptr;
static HarmonicModel*      g_modelRef      = nullptr;
static HarmonicCalculator* g_calculatorRef = nullptr;
static double              g_coeffDivisor  = 6.1;
static double              g_minAmplitude  = 0.005; // skip harmonics below 5 mm

static void clearStation() {
    delete g_calculator;  g_calculator = nullptr;
    delete g_model;       g_model      = nullptr;
}
static void clearRef() {
    delete g_calculatorRef; g_calculatorRef = nullptr;
    delete g_modelRef;      g_modelRef      = nullptr;
}

// Auto-loads Brest as the coefficient reference whenever a station is set.
static bool loadRef() {
    const StationDef* def = findStation("Brest");
    if (!def) {
        Serial.println("[Tides] Brest reference station not found in registry");
        return false;
    }
    clearRef();
    g_coeffDivisor  = def->refHigh;
    g_modelRef      = new HarmonicModel(def);
    g_calculatorRef = new HarmonicCalculator(*g_modelRef, getTableDef());
    Serial.println("[Tides] Loaded Brest reference ("
                   + String(def->harmonicCount) + " constituents)");
    return true;
}


bool setStation(const char* id, double minAmplitude) {
    const StationDef* def = findStation(id);
    if (!def) {
        Serial.println("[Tides] Station not found: " + String(id));
        return false;
    }
    g_minAmplitude = minAmplitude;
    clearStation();
    g_model      = new HarmonicModel(def);
    g_calculator = new HarmonicCalculator(*g_model, getTableDef(), g_minAmplitude);
    Serial.println("[Tides] Loaded " + String(id) +
                   " (" + String(def->harmonicCount) + " constituents)");
    return loadRef();
}

TideInfo tides(time_t epoch) {
    if (!g_calculator || !g_calculatorRef) {
        Serial.println("[Tides] No station loaded — call setStation() first");
        return TideInfo();
    }

    tmElements_t tm;
    breakTime(epoch, tm);
    Serial.println("[Tides] Calculating " +
                   String(tm.Day) + "/" + String(tm.Month) + "/" +
                   String(tmYearToCalendar(tm.Year)));

    astroCalculations(epoch);
    int utcOffset = getFranceTimezoneOffset(epoch);

    g_calculator->equi_tide();
    TideInfo result = g_calculator->findTidesAndTimes(utcOffset);

    g_calculatorRef->equi_tide();
    TideInfo refResult = g_calculatorRef->findTidesAndTimes(utcOffset, g_coeffDivisor);

    result.morningCoefficient   = refResult.morningCoefficient;
    result.afternoonCoefficient = refResult.afternoonCoefficient;
    result.epoch = epoch;
    return result;
}

TideInfo tides(int year, int month, int day) {
    tmElements_t tm = {};
    tm.Year  = CalendarYrToTm(year);
    tm.Month = month;
    tm.Day   = day;
    tm.Hour  = 12;  // noon UTC — safe midpoint
    return tides(makeTime(tm));
}
