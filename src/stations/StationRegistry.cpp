// Station registry — maps station ids to their compiled-in StationDef objects.
//
// To add a new station:
//   1. Create src/stations/MyCity.cpp  (copy LePalais.cpp as a template)
//   2. Add   extern const StationDef STATION_MY_CITY;   below
//   3. Add   &STATION_MY_CITY                           to REGISTRY[]

#include "StationDef.h"
#include <string.h>

extern const StationDef STATION_LE_PALAIS;
extern const StationDef STATION_BREST;

static const StationDef* const REGISTRY[] = {
    &STATION_LE_PALAIS,
    &STATION_BREST,
};
static const int REGISTRY_SIZE = (int)(sizeof(REGISTRY) / sizeof(REGISTRY[0]));

const StationDef* findStation(const char* id) {
    for (int i = 0; i < REGISTRY_SIZE; i++) {
        if (strcmp(REGISTRY[i]->id, id) == 0)
            return REGISTRY[i];
    }
    return nullptr;
}
