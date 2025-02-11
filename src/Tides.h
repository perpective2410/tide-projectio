#include <Arduino.h>

double reduc360(double angle);
String convertDecimalTimeToHM(float decimalTime);

struct TideEvent {
    double amplitude;
    float time;
    bool isPeak; // true if peak, false if trough
};

struct TideInfo {
    TideEvent events[4]; // Up to 2 peaks and 2 troughs
    int numEvents;
    int morningCoefficient;
    int afternoonCoefficient;
    time_t epoch;

    TideInfo() : numEvents(0), morningCoefficient(0), afternoonCoefficient(0) {
        epoch = -1;
    }
};

class TideStack {
    private:
        int stackSize;
        TideInfo* stack;
        int count;
    
    public:
        TideStack(int daysToCalculate);
        ~TideStack();
    
        void push(const TideInfo& tideInfo);
    
        // Getter methods for printing
        const TideInfo* getStack() const;
        int getCount() const;
    
        // Method to get the top index
        int getTop() const;
    
        // Method to peek at an index
        const TideInfo& peek(int index) const;
    };
    
TideInfo run_calculations(time_t epoch);

struct Table2NC {
    String name;
    double T, s, ls, p, p1, deg, speed;
    double (*u_func)();
    double (*f_func)();
};