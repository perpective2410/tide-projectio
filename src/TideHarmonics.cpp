#include "TideHarmonics.h"
#include <math.h>

double reduc360(double angle) {
    double result = fmod(angle, 360.0);
    return result < 0 ? result + 360.0 : result;
}

// --- Base nodal corrections ---

double zero() { return 0.0; }
double one()  { return 1.0; }

double fMm() {
    return 1.0 - (0.1311 * cos(radians(N))) + (0.0538 * cos(radians(2.0 * p)))
               + (0.0205 * cos(radians(2.0 * p - N)));
}
double uMf() {
    return (-23.7 * sin(radians(N))) + (2.7 * sin(radians(2.0 * N)))
           - (0.4 * sin(radians(3.0 * N)));
}
double fMf() {
    return 1.084 + (0.415 * cos(radians(N))) + (0.039 * cos(radians(2.0 * N)));
}
double uO1() {
    return (10.80 * sin(radians(N))) - (1.34 * sin(radians(2.0 * N)))
           + (0.19 * sin(radians(3.0 * N)));
}
double fO1() {
    return 1.0176 + (0.1871 * cos(radians(N))) - (0.0147 * cos(radians(2.0 * N)));
}
double uK1() {
    return (-8.86 * sin(radians(N))) + (0.68 * sin(radians(2.0 * N)))
           - (0.07 * sin(radians(3.0 * N)));
}
double fK1() {
    return 1.0060 + (0.1150 * cos(radians(N))) - (0.0088 * cos(radians(2.0 * N)))
                  + (0.0006 * cos(radians(3.0 * N)));
}
double uJ1() {
    return (-12.94 * sin(radians(N))) + (1.34 * sin(radians(2.0 * N)))
           - (0.19 * sin(radians(3.0 * N)));
}
double fJ1() {
    return 1.1029 + (0.1676 * cos(radians(N))) - (0.0170 * cos(radians(2.0 * N)))
                  + (0.0016 * cos(radians(3.0 * N)));
}
double uM2() { return -2.14 * sin(radians(N)); }
double fM2() {
    return 1.0007 - (0.0373 * cos(radians(N))) + (0.0002 * cos(radians(2.0 * N)));
}
double uK2() {
    return (-17.74 * sin(radians(N))) + (0.68 * sin(radians(2.0 * N)))
           - (0.04 * sin(radians(3.0 * N)));
}
double fK2() {
    return 1.0246 + (0.2863 * cos(radians(N))) + (0.0083 * cos(radians(2.0 * N)))
                  - (0.0015 * cos(radians(3.0 * N)));
}
double uM3() { return -3.21 * sin(radians(N)); }
double fM3() { return pow(sqrt(fM2()), 3); }

static double fsinM1() { return sin(radians(p)) + (0.2 * sin(radians(p - N))); }
static double fcosM1() { return 2 * (cos(radians(p)) + (0.2 * cos(radians(p - N)))); }
double uM1() { return degrees(atan2(fsinM1(), fcosM1())); }
double fM1() { return sqrt(pow(fsinM1(), 2) + pow(fcosM1(), 2)); }

static double fsinGamma2() { return 0.147 * sin(radians(2 * (N - p))); }
static double fcosGamma2() { return 1.0 + (0.147 * cos(radians(2 * (N - p)))); }
double uGamma2() { return degrees(atan2(fsinGamma2(), fcosGamma2())); }
double fGamma2() { return sqrt(pow(fsinGamma2(), 2) + pow(fcosGamma2(), 2)); }

static double fsinL2() {
    return (-0.2505 * sin(radians(2 * p))) - (0.1102 * sin(radians(2 * p - N)))
           - (0.0156 * sin(radians(2 * p - 2 * N))) - (0.037 * sin(radians(N)));
}
static double fcosL2() {
    return 1.0 - (0.2505 * cos(radians(2 * p))) - (0.1102 * cos(radians(2 * p - N)))
               - (0.0156 * cos(radians(2 * p - 2 * N))) - (0.037 * cos(radians(N)));
}
double uL2() { return degrees(atan2(fsinL2(), fcosL2())); }
double fL2() { return sqrt(pow(fsinL2(), 2) + pow(fcosL2(), 2)); }

// E2 — dedicated nodal functions (distinct from M2)
double uE2() {
    return degrees(atan2(-0.439 * sin(radians(N)), 1.0 + (0.439 * cos(radians(N)))));
}
double fE2() {
    double s = -0.439 * sin(radians(N));
    double c =  1.0   + 0.439 * cos(radians(N));
    return sqrt(s * s + c * c);
}

// --- Compound u functions ---

double uminusM2()  { return -uM2(); }
double u2M2()      { return 2 * uM2(); }
double u3M2()      { return 3 * uM2(); }
double u4M2()      { return 4 * uM2(); }
double u5M2()      { return 5 * uM2(); }
double u6M2()      { return 6 * uM2(); }
double u7M2()      { return 7 * uM2(); }
double uminus2M2() { return -2 * uM2(); }

double uM2pK1()  { return uM2() + uK1(); }
double uM2mK1()  { return uM2() - uK1(); }
double u2M2pK1() { return 2 * uM2() + uK1(); }
double u2M2mK1() { return 2 * uM2() - uK1(); }
double u3M2pK1() { return 3 * uM2() + uK1(); }
double u3M2mK1() { return 3 * uM2() - uK1(); }
double u4M2pK1() { return 4 * uM2() + uK1(); }
double uM2p2K1() { return uM2() + 2 * uK1(); }
double uM2m2K1() { return uM2() - 2 * uK1(); }
double u3K1pM2() { return 3 * uK1() + uM2(); }

double uM2pK2()   { return uM2() + uK2(); }
double uM2mK2()   { return uM2() - uK2(); }
double uK2mM2()   { return uK2() - uM2(); }
double u2M2pK2()  { return 2 * uM2() + uK2(); }
double u2M2mK2()  { return 2 * uM2() - uK2(); }
double u3M2pK2()  { return 3 * uM2() + uK2(); }
double u3M2mK2()  { return 3 * uM2() - uK2(); }
double u4M2mK2()  { return 4 * uM2() - uK2(); }
double u4M2pK2()  { return 4 * uM2() + uK2(); }
double u5M2mK2()  { return 5 * uM2() - uK2(); }
double u5M2pK2()  { return 5 * uM2() + uK2(); }
double u6M2pK2()  { return 6 * uM2() + uK2(); }
double u6M2mK2()  { return 6 * uM2() - uK2(); }
double u2K2()     { return 2 * uK2(); }
double u2K2pM2()  { return uM2() + 2 * uK2(); }
double uK2m2M2()  { return uK2() - 2 * uM2(); }
double u2M2m2K2() { return 2 * uM2() - 2 * uK2(); }
double u2M2p2K2() { return 2 * uM2() + 2 * uK2(); }

double uK1pJ1()  { return uK1() + uJ1(); }
double uK2mQ1()  { return uK2() - uO1(); }  // note: KQ1 uses K2-O1
double uK1mO1()  { return uK1() - uO1(); }
double uK1pO1()  { return uK1() + uO1(); }
double uK1pK2()  { return uK1() + uK2(); }
double uM2mJ1()  { return uM2() - uJ1(); }
double uM2mO1()  { return uM2() - uO1(); }
double uM2pO1()  { return uM2() + uO1(); }
double u2O1()    { return 2 * uO1(); }

double u2M2mO1()  { return 2 * uM2() - uO1(); }
double u2M2pO1()  { return 2 * uM2() + uO1(); }
double u3M2mO1()  { return 3 * uM2() - uO1(); }
double u3M2pO1()  { return 3 * uM2() + uO1(); }
double u4M2mO1()  { return 4 * uM2() - uO1(); }

double uM2pK2pO1()  { return uM2() + uK2() + uO1(); }
double u2M2pK2pO1() { return 2 * uM2() + uK2() + uO1(); }

double uM2pL2()      { return uM2() + uL2(); }
double u2M2pL2()     { return 2 * uM2() + uL2(); }
double u3M2pL2()     { return 3 * uM2() + uL2(); }
double u4M2pL2()     { return 4 * uM2() + uL2(); }
double uM2pL2mK2()   { return uM2() + uL2() - 2 * uK2(); }
double u2M2pL2mK2()  { return 2 * uM2() + uL2() - uK2(); }
double uM2pL2pK2()   { return uM2() + uL2() + uK2(); }

double uminusK1() { return -uK1(); }
double uminusO1() { return -uO1(); }

// --- Compound f functions ---

double f2M2()  { return pow(fM2(), 2); }
double f3M2()  { return pow(fM2(), 3); }
double f4M2()  { return pow(fM2(), 4); }
double f5M2()  { return pow(fM2(), 5); }
double f6M2()  { return pow(fM2(), 6); }
double f7M2()  { return pow(fM2(), 7); }

double fM2K1()  { return fM2() * fK1(); }
double f2M2K1() { return pow(fM2(), 2) * fK1(); }
double f3M2K1() { return pow(fM2(), 3) * fK1(); }
double f4M2K1() { return pow(fM2(), 4) * fK1(); }
double f2K1M2() { return pow(fK1(), 2) * fM2(); }
double f3K1M2() { return pow(fK1(), 3) * fM2(); }

double fM2K2()   { return fM2() * fK2(); }
double f2M2K2()  { return pow(fM2(), 2) * fK2(); }
double f3M2K2()  { return pow(fM2(), 3) * fK2(); }
double f4M2K2()  { return pow(fM2(), 4) * fK2(); }
double f5M2K2()  { return pow(fM2(), 5) * fK2(); }
double f6M2K2()  { return pow(fM2(), 6) * fK2(); }
double fM22K2()  { return pow(fK2(), 2) * fM2(); }
double f2M22K2() { return pow(fM2(), 2) * pow(fK2(), 2); }
double f2K2()    { return pow(fK2(), 2); }

double fK1J1() { return fK1() * fJ1(); }
double fK2Q1() { return fK2() * fO1(); }
double fK1O1() { return fK1() * fO1(); }
double fK1K2() { return fK1() * fK2(); }
double fM2J1() { return fM2() * fJ1(); }
double fM2O1() { return fM2() * fO1(); }
double f2O1()  { return pow(fO1(), 2); }

double f2M2O1() { return pow(fM2(), 2) * fO1(); }
double f3M2O1() { return pow(fM2(), 3) * fO1(); }
double f4M2O1() { return pow(fM2(), 4) * fO1(); }

double fM2L2()    { return fM2() * fL2(); }
double f2M2L2()   { return pow(fM2(), 2) * fL2(); }
double f3M2L2()   { return pow(fM2(), 3) * fL2(); }
double f4M2L2()   { return pow(fM2(), 4) * fL2(); }
double f2M2L2K2() { return pow(fM2(), 2) * fL2() * fK2(); }
double fM2L2K2()  { return fM2() * fL2() * fK2(); }

double fM2K2O1()   { return fM2() * fK2() * fO1(); }
double f2M2K2O1()  { return pow(fM2(), 2) * fK2() * fO1(); }

// --- Constituent table ---

Table2NC table2NcDefArray[] = {
    {"E2",       2, -5,  4,  1,  0,   0, 27.423834,   uE2,       fE2},
    {"J1",       1,  1,  1, -1,  0,  90, 15.5854433,  uJ1,       fJ1},
    {"K1",       1,  0,  1,  0,  0,  90, 15.0410686,  uK1,       fK1},
    {"K2",       2,  0,  2,  0,  0,   0, 30.0821373,  uK2,       fK2},
    {"L2",       2, -1,  2, -1,  0, 180, 29.5284789,  uL2,       fL2},
    {"M1",       1, -1,  1,  1,  0,  90, 14.4966939,  uM1,       fM1},
    {"M2",       2, -2,  2,  0,  0,   0, 28.9841042,  uM2,       fM2},
    {"M4",       4, -4,  4,  0,  0,   0, 57.9682084,  u2M2,      f2M2},
    {"M6",       6, -6,  6,  0,  0,   0, 86.9523127,  u3M2,      f3M2},
    {"MF",       0,  2,  0,  0,  0,   0,  1.0980331,  uMf,       fMf},
    {"MM",       0,  1,  0, -1,  0,   0,  0.5443747,  zero,      fMm},
    {"N2",       2, -3,  2,  1,  0,   0, 28.4397295,  uM2,       fM2},
    {"O1",       1, -2,  1,  0,  0, -90, 13.9430356,  uO1,       fO1},
    {"P1",       1,  0, -1,  0,  0, -90, 14.9589314,  zero,      one},
    {"Q1",       1, -3,  1,  1,  0, -90, 13.3986609,  uO1,       fO1},
    {"R2",       2,  0,  1,  0, -1, 180, 30.0410667,  zero,      one},
    {"S2",       2,  0,  0,  0,  0,   0, 30.0,        zero,      one},
    {"T2",       2,  0, -1,  0,  1,   0, 29.9589333,  zero,      one},
    {"2N2",      2, -4,  2,  2,  0,   0, 27.8953548,  uM2,       fM2},
    {"2Q1",      1, -4,  1,  2,  0, -90, 12.8542862,  uO1,       fO1},
    {"MN4",      4, -5,  4,  1,  0,   0, 57.4238337,  u2M2,      f2M2},
    {"MK4",      4, -2,  4,  0,  0,   0, 59.0662415,  uM2pK2,    fM2K2},
    {"KI1",      1, -1,  3, -1,  0, -90, 14.5695476,  uJ1,       fJ1},
    {"KJ2",      2,  1,  2, -1,  0,   0, 30.6265120,  uK1pJ1,    fK1J1},
    {"KQ1",      1,  3,  1, -1,  0,  90, 16.683476,   uK2mQ1,    fK2Q1},
    {"LAMBDA2",  2, -1,  0,  1,  0, 180, 29.4556253,  uM2,       fM2},
    {"MP1",      1, -2,  3,  0,  0,   0, 14.0251729,  uM2,       fM2},
    {"MS4",      4, -2,  2,  0,  0,   0, 58.9841042,  uM2,       fM2},
    {"MU2",      2, -4,  4,  0,  0,   0, 27.9682084,  uM2,       fM2},
    {"OO1",      1,  2,  1,  0,  0,  90, 16.1391017,  uK2mQ1,    fK2Q1},
    {"PI1",      1,  0, -2,  0,  1, -90, 14.9178647,  zero,      one},
    {"RO1",      1, -3,  3, -1,  0, -90, 13.471515,   uO1,       fO1},
    {"PHI1",     1,  0,  3,  0,  0,  90, 15.1232059,  uJ1,       fJ1},
    {"PSI1",     1,  0,  2,  0, -1,  90, 15.0821353,  zero,      one},
    {"SIGMA1",   1, -4,  3,  0,  0, -90, 12.9271398,  uO1,       fO1},
    {"THETA1",   1,  1, -1,  1,  0,  90, 15.5125897,  uJ1,       fJ1},
    {"M3",       3, -3,  3,  0,  0, 180, 43.4761563,  uM3,       fM3},
    {"M8",       8, -8,  8,  0,  0,   0, 115.9364169, u4M2,      f4M2},
    {"2MN2S2",   2, -7,  6,  1,  0,   0, 26.407938,   u3M2,      f3M2},
    {"2NS2",     2, -6,  4,  2,  0,   0, 26.879459,   u2M2,      f2M2},
    {"3M2S2",    2, -6,  6,  0,  0,   0, 26.952313,   u3M2,      f3M2},
    {"OQ2",      2, -5,  2,  1,  0,   0, 27.341696,   u2O1,      f2O1},
    {"MNS2",     2, -5,  4,  1,  0,   0, 27.423834,   u2M2,      f2M2},
    {"MNUS2",    2, -5,  6, -1,  0,   0, 27.496687,   u2M2,      f2M2},
    {"2MK2",     2, -4,  2,  0,  0,   0, 27.886071,   u2M2mK2,   f2M2K2},
    {"MU2",      2, -4,  4,  0,  0,   0, 27.968208,   uM2,       fM2},
    {"N2",       2, -3,  2,  1,  0,   0, 28.439730,   uM2,       fM2},
    {"NU2",      2, -3,  4, -1,  0,   0, 28.512583,   uM2,       fM2},
    {"OP2",      2, -2,  0,  0,  0,   0, 28.901967,   uO1,       fO1},
    {"GAMMA2",   2, -2,  0,  2,  0, 180, 28.911251,   uGamma2,   fGamma2},
    {"M(SK)2",   2, -2,  1,  0,  1, 180, 28.943038,   uM2pK1,    fM2K1},
    {"MKS2",     2, -2,  4,  0,  0,   0, 29.066242,   uM2pK2,    fM2K2},
    {"NKM2",     2, -1,  2,  1,  0,   0, 29.537763,   uK2,       f2M2K2},
    {"T2",       2,  0, -1,  0,  1,   0, 29.958933,   zero,      one},
    {"R2",       2,  0,  1,  0, -1, 180, 30.041067,   zero,      one},
    {"MSN2",     2,  1,  0, -1,  0,   0, 30.544375,   zero,      f2M2},
    {"2SM2",     2,  2, -2,  0,  0,   0, 31.015896,   uminusM2,  fM2},
    {"SKM2",     2,  2,  0,  0,  0,   0, 31.098033,   uK2mM2,    fM2K2},
    {"M(KS)2",   2, -2,  3,  0, -1,   0, 29.025171,   uM2pK1,    fM2K1},
    {"S1",       1,  0,  0,  0,  0, -90, 15.0,        zero,      one},
    {"S4",       4,  0,  0,  0,  0,   0, 60.0,        zero,      one},
    {"S6",       6,  0,  0,  0,  0,   0, 90.0,        zero,      one},
    {"RHO1",     1, -3,  3, -1,  0, -90, 13.4715145,  uO1,       fO1},
    {"MK3",      3, -2,  3,  0,  0, +90, 44.0251729,  uM2pK1,    fM2K1},
    {"2MK3",     3, -4,  3,  0,  0, -90, 42.9271398,  u2M2pK1,   f2M2K1},
    {"MSF",      0,  2, -2,  0,  0,   0,  1.0158958,  uminusM2,  fM2},
    {"SA",       0,  0,  1,  0,  0,   0,  0.0410686,  zero,      one},
    {"SSA",      0,  0,  2,  0,  0,   0,  0.0821373,  zero,      one},
    {"MS1",      1, -2,  2,  0,  0, 180, 13.9841042,  uM2,       fM2},
    {"MS3",      3, -2,  2,  0,  0, 180, 43.9841042,  uM2,       fM2},
    {"SP3",      3,  0, -1,  0,  0,   0, 44.9589314,  zero,      one},
    {"S3",       3,  0,  0,  0,  0,   0, 45.0,        zero,      one},
    {"SK3",      3,  0,  1,  0,  0, +90, 45.0410686,  uK1,       fK1},
    {"2MNS4",    4, -7,  6,  1,  0,   0, 56.4079380,  u3M2,      f3M2},
    {"N4",       4, -6,  4,  2,  0,   0, 56.8794591,  u2M2,      f2M2},
    {"3MS4",     4, -6,  6,  0,  0,   0, 56.9523127,  u3M2,      f3M2},
    {"MNU4",     4, -5,  6, -1,  0,   0, 57.4966874,  u2M2,      f2M2},
    {"2MSK4",    4, -4,  2,  0,  0,   0, 57.8860712,  u2M2mK2,   f2M2K2},
    {"SN4",      4, -3,  2,  1,  0,   0, 58.4397295,  uM2,       fM2},
    {"3MN4",     4, -3,  4, -1,  0,   0, 58.5125832,  u2M2,      f4M2},
    {"NK4",      4, -3,  4,  1,  0,   0, 58.5218667,  uM2pK2,    fM2K2},
    {"MT4",      4, -2,  1,  0,  1,   0, 58.9430375,  uM2,       fM2},
    {"2SNM4",    4, -1,  0,  1,  0,   0, 59.4556253,  zero,      f2M2},
    {"2MSN4",    4, -1,  2, -1,  0,   0, 59.5284789,  uM2,       f3M2},
    {"SK4",      4,  0,  2,  0,  0,   0, 60.0821372,  uK2,       fK2},
    {"3MNK6",    6, -9,  6,  1,  0,   0, 85.3099049,  u4M2mK2,   f4M2K2},
    {"3MNUS6",   6, -9, 10, -1,  0,   0, 85.4648958,  u4M2,      f4M2},
    {"4MK6",     6, -8,  6,  0,  0,   0, 85.8542796,  u4M2mK2,   f4M2K2},
    {"2NM6",     6, -8,  6,  2,  0,   0, 85.8635632,  u3M2,      f3M2},
    {"4MS6",     6, -8,  8,  0,  0,   0, 85.9364169,  u4M2,      f4M2},
    {"2MN6",     6, -7,  6,  1,  0,   0, 86.4079380,  u3M2,      f3M2},
    {"2MNU6",    6, -7,  8, -1,  0,   0, 86.4807915,  u3M2,      f3M2},
    {"3MSK6",    6, -6,  4,  0,  0,   0, 86.8701754,  u3M2mK2,   f3M2K2},
    {"3MNS6",    6, -9,  8,  1,  0,   0, 85.3920422,  u4M2,      f4M2},
    {"3MKS6",    6, -6,  8,  0,  0,   0, 87.0344499,  u3M2pK2,   f3M2K2},
    {"MSN6",     6, -5,  4,  1,  0,   0, 87.4238337,  u2M2,      f2M2},
    {"4MN6",     6, -5,  6, -1,  0,   0, 87.4966874,  u3M2,      f5M2},
    {"MNK6",     6, -5,  6,  1,  0,   0, 87.5059709,  u2M2pK2,   f2M2K2},
    {"2MT6",     6, -4,  3,  0,  1,   0, 87.9271417,  u2M2,      f2M2},
    {"2MS6",     6, -4,  4,  0,  0,   0, 87.9682084,  u2M2,      f2M2},
    {"2MK6",     6, -4,  6,  0,  0,   0, 88.0503457,  u2M2pK2,   f2M2K2},
    {"2SN6",     6, -3,  2,  1,  0,   0, 88.4397295,  uM2,       fM2},
    {"3MSN6",    6, -3,  4, -1,  0,   0, 88.5125831,  u2M2,      f4M2},
    {"3MKN6",    6, -3,  6, -1,  0,   0, 88.5947203,  u2M2pK2,   f4M2K2},
    {"2SM6",     6, -2,  2,  0,  0,   0, 88.9841042,  uM2,       fM2},
    {"MSK6",     6, -2,  4,  0,  0,   0, 89.0662415,  uM2pK2,    fM2K2},
    {"2(MN)8",   8,-10,  8,  2,  0,   0, 114.8476675, u4M2,      f4M2},
    {"3MN8",     8, -9,  8,  1,  0,   0, 115.3920422, u4M2,      f4M2},
    {"3MNU8",    8, -9, 10, -1,  0,   0, 115.4648957, u4M2,      f4M2},
    {"2MSN8",    8, -7,  6,  1,  0,   0, 116.4079380, u3M2,      f3M2},
    {"3MS8",     8, -6,  6,  0,  0,   0, 116.9523127, u3M2,      f3M2},
    {"3MK8",     8, -6,  8,  0,  0,   0, 117.0344499, u3M2pK2,   f3M2K2},
    {"4MSN8",    8, -5,  6, -1,  0,   0, 117.4966873, u3M2,      f5M2},
    {"2(MS)8",   8, -4,  4,  0,  0,   0, 117.9682084, u2M2,      f2M2},
    {"4MN10",   10,-11, 10,  1,  0,   0, 144.3761463, u5M2,      f5M2},
    {"M10",     10,-10, 10,  0,  0,   0, 144.9205210, u5M2,      f5M2},
    {"3MSN10",  10, -9,  8,  1,  0,   0, 145.3920421, u4M2,      f4M2},
    {"4MS10",   10, -8,  8,  0,  0,   0, 145.9364168, u4M2,      f4M2},
    {"5MSN10",  10, -7,  8, -1,  0,   0, 146.4807915, u4M2,      f6M2},
    {"3M2S10",  10, -6,  6,  0,  0,   0, 146.9523126, u3M2,      f3M2},
};

const int TABLE2NC_COUNT = sizeof(table2NcDefArray) / sizeof(table2NcDefArray[0]);
