// Tidal station: Brest, France — used as the reference for French tide coefficients.
// refHigh = 6.1 m  →  coefficient = |HM - BM| / 6.1 * 100
// Lat 48.38333 / Lon -4.49333

#include "StationDef.h"

static const HarmonicConst HARMONICS[] = {
    // name        amplitude (m)   phase (°)
    { "2MN2S2",    0.0018,  283.54 },
    { "2NS2",      0.0029,  104.91 },
    { "3M2S2",     0.0038,  314.69 },
    { "OQ2",       0.0042,  184.05 },
    { "MNS2",      0.0189,  116.93 },
    { "MNUS2",     0.0059,  130.58 },
    { "2MK2",      0.0110,  192.58 },
    { "2N2",       0.0566,  100.08 },
    { "MU2",       0.0848,  132.92 },
    { "N2",        0.4151,  118.87 },
    { "NU2",       0.0779,  115.09 },
    { "OP2",       0.0090,  211.87 },
    { "GAMMA2",    0.0069,  307.03 },
    { "M2",        2.0474,  137.79 },
    { "MKS2",      0.0080,  203.54 },
    { "LAMBDA2",   0.0259,  105.23 },
    { "L2",        0.0658,  130.22 },
    { "NKM2",      0.0116,  265.01 },
    { "T2",        0.0419,  168.22 },
    { "S2",        0.7489,  178.09 },
    { "R2",        0.0050,  186.42 },
    { "K2",        0.2140,  175.78 },
    { "MSN2",      0.0139,  320.59 },
    { "KJ2",       0.0090,  217.77 },
    { "2SM2",      0.0168,  342.36 },
    { "SKM2",      0.0079,  325.18 },
    { "M(SK)2",    0.0109,  350.98 },
    { "M(KS)2",    0.0115,  206.71 },
};

extern const StationDef STATION_BREST = {
    "Brest",
    4.1364,  // z0 — datum offset (m)
    0.0,     // refLow  (reserved)
    6.1,     // refHigh — tidal range used for coefficient calculation
    HARMONICS,
    (int)(sizeof(HARMONICS) / sizeof(HARMONICS[0]))
};
