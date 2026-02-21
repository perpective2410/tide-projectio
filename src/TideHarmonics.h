#pragma once
#include <Arduino.h>

// Astronomical variables — set by astroCalculations() in Tides.cpp before each calculation
extern double T;   // Julian century from J2000.0
extern double s;   // Moon's mean longitude
extern double ls;  // Sun's mean longitude
extern double p;   // Moon's mean perigee longitude
extern double M;   // Sun's mean anomaly
extern double p1;  // Sun's mean perigee
extern double N;   // Longitude of moon's ascending node

double reduc360(double angle);

// --- Base nodal corrections ---
// f_func : amplitude (node) correction factor
// u_func : phase correction (degrees)
double zero();
double one();
double fMm();
double uMf();     double fMf();
double uO1();     double fO1();
double uK1();     double fK1();
double uJ1();     double fJ1();
double uM2();     double fM2();
double uK2();     double fK2();
double uM3();     double fM3();
double uM1();     double fM1();
double uGamma2(); double fGamma2();
double uL2();     double fL2();
double uE2();     double fE2();

// --- Compound u (phase correction) functions ---
double uminusM2();
double u2M2();    double u3M2();    double u4M2();
double u5M2();    double u6M2();    double u7M2();
double uminus2M2();
double uM2pK1();  double uM2mK1();
double u2M2pK1(); double u2M2mK1();
double u3M2pK1(); double u3M2mK1();
double u4M2pK1();
double uM2p2K1(); double uM2m2K1();
double u3K1pM2();
double uM2pK2();  double uM2mK2();  double uK2mM2();
double u2M2pK2(); double u2M2mK2();
double u3M2pK2(); double u3M2mK2();
double u4M2mK2(); double u4M2pK2();
double u5M2mK2(); double u5M2pK2();
double u6M2pK2(); double u6M2mK2();
double u2K2();
double u2K2pM2(); double uK2m2M2();
double u2M2m2K2(); double u2M2p2K2();
double uK1pJ1();
double uK2mQ1();
double uK1mO1();  double uK1pO1();
double uK1pK2();
double uM2mJ1();
double uM2mO1();  double uM2pO1();
double u2O1();
double u2M2mO1(); double u2M2pO1();
double u3M2mO1(); double u3M2pO1();
double u4M2mO1();
double uM2pK2pO1();  double u2M2pK2pO1();
double uM2pL2();   double u2M2pL2();  double u3M2pL2(); double u4M2pL2();
double uM2pL2mK2(); double u2M2pL2mK2(); double uM2pL2pK2();
double uminusK1();
double uminusO1();

// --- Compound f (amplitude correction) functions ---
double f2M2();    double f3M2();    double f4M2();
double f5M2();    double f6M2();    double f7M2();
double fM2K1();
double f2M2K1();  double f3M2K1();  double f4M2K1();
double f2K1M2();  double f3K1M2();
double fM2K2();
double f2M2K2();  double f3M2K2();  double f4M2K2();
double f5M2K2();  double f6M2K2();
double fM22K2();  double f2M22K2();
double f2K2();
double fK1J1();
double fK2Q1();
double fK1O1();
double fK1K2();
double fM2J1();
double fM2O1();
double f2O1();
double f2M2O1();  double f3M2O1();  double f4M2O1();
double fM2L2();   double f2M2L2();  double f3M2L2(); double f4M2L2();
double f2M2L2K2(); double fM2L2K2();
double fM2K2O1(); double f2M2K2O1();

// --- Constituent database ---
// name is const char* (not String) so the array has trivial construction —
// avoids static-initialization-order issues across translation units.
struct Table2NC {
    const char* name;
    double T, s, ls, p, p1, deg, speed;
    double (*u_func)();
    double (*f_func)();
};

extern Table2NC table2NcDefArray[];
extern const int TABLE2NC_COUNT;
