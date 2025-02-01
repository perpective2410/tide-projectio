
#include <Arduino.h>
#include <TimeLib.h>
#include <math.h>
#include <unordered_map>
#include <functional> 
#include <map>
#include <string>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <WiFi.h>

const char* ssid = "Wokwi-GUEST";
//const char* password = "Magenta2110";


// Global variables
double T, s, h, p, M, p1, N;

double reduc360(double angle);


class MyDate {
  public:
    int year;
    int month;
    int day;

    // Default constructor
    MyDate() {
      year = 0;
      month = 0;
      day = 0;
    }

    // Constructor
    MyDate(int y, int m, int d) {
      year = y;
      month = m;
      day = d;
    }

    String getDate() const {
        char buffer[11];  // Buffer to hold the formatted date string
        sprintf(buffer, "%02d/%02d/%04d", day, month, year);  // Format as DD/MM/YYYY
        return String(buffer);
    }
};



struct Table2NC {
    String name;
    double T, s, h, p, p1, deg, speed;
    double (*u_func)();
    double (*f_func)();
};


// Function definitions
double zero() {
    return 0.0;
}
double fMm() {
    return 1.0 - (0.1311 * cos(radians(N))) + (0.0538 * cos(radians(2.0 * p))) + (0.0205 * cos(radians(2.0 * p - N)));
}

double uMf() {
    return (-23.7 * sin(radians(N))) + (2.7 * sin(radians(2.0 * N))) - (0.4 * sin(radians(3.0 * N)));
}

double fMf() {
    return 1.084 + (0.415 * cos(radians(N))) + (0.039 * cos(radians(2.0 * N)));
}

double uO1() {
    return (10.80 * sin(radians(N))) - (1.34 * sin(radians(2.0 * N))) + (0.19 * sin(radians(3.0 * N)));
}

double fO1() {
    return 1.0176 + (0.1871 * cos(radians(N))) - (0.0147 * cos(radians(2.0 * N)));
}

double uK1() {
    return (-8.86 * sin(radians(N))) + (0.68 * sin(radians(2.0 * N))) - (0.07 * sin(radians(3.0 * N)));
}

double fK1() {
    return 1.0060 + (0.1150 * cos(radians(N))) - (0.0088 * cos(radians(2.0 * N))) + (0.0006 * cos(radians(3.0 * N)));
}

double one() {
    return 1.0;
}

double uJ1() {
    return (-12.94 * sin(radians(N))) + (1.34 * sin(radians(2.0 * N))) - (0.19 * sin(radians(3.0 * N)));
}

double fJ1() {
    return 1.1029 + (0.1676 * cos(radians(N))) - (0.0170 * cos(radians(2.0 * N))) + (0.0016 * cos(radians(3.0 * N)));
}

double uM2() {
    return -2.14 * sin(radians(N));
}

double fM2() {
    return 1.0007 - (0.0373 * cos(radians(N))) + (0.0002 * cos(radians(2.0 * N)));
}

double uK2() {
    return (-17.74 * sin(radians(N))) + (0.68 * sin(radians(2.0 * N))) - (0.04 * sin(radians(3.0 * N)));
}

double fK2() {
    return 1.0246 + (0.2863 * cos(radians(N))) + (0.0083 * cos(radians(2.0 * N))) - (0.0015 * cos(radians(3.0 * N)));
}

double uM3() {
    return -3.21 * sin(radians(N));
}

double fM3() {
    return pow(sqrt(fM2()), 3);
}

double uminusM2() {
    return -uM2();
}

double u2M2() {
    return 2 * uM2();
}

double u3M2() {
    return 3 * uM2();
}

double u4M2() {
    return 4 * uM2();
}

double u5M2() {
    return 5 * uM2();
}

double u6M2() {
    return 6 * uM2();
}

double f2M2() {
    return pow(fM2(), 2);
}

double f3M2() {
    return pow(fM2(), 3);
}

double f4M2() {
    return pow(fM2(), 4);
}

double f5M2() {
    return pow(fM2(), 5);
}

double f6M2() {
    return pow(fM2(), 6);
}

double uM2mK1() {
    return uM2() - uK1();
}

double uM2pK1() {
    return uM2() + uK1();
}

double fM2K1() {
    return fM2() * fK1();
}

double u2M2pK1() {
    return (2 * uM2()) + uK1();
}

double f2M2K1() {
    return pow(fM2(), 2) * fK1();
}

double uM2mK2() {
    return uM2() - uK2();
}

double uM2pK2() {
    return uM2() + uK2();
}

double uK2mM2() {
    return uK2() - uM2();
}

double fM2K2() {
    return fM2() * fK2();
}

double u2M2mK2() {
    return (2 * uM2()) - uK2();
}

double u2M2pK2() {
    return (2 * uM2()) + uK2();
}

double f2M2K2() {
    return pow(fM2(), 2) * fK2();
}

double u3M2mK2() {
    return (3 * uM2()) - uK2();
}

double u3M2pK2() {
    return (3 * uM2()) + uK2();
}

double f3M2K2() {
    return pow(fM2(), 3) * fK2();
}

double u4M2mK2() {
    return (4 * uM2()) - uK2();
}

double f4M2K2() {
    return pow(fM2(), 4) * fK2();
}

double fsinM1() {
    return sin(radians(p)) + (0.2 * sin(radians(p - N)));
}

double fcosM1() {
    return 2 * (cos(radians(p)) + (0.2 * cos(radians(p - N))));
}

double uM1() {
    return degrees(atan2(fsinM1(), fcosM1()));
}

double fM1() {
    return sqrt(pow(fsinM1(), 2) + pow(fcosM1(), 2));
}

double fsinGamma2() {
    return 0.147 * sin(radians(2 * (N - p)));
}

double fcosGamma2() {
    return 1.0 + (0.147 * cos(radians(2 * (N - p))));
}

double uGamma2() {
    return degrees(atan2(fsinGamma2(), fcosGamma2()));
}

double fGamma2() {
    return sqrt(pow(fsinGamma2(), 2) + pow(fcosGamma2(), 2));
}

double fsinL2() {
    return (-0.2505 * sin(radians(2 * p))) - (0.1102 * sin(radians(2 * p - N))) - (0.0156 * sin(radians(2 * p - 2 * N))) - (0.037 * sin(radians(N)));
}

double fcosL2() {
    return 1.0 - (0.2505 * cos(radians(2 * p))) - (0.1102 * cos(radians(2 * p - N))) - (0.0156 * cos(radians(2 * p - 2 * N))) - (0.037 * cos(radians(N)));
}

double uL2() {
    return degrees(atan2(fsinL2(), fcosL2()));
}

double fL2() {
    return sqrt(pow(fsinL2(), 2) + pow(fcosL2(), 2));
}

double fsinM1B() {
    return (2.783 * sin(radians(2 * p)) + (0.558 * radians((2 * p) - N))) + (0.184 * sin(radians(N)));
}

double fcosM1B() {
    return (1.0 + (2.783 * cos(radians(2 * p)))) + (0.558 * sin(radians((2 * p) - N))) + (0.184 * sin(radians(N)));
}

double uM1B() {
    return degrees(atan2(fsinM1B(), fcosM1B()));
}

double fM1B() {
    return sqrt(pow(fsinM1B(), 2) + pow(fcosM1B(), 2));
}

double fsinM1A() {
    return -(0.3593 * sin(radians(2 * p))) - (0.2 * sin(radians(N))) - (0.066 * sin(radians((2 * p) - N)));
}

double fcosM1A() {
    return 1.0 + (0.3593 * cos(radians(2 * p))) + (0.2 * cos(radians(N))) + (0.066 * cos(radians((2 * p) - N)));
}

double uM1A() {
    return degrees(atan2(fsinM1A(), fcosM1A()));
}

double fM1A() {
    return sqrt(pow(fsinM1A(), 2) + pow(fcosM1A(), 2));
}

double fsinalpha2() {
    return -(0.0446 * sin(radians(p - p1)));
}

double fcosalpha2() {
    return 1.0 - (0.0446 * cos(radians(p - p1)));
}

double ualpha2() {
    return degrees(atan2(fsinalpha2(), fcosalpha2()));
}

double falpha2() {
    return sqrt(pow(fsinalpha2(), 2) + pow(fcosalpha2(), 2));
}

double fsindelta2() {
    return 0.477 * sin(radians(N));
}

double fcosdelta2() {
    return 1.0 - (0.477 * cos(radians(N)));
}

double udelta2() {
    return degrees(atan2(fsindelta2(), fcosdelta2()));
}

double fdelta2() {
    return sqrt(pow(fsindelta2(), 2) + pow(fcosdelta2(), 2));
}

double uE2() {
    return degrees(atan2(-0.439 * sin(radians(N)), 1.0 + (0.439 * cos(radians(N)))));
}

double fE2() {
    return sqrt(pow(-0.439 * sin(radians(N)), 2) + pow(1.0 + (0.439 * cos(radians(N))), 2));
}

double uK1pJ1() {
    return uK1() + uJ1();
}

double fK1J1() {
    return fK1() * fJ1();
}

double uK2mQ1() {
    return uK2() - uO1();
}

double fK2Q1() {
    return fK2() * fO1();
}

double uK1mO1() {
    return uK1() - uO1();
}

double fK1O1() {
    return fK1() * fO1();
}

double uM2mJ1() {
    return uM2() - uJ1();
}

double fM2J1() {
    return fM2() * fJ1();
}

double uminusK1() {
    return -uK1();
}

double uM2mO1() {
    return uM2() - uO1();
}

double fM2O1() {
    return fM2() * fO1();
}

double uminusO1() {
    return -uO1();
}

double u2O1() {
    return 2 * uO1();
}

double f2O1() {
    return pow(fO1(), 2);
}

double u2M2pL2() {
    return (2 * uM2()) + uL2();
}

double f2M2L2() {
    return pow(fM2(), 2) * fL2();
}

double u2M2m2K2() {
    return (2 * uM2()) - 2 * uK2();
}

double f2M22K2() {
    return pow(fM2(), 2) * pow(fK2(), 2);
}

double uM2pL2mK2() {
    return uM2() + uL2() - 2 * uK2();
}

double fM2L2K2() {
    return fM2() * fL2() * fK2();
}

double uK1pO1() {
    return uK1() + uO1();
}

double uM2p2K1() {
    return uM2() + 2 * uK1();
}

double f2K1M2() {
    return pow(fK1(), 2) * fM2();
}

double uM2m2K1() {
    return uM2() - 2 * uK1();
}

double u2K2pM2() {
    return uM2() + 2 * uK2();
}

double fM22K2() {
    return pow(fK2(), 2) * fM2();
}

double u2K2() {
    return 2 * uK2();
}

double uminus2M2() {
    return -2 * uM2();
}

double uK2m2M2() {
    return uK2() - 2 * uM2();
}

double uM2pO1() {
    return uM2() + uO1();
}

double u2M2mK1() {
    return 2 * uM2() - uK1();
}

double f3M2K1() {
    return pow(fM2(), 3) * fK1();
}

double uMS3() {
    return uM3();
}

double fMS3() {
    return fM3();
}

double uMS5() {
    return -5 * (1.07 * sin(radians(N)));
}

double fMS5() {
    return pow(sqrt(fM2()), 5);
}

double uMS7() {
    return -7 * (1.07 * sin(radians(N)));
}

double fMS7() {
    return pow(sqrt(fM2()), 7);
}

double u2M2mO1() {
    return 2 * uM2() - uO1();
}

double f2M2O1() {
    return pow(fM2(), 2) * fO1();
}

double u2M2pL2mK2() {
    return 2 * uM2() + uL2() - uK2();
}

double f2M2L2K2() {
    return pow(fM2(), 2) * fL2() * fK2();
}

double f2K2() {
    return pow(fK2(), 2);
}

double u2M2pO1() {
    return 2 * uM2() + uO1();
}

double u3M2pK1() {
    return 3 * uM2() + uK1();
}

double u3M2mK1() {
    return 3 * uM2() - uK1();
}

double u3M2mO1() {
    return 3 * uM2() - uO1();
}

double f3M2O1() {
    return pow(fM2(), 3) * fO1();
}

double u3K1pM2() {
    return 3 * uK1() + uM2();
}

double f3K1M2() {
    return pow(fK1(), 3) * fM2();
}

double uK1pK2() {
    return uK1() + uK2();
}

double fK1K2() {
    return fK1() * fK2();
}

double u5M2mK2() {
    return 5 * uM2() - uK2();
}

double f5M2K2() {
    return pow(fM2(), 5) * fK2();
}

double u3M2pO1() {
    return 3 * uM2() + uO1();
}

double uM2pL2() {
    return uM2() + uL2();
}

double fM2L2() {
    return fM2() * fL2();
}

double uM2pL2pK2() {
    return uM2() + uL2() + uK2();
}

double u4M2mO1() {
    return 4 * uM2() - uO1();
}

double f4M2O1() {
    return pow(fM2(), 4) * fO1();
}

double u2M2pK2pO1() {
    return 2 * uM2() + uK2() + uO1();
}

double f2M2K2O1() {
    return pow(fM2(), 2) * fK2() * fO1();
}

double uM2pK2pO1() {
    return uM2() + uK2() + uO1();
}

double fM2K2O1() {
    return fM2() * fK2() * fO1();
}

double u4M2pK2() {
    return 4 * uM2() + uK2();
}

double u3M2pL2() {
    return 3 * uM2() + uL2();
}

double f3M2L2() {
    return pow(fM2(), 3) * fL2();
}

double u2M2p2K2() {
    return 2 * uM2() + 2 * uK2();
}

double u4M2pK1() {
    return 4 * uM2() + uK1();
}

double f4M2K1() {
    return pow(fM2(), 4) * fK1();
}

double u5M2pK2() {
    return 5 * uM2() + uK2();
}

double f7M2() {
    return pow(fM2(), 7);
}

double u7M2() {
    return 7 * uM2();
}

double u4M2pL2() {
    return 4 * uM2() + uL2();
}

double f4M2L2() {
    return pow(fM2(), 4) * fL2();
}

double u6M2pK2() {
    return 6 * uM2() + uK2();
}

double f6M2K2() {
    return pow(fM2(), 6) * fK2();
}

double u6M2mK2() {
    return 6 * uM2() - uK2();
}

double u2SM() {
    return -2 * uM2();
}

double f2SM() {
    return pow(fM2(), 2);
}



struct TideInfo {
    std::pair<float, std::string> peaks[2];
    std::pair<float, std::string> troughs[2];
    int numPeaks;
    int numTroughs;
    int lowTideCoefficient;
    int highTideCoefficient;
    MyDate date;
};
class TideStack {
  private:
    static const int MAX_CAPACITY = 4;
    TideInfo stack[MAX_CAPACITY];
    int top;

  public:
    // Constructor to initialize the stack
    TideStack() : top(-1) {}

    // Function to push a TideInfo object onto the stack
    bool push(const TideInfo& tideInfo) {
        if (top < MAX_CAPACITY - 1) {
            stack[++top] = tideInfo;
            return true;
        } else {
          return false;
        }
    }

    // Function to get a TideInfo object from the stack
    bool get(TideInfo& tideInfo) {
        if (top >= 0) {
            tideInfo = stack[top--];
            return true;
        } else {
            Serial.println("Stack is empty. Cannot get more TideInfo objects.");
            return false;
        }
    }

     // Pop method
    TideInfo pop() {
        if (top < 0) {
            Serial.println("Stack Underflow");
            return TideInfo();
        }
        return stack[top--];
    }

    // Function to peek at a specific index in the stack
    TideInfo peek(int index) {
        if (index >= 0 && index <= top) {
            return stack[index];
        } else {
            Serial.println("Invalid index. Returning default TideInfo.");
            return TideInfo();  // Return a default TideInfo object
        }
    }

    // Function to get the top index of the stack
    int getTop() const {
        return top;
    }

    // Function to check if the stack is empty
    bool isEmpty() const {
        return top == -1;
    }

    // Function to check if the stack is full
    bool isFull() const {
        return top == MAX_CAPACITY - 1;
    }
};



// Function to convert date to Julian Day Number
double dateToJulianDay(const MyDate& date) {
    int year = date.year;
    int month = date.month;
    int day = date.day;

    // Adjust month and year for the Julian Day Number calculation
    if (month <= 2) {
        year -= 1;
        month += 12;
    }

    // Calculate the Julian Day Number
    int A = year / 100;
    int B = 2 - A + A / 4;

    double JDN = int(365.25 * (year + 4716)) + int(30.6001 * (month + 1)) + day + B - 1524.5;

    return JDN;
}

// Function to convert a Julian day to Julian century
double julianDateToCentury(double julianDay) {
    return (julianDay - 2451545.0) / 36525.0;
}



// Function to perform astronomical calculations
void astroCalculations(MyDate date) {
    double julianDay = dateToJulianDay(date);
    T = julianDateToCentury(julianDay);
    s = reduc360(218.3164591 + 481267.88134236 * T - 0.0013268 * T * T + T * T * T / 538841.0 - T * T * T * T / 65194000.0);
    h = reduc360(280.46645 + 36000.76983 * T + 0.0003032 * T * T);
    p = reduc360(83.3532430 + 4069.0137111 * T - 0.0103238 * T * T - T * T * T / 80053.0 + T * T * T * T / 18999000.0);
    M = 357.52910 + 35999.05030 * T - 0.0001559 * T * T - 0.00000048 * T * T * T;
    p1 = reduc360(h - M);
    N = reduc360(125.04452 - 1934.136261 * T + 0.0020708 * T * T + T * T * T / 450000.0);
    
    //Serial.println("Astro Values: ");
    //Serial.print("JD: "); Serial.print(julianDay, 10);
    //Serial.print("T: "); Serial.print(T, 10);
    //Serial.print(", s: "); Serial.print(s,10);
    //Serial.print(", h: "); Serial.print(h,10);
    //Serial.print(", p: "); Serial.print(p,10);
    //Serial.print(", M: "); Serial.print(M,10);
    //Serial.print(", p1: "); Serial.print(p1,10);
    //Serial.print(", N: "); Serial.println(N,10);
}




// Custom hash function for String
struct StringHash {
    size_t operator()(const String& s) const {
        size_t hash = 0;
        for (size_t i = 0; i < s.length(); ++i) {
            hash = hash * 31 + s[i];  // A simple hash function
        }
        return hash;
    }
};



class Table2NCDef {
private:
    std::unordered_map<String, Table2NC, StringHash> tableMap;  // Use custom hash function

public:
    // Constructor to initialize the table
    template <size_t N>
    Table2NCDef(Table2NC (&array)[N]) {
        for (int i = 0; i < N; i++) {
            tableMap[array[i].name] = array[i]; // Insert into the hash map
        }
    }
    ~Table2NCDef() {
      tableMap.clear();
    }
    Table2NC get_constituent(const String& name) const {
        auto it = tableMap.find(name);
        if (it != tableMap.end()) {
            return it->second;  // Return the found element
        }
        // Return an empty object if not found
        return Table2NC{"", 0, 0, 0, 0, 0, 0, 0, nullptr, nullptr};
    }
};
 
struct Harmonic {
    String name;
    double amplitude;
    double phase;
};


class HarmonicModel {
private:
    Harmonic* harmonics; // Pointer to dynamically allocated array of harmonics
    Harmonic z0Harmonic;
    int harmonicCount;   // Number of harmonics

public:
    template <size_t N>
    // Constructor: initialize with harmonics array and its size
    HarmonicModel(Harmonic (&harmonicsArray)[N], Harmonic z0) {
        harmonicCount = N;
        harmonics = new Harmonic[harmonicCount];
        z0Harmonic = z0;
        for (int i = 0; i < harmonicCount; ++i) {
            harmonics[i] = harmonicsArray[i]; // Deep copy of harmonics
        }
    }

    // Destructor to free allocated memory
    ~HarmonicModel() {
        delete[] harmonics;
    }

    // Getter for harmonics array
    Harmonic* get_harmonics() {
        return harmonics; // Return pointer to harmonics array
    }

    Harmonic get_z0_harmonic() {
        return z0Harmonic;
    }

    // Getter for harmonic count
    int get_harmonic_count() const {
        return harmonicCount;
    }
};



// Function to find the last Sunday of a given month
int lastSunday(int year, int month) {
    // Find the last day of the month
    int lastDayOfMonth = 31;
    if (month == 4 || month == 6 || month == 9 || month == 11) {
        lastDayOfMonth = 30;
    } else if (month == 2) {
        if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) {
            lastDayOfMonth = 29;
        } else {
            lastDayOfMonth = 28;
        }
    }

    // Create a MyDate object for the last day of the month
    MyDate lastDay(year, month, lastDayOfMonth);

    // Calculate the day of the week for the last day of the month
    // Using Zeller's Congruence algorithm
    int q = lastDay.day;
    int m = (lastDay.month < 3) ? lastDay.month + 12 : lastDay.month;
    int K = lastDay.year % 100;
    int J = lastDay.year / 100;

    int h = q + 13 * (m + 1) / 5 + K + K / 4 + J / 4 + 5 * J;
    h = h % 7;

    // Adjust to the last Sunday
    int lastSunday = lastDayOfMonth - h;
    return lastSunday;
}
// Function to determine the UTC offset for France
int franceTimeOffset(const MyDate& currentDate) {
    int lastSundayMarch = lastSunday(currentDate.year, 3); // March is month 3
    int lastSundayOctober = lastSunday(currentDate.year, 10); // October is month 10

    // Create date objects for the last Sundays
    MyDate lastSundayMarchDate(currentDate.year, 3, lastSundayMarch);
    MyDate lastSundayOctoberDate(currentDate.year, 10, lastSundayOctober);

    // Convert dates to comparable format
    int nowComparable = currentDate.year * 10000 + currentDate.month * 100 + currentDate.day;
    int lastSundayMarchComparable = lastSundayMarchDate.year * 10000 + lastSundayMarchDate.month * 100 + lastSundayMarchDate.day;
    int lastSundayOctoberComparable = lastSundayOctoberDate.year * 10000 + lastSundayOctoberDate.month * 100 + lastSundayOctoberDate.day;

    if (nowComparable >= lastSundayMarchComparable && nowComparable < lastSundayOctoberComparable) {
        // Daylight Saving Time (Summer Time)
        return 2; // UTC+2
    } else {
        // Standard Time (Winter Time)
        return 1; // UTC+1
    }
}



class HarmonicCalculator {
private:
    HarmonicModel &harmonic_model;
    Table2NCDef &table2NCDef;
    std::map<String, double> equilbrm;
    std::map<String, double> nodefctr;


public:
    HarmonicCalculator(HarmonicModel &model, Table2NCDef &table_def)
        : harmonic_model(model), table2NCDef(table_def) {
        equi_tide();
    }

    ~HarmonicCalculator() {
        equilbrm.clear();
        nodefctr.clear();
    }

    void equi_tide() {
        double T = 180.0;
        for (int i = 0; i < harmonic_model.get_harmonic_count(); i++) {
            Harmonic& harmonic = harmonic_model.get_harmonics()[i]; 
            Table2NC constituent = table2NCDef.get_constituent(harmonic.name);
            equilbrm[harmonic.name] = (constituent.T * T +
                                       constituent.s * s +
                                       constituent.h * h +
                                       constituent.p * p +
                                       constituent.p1 * p1 +
                                       constituent.deg +
                                       constituent.u_func());
            equilbrm[harmonic.name] = reduc360(equilbrm[harmonic.name]);
            nodefctr[harmonic.name] = constituent.f_func();
        }
    }
    std::string convertToReadableTime(int minutesSinceMidnight) {
        // Ensure input is within a valid range
        if (minutesSinceMidnight < 0 || minutesSinceMidnight >= 24 * 60) {
            return "Invalid time"; // Handle invalid input
        }

        // Adjust for the time zone offset
        minutesSinceMidnight %= 24 * 60; // Wrap around if it exceeds a day

        // Calculate hours and minutes
        int hours = minutesSinceMidnight / 60;
        int minutes = minutesSinceMidnight % 60;

        // Format hours and minutes as a string
        char timeBuffer[6]; // Enough for "HH:MM" + null terminator
        snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d", hours, minutes);

        return std::string(timeBuffer);
    }


    TideInfo findTidesAndTimes(int utcOffsetHours) {
        const int SAMPLES = 1440; // Assuming 140 samples in a day
        float hours[SAMPLES];
        float amplitudes[SAMPLES];

        // Fill hours and amplitudes arrays
        for (int i = 0; i < SAMPLES; ++i) {
            hours[i] = (i / 60.0);
            amplitudes[i] = amplitude(hours[i] - utcOffsetHours);
        }

        double highestPeak1 = -INFINITY, highestPeak2 = -INFINITY;
        int highestPeak1Index = -1, highestPeak2Index = -1;
        double lowestTrough1 = INFINITY, lowestTrough2 = INFINITY;
        int lowestTrough1Index = -1, lowestTrough2Index = -1;

        // Iterate through the amplitudes array to find peaks and troughs
        for (int i = 1; i < SAMPLES - 1; ++i) {
            // Check if it's a local maximum (peak)
            if (amplitudes[i] > amplitudes[i - 1] && amplitudes[i] > amplitudes[i + 1]) {
                if (amplitudes[i] > highestPeak1) {
                    highestPeak2 = highestPeak1;
                    highestPeak2Index = highestPeak1Index;

                    highestPeak1 = amplitudes[i];
                    highestPeak1Index = i;
                } else if (amplitudes[i] > highestPeak2) {
                    highestPeak2 = amplitudes[i];
                    highestPeak2Index = i;
                }
            }

            // Check if it's a local minimum (trough)
            if (amplitudes[i] < amplitudes[i - 1] && amplitudes[i] < amplitudes[i + 1]) {
                if (amplitudes[i] < lowestTrough1) {
                    lowestTrough2 = lowestTrough1;
                    lowestTrough2Index = lowestTrough1Index;
                    lowestTrough1 = amplitudes[i];
                    lowestTrough1Index = i;
                } else if (amplitudes[i] < lowestTrough2) {
                    lowestTrough2 = amplitudes[i];
                    lowestTrough2Index = i;
                }
            }
        }

        // Create a TideInfo object to hold the results
        TideInfo tideInfo;
        tideInfo.numPeaks = 0;
        tideInfo.numTroughs = 0;

        // Store the peaks
        if (highestPeak1Index != -1) {
            tideInfo.peaks[tideInfo.numPeaks++] = std::make_pair(highestPeak1, convertToReadableTime(highestPeak1Index));
        }
        if (highestPeak2Index != -1) {
            tideInfo.peaks[tideInfo.numPeaks++] = std::make_pair(highestPeak2, convertToReadableTime(highestPeak2Index));
        }

        // Store the troughs
        if (lowestTrough1Index != -1) {
            tideInfo.troughs[tideInfo.numTroughs++] = std::make_pair(lowestTrough1, convertToReadableTime(lowestTrough1Index));
        }
        if (lowestTrough2Index != -1) {
            tideInfo.troughs[tideInfo.numTroughs++] = std::make_pair(lowestTrough2, convertToReadableTime(lowestTrough2Index));
        }

        // Calculate the coefficients
        if (tideInfo.numPeaks > 0 && tideInfo.numTroughs > 0) {
            tideInfo.lowTideCoefficient = round(((tideInfo.peaks[0].first - tideInfo.troughs[0].first) / 6.1) * 100);
            tideInfo.highTideCoefficient = round(((tideInfo.peaks[tideInfo.numPeaks - 1].first - tideInfo.troughs[tideInfo.numTroughs - 1].first) / 6.1) * 100);
        } else {
            tideInfo.lowTideCoefficient = 0;
            tideInfo.highTideCoefficient = 0;
        }

        return tideInfo;
    }

    double amplitude(double t) {
      double total_amplitude = 0;
      Harmonic  harmonicZ0 = harmonic_model.get_z0_harmonic();
       for (int i = 0; i < harmonic_model.get_harmonic_count(); i++) {
           Harmonic& harmonic = harmonic_model.get_harmonics()[i]; 
           Table2NC constituent = table2NCDef.get_constituent(harmonic.name);
           if (harmonic.amplitude > 0.0) {  
               double var = reduc360(constituent.speed * t + equilbrm[harmonic.name] - harmonic.phase);
               total_amplitude += nodefctr[harmonic.name] * harmonic.amplitude * cos(radians(var));
           }
        }
        return harmonicZ0.amplitude + total_amplitude;
    }
};

double reduc360(double angle) {
    double result = fmod(angle, 360.0);
    return result < 0 ? result + 360.0 : result;
}


void displayTidesSortedByTime(const TideInfo& tides) {
    struct TideEntry {
        float level;
        std::string time;
        bool isHigh; // true for high tide, false for low tide
    };

    // Combine peaks and troughs into one array
    TideEntry combined[4];
    int combinedCount = 0;

    for (int i = 0; i < tides.numPeaks; ++i) {
        combined[combinedCount++] = {tides.peaks[i].first, tides.peaks[i].second, true};
    }

    for (int i = 0; i < tides.numTroughs; ++i) {
        combined[combinedCount++] = {tides.troughs[i].first, tides.troughs[i].second, false};
    }

    // Sort the combined array by time
    for (int i = 0; i < combinedCount - 1; ++i) {
        for (int j = i + 1; j < combinedCount; ++j) {
            if (combined[i].time > combined[j].time) {
                TideEntry temp = combined[i];
                combined[i] = combined[j];
                combined[j] = temp;
            }
        }
    }

    // Display the sorted tides
    for (int i = 0; i < combinedCount; ++i) {
        Serial.print(combined[i].isHigh ? "High tide: " : "Low tide: ");
        Serial.print(combined[i].time.c_str());
        Serial.print(" (");
        Serial.print(combined[i].level, 2);
        Serial.println("m)");
    }
}



  static Harmonic nonRefHarmonics[] = {
      {"E2", 0.010830917, 5.015839},
      {"J1", 0.003194284, 139.93358},
      {"K1", 0.06072261, 74.710915},
      {"K2", 0.1555091, 127.51676},
      {"L2", 0.03386434, 104.686165},
      {"M1", 0.005852548, 16.775864},
      {"M2", 1.5953193, 98.47019},
      {"M4", 0.11206561, 23.436436},
      {"M6", 0.018795298, 6.350185},
      {"MF", 0.020645503, 150.76256},
      {"MM", 0.018996427, -142.2232},
      {"N2", 0.3303059, 78.88077},
      {"O1", 0.064897336, -32.013294},
      {"P1", 0.024959715, 54.412617},
      {"Q1", 0.022134459, -80.21075},
      {"R2", 0.00512981, 120.21165},
      {"S2", 0.58522826, 131.7675},
      {"T2", 0.031807724, 124.00578},
      {"2N2", 0.044024024, 54.857944},
      {"2Q1", 0.003559956, -107.30296},
      {"KI1", 0.00161238, -6.171758},
      {"KJ2", 0.005639474, 141.04358},
      {"KQ1", 4.11604E-4, -132.51723},
      {"LAMBDA2", 0.014586009, 113.8545},
      {"MK4", 0.0153186, 103.52293},
      {"MN4", 0.044745367, -38.25004},
      {"MP1", 0.00157135, 77.29319},
      {"MS4", 0.040891398, 106.30928},
      {"MU2", 0.053256232, 54.468468},
      {"OO1", 0.001467735, -116.3549},
      {"PI1", 0.002542759, 17.953857},
      {"RO1", 0.005720264, -106.69113},
      {"PHI1", 0.001111132, 152.33963},
      {"PSI1", 0.002002684, 36.60781},
      {"SIGMA1", 0.005182066, -123.58492},
      {"THETA1", 0.001797353, 96.83272}
  };
  static Harmonic nonRefZ0Harmonic = {"Z0", 3, 0};

  static Harmonic refHarmonics[] = {
      {"2MN2S2", 0.0018, 283.54},
      {"2NS2", 0.0029, 104.91},
      {"3M2S2", 0.0038, 314.69},
      {"OQ2", 0.0042, 184.05},
      {"MNS2", 0.0189, 116.93},
      {"MNUS2", 0.0059, 130.58},
      {"2MK2", 0.0110, 192.58},
      {"2N2", 0.0566, 100.08},
      {"MU2", 0.0848, 132.92},
      {"N2", 0.4151, 118.87},
      {"NU2", 0.0779, 115.09},
      {"OP2", 0.0090, 211.87},
      {"GAMMA2", 0.0069, 307.03},
      {"M2", 2.0474, 137.79},
      {"MKS2", 0.0080, 203.54},
      {"LAMBDA2", 0.0259, 105.23},
      {"L2", 0.0658, 130.22},
      {"NKM2", 0.0116, 265.01},
      {"T2", 0.0419, 168.22},
      {"S2", 0.7489, 178.09},
      {"R2", 0.0050, 186.42},
      {"K2", 0.2140, 175.78},
      {"MSN2", 0.0139, 320.59},
      {"KJ2", 0.0090, 217.77},
      {"2SM2", 0.0168, 342.36},
      {"SKM2", 0.0079, 325.18},
      {"M(SK)2", 0.0109, 350.98},
      {"M(KS)2", 0.0115, 206.71},
  };
  static Harmonic z0RefHarmonic = {"Z0", 4.1364, 0.0};

  static Table2NC table2NcDefArray[]  = {
      {"E2", 2, -5, 4, 1, 0, 0, 27.423834, uM2, fM2},
      {"J1", 1, 1, 1, -1, 0, 90, 15.5854433, uJ1, fJ1},
      {"K1", 1, 0, 1, 0, 0, 90, 15.0410686, uK1, fK1},
      {"K2", 2, 0, 2, 0, 0, 0, 30.0821373, uK2, fK2},
      {"L2", 2, -1, 2, -1, 0, 180, 29.5284789, uL2, fL2},
      {"M1", 1, -1, 1, 1, 0, 90, 14.4966939, uM1, fM1},
      {"M2", 2, -2, 2, 0, 0, 0, 28.9841042, uM2, fM2},
      {"M4", 4, -4, 4, 0, 0, 0, 57.9682084, u2M2, f2M2},
      {"M6", 6, -6, 6, 0, 0, 0, 86.9523127, u3M2, f3M2},
      {"MF", 0, 2, 0, 0, 0, 0, 1.0980331, uMf, fMf},
      {"MM", 0, 1, 0, -1, 0, 0, 0.5443747,  zero, fMm},
      {"N2", 2, -3, 2, 1, 0, 0, 28.4397295, uM2, fM2},
      {"O1", 1, -2, 1, 0, 0, -90, 13.9430356, uO1, fO1},
      {"P1", 1, 0, -1, 0, 0, -90, 14.9589314, zero,  one},
      {"Q1", 1, -3, 1, 1, 0, -90, 13.3986609, uO1, fO1},
      {"R2", 2, 0, 1, 0, -1, 180, 30.0410667, zero,  one},
      {"S2", 2, 0, 0, 0, 0, 0, 30.0,zero,  one},
      {"T2", 2, 0, -1, 0, 1, 0, 29.9589333,  zero,  one},
      {"2N2", 2, -4, 2, 2, 0, 0, 27.8953548, uM2, fM2},
      {"2Q1", 1, -4, 1, 2, 0, -90, 12.8542862, uO1, fO1},
      {"MN4", 4, -5, 4, 1, 0, 0, 57.4238337, u2M2, f2M2},
      {"MK4", 4, -2, 4, 0, 0, 0, 59.0662415, uM2pK2, fM2K2},
      {"KI1", 1, -1, 3, -1, 0, -90, 14.5695476, uJ1, fJ1},
      {"KI1" ,1,-1,3,-1,0,-90,14.5695476,uJ1,fJ1},
      {"KJ2", 2, 1, 2, -1, 0, 0, 30.6265120, uK1pJ1, fK1J1},
      {"KQ1", 1, 3, 1, -1, 0, 90, 16.683476, uK2mQ1, fK2Q1},
      {"LAMBDA2", 2, -1, 0, 1, 0, 180, 29.4556253, uM2, fM2},
      {"MP1", 1, -2, 3, 0, 0, 0, 14.0251729, uM2, fM2},
      {"MS4", 4, -2, 2, 0, 0, 0, 58.9841042, uM2, fM2},
      {"MU2", 2, -4, 4, 0, 0, 0, 27.9682084, uM2, fM2},
      {"OO1", 1, 2, 1, 0, 0, 90, 16.1391017, uK2mQ1, fK2Q1},
      {"PI1", 1, 0, -2, 0, 1, -90, 14.9178647,  zero,  one},
      {"RO1" ,1,-3,3,-1, 0,-90,13.471515,uO1,fO1},
      {"PHI1", 1, 0, 3, 0, 0, 90, 15.1232059, uJ1, fJ1},
      {"PSI1", 1, 0, 2, 0, -1, 90, 15.0821353,  zero,  one},
      {"SIGMA1", 1, -4, 3, 0, 0, -90, 12.9271398, uO1, fO1},
      {"THETA1", 1, 1, -1, 1, 0, 90, 15.5125897, uJ1, fJ1},
      {"M3"      , 3,-3, 3, 0, 0,180, 43.4761563, uM3,      fM3},
      {"M8"      , 8,-8, 8, 0, 0,   0, 115.9364169, u4M2,    f4M2},
      {"2MN2S2" , 2,-7, 6, 1, 0, 0, 26.407938, u3M2,f3M2},
      {"2NS2" , 2,-6, 4, 2, 0, 0, 26.879459,u2M2,f2M2}, 
      {"3M2S2" , 2,-6, 6, 0, 0, 0, 26.952313,u3M2,f3M2},
      {"OQ2" , 2,-5, 2, 1, 0, 0, 27.341696,u2O1,f2O1},
      {"MNS2" , 2,-5, 4, 1, 0, 0, 27.423834,u2M2,f2M2},
      {"MNUS2" , 2,-5, 6,-1, 0, 0, 27.496687,u2M2,f2M2},
      {"2MK2" , 2,-4, 2, 0, 0, 0, 27.886071,u2M2mK2,f2M2K2},
      {"2N2" , 2,-4, 2, 2, 0, 0, 27.895355,uM2,fM2},
      {"MU2" , 2,-4, 4, 0, 0, 0, 27.968208,uM2,fM2},
      {"N2" , 2,-3, 2, 1, 0, 0, 28.439730,uM2,fM2},
      {"NU2" , 2,-3, 4,-1, 0, 0, 28.512583,uM2,fM2},
      {"OP2" ,2,-2, 0, 0, 0, 0, 28.901967,uO1,fO1},
      {"GAMMA2" , 2,-2, 0, 2, 0,180, 28.911251,uGamma2,fGamma2},
      {"M(SK)2" , 2,-2, 1, 0, 1,180, 28.943038,uM2pK1,fM2K1},
      {"M2" , 2,-2, 2, 0, 0, 0, 28.984104,uM2,fM2},
      {"M(KS)2" , 2,-2, 3, 0,-1, 0, 29.025171,uM2pK1,fM2K1},
      {"MKS2" , 2,-2, 4, 0, 0, 0, 29.066242,uM2pK2,fM2K2},
      {"LAMBDA2", 2,-1, 0, 1, 0,180, 29.455625,uM2,fM2},
      {"L2" , 2,-1, 2,-1, 0,180, 29.528479,uL2,fL2},
      {"NKM2" , 2,-1, 2, 1, 0, 0, 29.537763,uK2,f2M2K2},
      {"T2" ,2, 0,-1, 0, 1, 0, 29.958933,zero,one},
      {"S2" ,2, 0, 0, 0, 0, 0, 30.000000,zero,one},
      {"R2" ,2, 0, 1, 0, -1,180, 30.041067,zero,one},
      {"MSN2" ,2, 1, 0,-1, 0, 0, 30.544375,zero,f2M2},
      {"2SM2" , 2, 2,-2, 0, 0, 0, 31.015896,uminusM2,fM2},
      {"SKM2" , 2, 2, 0, 0, 0, 0, 31.098033,uK2mM2,fM2K2},
      {"S1"      , 1, 0, 0, 0, 0, -90, 15.0      , zero,    one},
      {"S4"      , 4, 0, 0, 0, 0,   0, 60.0      , zero,     one},
      {"S6" ,6, 0,0, 0, 0, 0, 90.000000,zero,one},
      {"RHO1"    , 1,-3, 3,-1, 0, -90, 13.4715145, uO1,     fO1},
      {"MK3"     , 3,-2, 3, 0, 0, +90, 44.0251729, uM2pK1,   fM2K1},
      {"2MK3"    , 3,-4, 3, 0, 0, -90, 42.9271398, u2M2pK1, f2M2K1},
      {"MSF"     , 0, 2,-2, 0, 0,   0,  1.0158958, uminusM2,fM2},
      {"SA"      , 0, 0, 1, 0, 0,   0,  0.0410686, zero,    one},
      {"SSA"     , 0, 0, 2, 0, 0,   0,  0.0821373, zero,    one},
      {"MS1"     , 1,-2, 2, 0, 0,180, 13.9841042, uM2,     fM2},
      {"MS3"     , 3,-2, 2, 0, 0,180, 43.9841042, uM2,      fM2},
      {"SP3"     , 3, 0,-1, 0, 0,   0, 44.9589314, zero,     one},
      {"S3"      , 3, 0, 0, 0, 0,   0, 45.0      , zero,     one},
      {"SK3"     , 3, 0, 1, 0, 0, +90, 45.0410686, uK1,      fK1},
      {"2MNS4"   , 4,-7, 6, 1, 0,   0, 56.4079380, u3M2,     f3M2},
      {"N4"      , 4,-6, 4, 2, 0,   0, 56.8794591, u2M2,     f2M2},
      {"3MS4"    , 4,-6, 6, 0, 0,   0, 56.9523127, u3M2,     f3M2},
      {"MNU4"    , 4,-5, 6,-1, 0,   0, 57.4966874, u2M2,     f2M2},
      {"2MSK4"   , 4,-4, 2, 0, 0,   0, 57.8860712, u2M2mK2,  f2M2K2},
      {"SN4"     , 4,-3, 2, 1, 0,   0, 58.4397295, uM2,      fM2},
      {"3MN4"    , 4,-3, 4,-1, 0,   0, 58.5125832, u2M2,     f4M2},
      {"NK4"     , 4,-3, 4, 1, 0,   0, 58.5218667, uM2pK2,   fM2K2},
      {"MT4"     , 4,-2, 1, 0, 1,   0, 58.9430375, uM2,      fM2},
      {"2SNM4"   , 4,-1, 0, 1, 0,   0, 59.4556253, zero,     f2M2},
      {"2MSN4"   , 4,-1, 2,-1, 0,   0, 59.5284789, uM2,      f3M2},
      {"SK4"     , 4, 0, 2, 0, 0,   0, 60.0821372, uK2,      fK2},
      {"3MNK6"   , 6,-9, 6, 1, 0,   0, 85.3099049, u4M2mK2, f4M2K2},
      {"3MNUS6"  , 6,-9,10,-1, 0,   0, 85.4648958, u4M2,    f4M2},
      {"4MK6"    , 6,-8, 6, 0, 0,   0, 85.8542796, u4M2mK2, f4M2K2},
      {"2NM6"    , 6,-8, 6, 2, 0,   0, 85.8635632, u3M2,    f3M2},
      {"4MS6"    , 6,-8, 8, 0, 0,   0, 85.9364169, u4M2,    f4M2},
      {"2MN6"    , 6,-7, 6, 1, 0,   0, 86.4079380, u3M2,    f3M2},
      {"2MNU6"   , 6,-7, 8,-1, 0,   0, 86.4807915, u3M2,    f3M2},
      {"3MSK6"   , 6,-6, 4, 0, 0,   0, 86.8701754, u3M2mK2, f3M2K2},
      {"3MNS6"   , 6,-9, 8, 1, 0,   0, 85.3920422, u4M2,    f4M2},
      {"M6"      , 6,-6, 6, 0, 0,   0, 86.9523127, u3M2,    f3M2},
      {"3MKS6"   , 6,-6, 8, 0, 0,   0, 87.0344499, u3M2pK2, f3M2K2},
      {"MSN6"    , 6,-5, 4, 1, 0,   0, 87.4238337, u2M2,    f2M2},
      {"4MN6"    , 6,-5, 6,-1, 0,   0, 87.4966874, u3M2,    f5M2},
      {"MNK6"    , 6,-5, 6, 1, 0,   0, 87.5059709, u2M2pK2, f2M2K2},
      {"2MT6"    , 6,-4, 3, 0, 1,   0, 87.9271417, u2M2,    f2M2},
      {"2MS6"    , 6,-4, 4, 0, 0,   0, 87.9682084, u2M2,    f2M2},
      {"2MK6"    , 6,-4, 6, 0, 0,   0, 88.0503457, u2M2pK2, f2M2K2},
      {"2SN6"    , 6,-3, 2, 1, 0,   0, 88.4397295, uM2,     fM2},
      {"3MSN6"   , 6,-3, 4,-1, 0,   0, 88.5125831, u2M2,    f4M2},
      {"3MKN6"   , 6,-3, 6,-1, 0,   0, 88.5947203, u2M2pK2, f4M2K2},
      {"2SM6"    , 6,-2, 2, 0, 0,   0, 88.9841042, uM2,     fM2},
      {"MSK6"    , 6,-2, 4, 0, 0,   0, 89.0662415, uM2pK2,  fM2K2},
      {"2(MN)8"  , 8,-10,8, 2, 0,   0, 114.8476675, u4M2,    f4M2},
      {"3MN8"    , 8,-9, 8, 1, 0,   0, 115.3920422, u4M2,    f4M2},
      {"3MNU8"   , 8,-9,10,-1, 0,   0, 115.4648957, u4M2,    f4M2},
      {"M8"      , 8,-8, 8, 0, 0,   0, 115.9364169, u4M2,    f4M2},
      {"2MSN8"   , 8,-7, 6, 1, 0,   0, 116.4079380, u3M2,    f3M2},
      {"3MS8"    , 8,-6, 6, 0, 0,   0, 116.9523127, u3M2,    f3M2},
      {"3MK8"    , 8,-6, 8, 0, 0,   0, 117.0344499, u3M2pK2, f3M2K2},
      {"4MSN8"   , 8,-5, 6,-1, 0,   0, 117.4966873, u3M2,    f5M2},
      {"2(MS)8"  , 8,-4, 4, 0, 0,   0, 117.9682084, u2M2,    f2M2},
      {"4MN10"   ,10,-11,10, 1, 0,  0, 144.3761463, u5M2,    f5M2},
      {"M10"     ,10,-10,10, 0, 0,  0, 144.9205210, u5M2,    f5M2},
      {"3MSN10"  ,10, -9, 8, 1, 0,  0, 145.3920421, u4M2,    f4M2},
      {"4MS10"   ,10, -8, 8, 0, 0,  0, 145.9364168, u4M2,    f4M2},
      {"5MSN10"  ,10, -7, 8,-1, 0,  0, 146.4807915, u4M2,    f6M2},
      {"3M2S10"  ,10, -6, 6, 0, 0,  0, 146.9523126, u3M2,    f3M2}
  };

TideStack tideStack;
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 0, 86400000 ); // UTC time with 24h refresh

int lastDay = -1; // Stores the last recorded day to detect midnight transition
const int daysToCalculate = 4;  // Number of days to calculate tides for

void run_calculations(const MyDate& date) {
    
 Serial.println("Calculating Astronomical references for date: " + String(date.day) + "/" + String(date.month) + "/" + String(date.year));

  astroCalculations(date);
  int utcOffsetHours = franceTimeOffset(date);
  Table2NCDef table2NCDef(table2NcDefArray);
  HarmonicModel model(nonRefHarmonics,  nonRefZ0Harmonic); 
  HarmonicModel modelRef(refHarmonics, z0RefHarmonic); 
  HarmonicCalculator calculatorRef(modelRef, table2NCDef);
  Serial.println("Calculating Coefficients...");
  TideInfo tides_ref = calculatorRef.findTidesAndTimes(utcOffsetHours);

  // Calculate Coefficient for Model Ref (Brest)
  Serial.print("Low Tide Coefficient: ");
  Serial.println(tides_ref.lowTideCoefficient);
  Serial.print("High Tide Coefficient: ");
  Serial.println(tides_ref.highTideCoefficient);

  // Calculate Tides and Times for Model (Le Palais)
  HarmonicCalculator calculator(model, table2NCDef);
  Serial.println("Calculating tide times for Belle-île...");
  
  TideInfo tides = calculator.findTidesAndTimes(utcOffsetHours);
  tides.lowTideCoefficient = tides_ref.lowTideCoefficient;
  tides.highTideCoefficient = tides_ref.highTideCoefficient;
  tides.date = date;
  tideStack.push(tides);

}



MyDate epochToDate(time_t epoch) {
    setTime(epoch);  // Set the current time to epoch
    return MyDate(year(), month(), day());
}



void setup() {
    Serial.begin(115200);
    while (!Serial);

    WiFi.begin(ssid);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    timeClient.begin();
    timeClient.update();
    setTime(1738454154);

    lastDay = day();
    Serial.println("Setup completed!");
    
    time_t currentEpoch = timeClient.getEpochTime();
    for (int i = 0; i < daysToCalculate; i++) {
        MyDate futureDate = epochToDate(currentEpoch + i * SECS_PER_DAY);
        run_calculations(futureDate);
    }
}

void loop() {
  //timeClient.update();
  //setTime(timeClient.getEpochTime());
  int currentDay = day();
  if (currentDay != lastDay) {
      Serial.println("Midnight transition detected! Updating tide data...");
      lastDay = currentDay;
      tideStack.pop(); // Remove the oldest entry
      time_t newEpoch = timeClient.getEpochTime() + (daysToCalculate - 1) * SECS_PER_DAY;
      MyDate newDay = epochToDate(newEpoch);
      run_calculations(newDay); // Calculate tide for the new day
  }

  for (int i = 0; i <= tideStack.getTop(); i++) {
      TideInfo tideInfo = tideStack.peek(i);
      
      Serial.print("Tide Info ");
      Serial.print(i + 1);
      Serial.println(":");

      // Print the date
      Serial.print("  Date: ");
      Serial.println(tideInfo.date.getDate().c_str());

      Serial.println("  Peaks:");
      for (int j = 0; j < tideInfo.numPeaks; j++) {
          Serial.print("    ");
          Serial.print(tideInfo.peaks[j].first);
          Serial.print(" meters at ");
          Serial.println(tideInfo.peaks[j].second.c_str());  
      }

      Serial.println("  Troughs:");
      for (int j = 0; j < tideInfo.numTroughs; j++) {
          Serial.print("    ");
          Serial.print(tideInfo.troughs[j].first);
          Serial.print(" meters at ");
          Serial.println(tideInfo.troughs[j].second.c_str()); 
      }

      Serial.print("  Low Tide Coefficient: ");
      Serial.println(tideInfo.lowTideCoefficient);
      Serial.print("  High Tide Coefficient: ");
      Serial.println(tideInfo.highTideCoefficient);
  }


    delay(1000);  // Check every second
}


