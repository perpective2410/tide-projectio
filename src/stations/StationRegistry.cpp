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
//extern const StationDef STATION_SHEERNESS;
//extern const StationDef STATION_DOVER;
//extern const StationDef STATION_BRIGHTON;
//extern const StationDef STATION_BOURNEMOUTH;
//extern const StationDef STATION_PENZANCE;
//extern const StationDef STATION_CARDIFF;
//extern const StationDef STATION_ISLES_OF_SCILLY;
//extern const StationDef STATION_OOSTENDE;
//extern const StationDef STATION_DUNKERQUE;
//extern const StationDef STATION_GRAND_FORT_PHILIPPE;
//extern const StationDef STATION_CALAIS;
//extern const StationDef STATION_BOULOGNE_SUR_MER;
//extern const StationDef STATION_LE_TOUQUET;
//extern const StationDef STATION_STELLA_PLAGE;
//extern const StationDef STATION_BERCK_SUR_MER;
//extern const StationDef STATION_FORT_MAHON_PLAGE;
//extern const StationDef STATION_SAINT_VALERY_SUR_SOMME;
//extern const StationDef STATION_CAYEUX_SUR_MER;
//extern const StationDef STATION_LE_TREPORT;
//extern const StationDef STATION_DIEPPE;
//extern const StationDef STATION_FECAMP;
//extern const StationDef STATION_YPORT;
//extern const StationDef STATION_ETRETAT;
//extern const StationDef STATION_LE_HAVRE;
//extern const StationDef STATION_DEAUVILLE;
//extern const StationDef STATION_VILLERS_SUR_MER;
//extern const StationDef STATION_HOULGATE;
//extern const StationDef STATION_OUISTREHAM;
//extern const StationDef STATION_COURSEULLES_SUR_MER;
//extern const StationDef STATION_PORT_EN_BESSIN;
//extern const StationDef STATION_GRANDCAMP_MAISY;
//extern const StationDef STATION_SAINT_VAAST_LA_HOUGUE;
//extern const StationDef STATION_BARFLEUR;
//extern const StationDef STATION_CHERBOURG;
//extern const StationDef STATION_GOURY;
//extern const StationDef STATION_DIELETTE;
//extern const StationDef STATION_SAINT_PETER_PORT;
//extern const StationDef STATION_CARTERET;
//extern const StationDef STATION_PORTBAIL;
//extern const StationDef STATION_BLAINVILLE_SUR_MER;
//extern const StationDef STATION_SAINT_HELIER;
//extern const StationDef STATION_ILES_CHAUSEY;
//extern const StationDef STATION_GRANVILLE;
//extern const StationDef STATION_ST_PAIR_SUR_MER;
//extern const StationDef STATION_CANCALE;
//extern const StationDef STATION_SAINT_MALO;
//extern const StationDef STATION_LANCIEUX;
//extern const StationDef STATION_SAINT_CAST_LE_GUILDO;
//extern const StationDef STATION_ERQUY;
//extern const StationDef STATION_PLENEUF_VAL_ANDRE;
//extern const StationDef STATION_DAHOUET;
//extern const StationDef STATION_LE_LEGUE;
//extern const StationDef STATION_BINIC;
//extern const StationDef STATION_PAIMPOL;
//extern const StationDef STATION_BREHAT;
//extern const StationDef STATION_PORT_BENI;
//extern const StationDef STATION_PERROS_GUIREC;
//extern const StationDef STATION_PLOUMANACH;
//extern const StationDef STATION_TREBEURDEN;
//extern const StationDef STATION_ST_MICHEL_EN_GREVE;
//extern const StationDef STATION_TERENEZ;
//extern const StationDef STATION_ROSCOFF;
//extern const StationDef STATION_BRIGNOGAN_PLAGE;
//extern const StationDef STATION_LABERWRACH;
//extern const StationDef STATION_SAINT_PABU;
//extern const StationDef STATION_PORTSALL;
//extern const StationDef STATION_LANILDUT;
//extern const StationDef STATION_LE_CONQUET;
//extern const StationDef STATION_CAMARET_SUR_MER;
//extern const StationDef STATION_MORGAT;
//extern const StationDef STATION_DOUARNENEZ;
//extern const StationDef STATION_AUDIERNE;
//extern const StationDef STATION_TREGUENNEC;
//extern const StationDef STATION_LE_GUILVINEC;
//extern const StationDef STATION_LOCTUDY;
//extern const StationDef STATION_BENODET;
//extern const StationDef STATION_CONCARNEAU;
//extern const StationDef STATION_RIEC_SUR_BELON;
//extern const StationDef STATION_PORT_TUDY_GROIX;
//extern const StationDef STATION_PORT_LOUIS;
//extern const StationDef STATION_LORIENT;
//extern const StationDef STATION_ETEL;
//extern const StationDef STATION_LE_LOGEO;
//extern const StationDef STATION_PORTIVY;
//extern const StationDef STATION_SAUZON;
//extern const StationDef STATION_LOCMARIA;
//extern const StationDef STATION_LES_DONNANTS;
//extern const StationDef STATION_QUIBERON;
//extern const StationDef STATION_CARNAC;
//extern const StationDef STATION_LA_TRINITE_SUR_MER;
//extern const StationDef STATION_AURAY_LE_BONO;
//extern const StationDef STATION_PORT_NAVALO;
//extern const StationDef STATION_LE_CROUESTY;
//extern const StationDef STATION_ARRADON;
//extern const StationDef STATION_LARMOR_BADEN;
//extern const StationDef STATION_VANNES_PORT_ANNA_SENE;
//extern const StationDef STATION_ST_GILDAS_DE_RHUYS;
//extern const StationDef STATION_PORT_SAINT_JACQUES;
//extern const StationDef STATION_LE_ROALIGUEN;
//extern const StationDef STATION_PENERF;
//extern const StationDef STATION_HOUAT;
//extern const StationDef STATION_HOEDIC;
//extern const StationDef STATION_PENESTIN;
//extern const StationDef STATION_QUIMIAC;
//extern const StationDef STATION_PIRIAC_SUR_MER;
//extern const StationDef STATION_LA_TURBALLE;
//extern const StationDef STATION_LE_CROISIC;
//extern const StationDef STATION_LE_POULIGUEN;
//extern const StationDef STATION_PORNICHET;
//extern const StationDef STATION_SAINT_NAZAIRE;
//extern const StationDef STATION_LE_PELLERIN;
//extern const StationDef STATION_CORDEMAIS;
//extern const StationDef STATION_NANTES_SALORGES;
//extern const StationDef STATION_NANTES_USINE_BRULEE;
//extern const StationDef STATION_PORT_LA_GRAVETTE;
//extern const StationDef STATION_PAIMBOEUF;
//extern const StationDef STATION_MONTOIR_DE_BRETAGNE;
//extern const StationDef STATION_PORNIC;
//extern const StationDef STATION_LHERBAUDIERE;
//extern const StationDef STATION_PORT_JOINVILLE;
//extern const StationDef STATION_SAINT_JEAN_DE_MONTS;
//extern const StationDef STATION_SAINT_GILLES_CROIX_DE_VIE;
//extern const StationDef STATION_LES_SABLES_DOLONNE;
//extern const StationDef STATION_ARS_EN_RE;
//extern const StationDef STATION_ST_MARTIN_EN_RE;
//extern const StationDef STATION_PORT_DU_PLOMB;
//extern const StationDef STATION_LA_ROCHELLE;
//extern const StationDef STATION_SAINT_DENIS_DOLERON;
//extern const StationDef STATION_LES_SABLES_VIGNIERS;
//extern const StationDef STATION_LA_CAYENNE;
//extern const StationDef STATION_LA_COTINIERE;
//extern const StationDef STATION_ILE_DAIX;
//extern const StationDef STATION_LA_COUBRE;
//extern const StationDef STATION_LA_PALMYRE;
//extern const StationDef STATION_ROYAN;
//extern const StationDef STATION_ST_GEORGES_DE_DIDONNE;
//extern const StationDef STATION_CORDOUAN;
//extern const StationDef STATION_PORT_BLOC;
//extern const StationDef STATION_SOULAC;
//extern const StationDef STATION_LAMENA;
//extern const StationDef STATION_BORDEAUX;
//extern const StationDef STATION_BLAYE;
//extern const StationDef STATION_MONTALIVET;
//extern const StationDef STATION_CAP_FERRET;
//extern const StationDef STATION_ARCACHON;
//extern const StationDef STATION_BISCARROSSE;
//extern const StationDef STATION_MIMIZAN;
//extern const StationDef STATION_ST_JEAN_DE_LUZ;
//extern const StationDef STATION_CAPBRETON_HOSSEGOR;
//extern const StationDef STATION_BOUCAU_BAYONNE;
//extern const StationDef STATION_BIARRITZ;
//extern const StationDef STATION_SOCOA;
//extern const StationDef STATION_BILBAO;
//extern const StationDef STATION_PORT_VENDRES;
//extern const StationDef STATION_PORT_LA_NOUVELLE;
//extern const StationDef STATION_SETE;
//extern const StationDef STATION_PORT_FERREOL;
//extern const StationDef STATION_PORT_CAMARGUE;
//extern const StationDef STATION_FOS_SUR_MER;
//extern const StationDef STATION_LA_FIGUEIREITTE;
//extern const StationDef STATION_TOULON;
//extern const StationDef STATION_MARSEILLE;
//extern const StationDef STATION_NICE;
//extern const StationDef STATION_MONACO_FONVIEILLE;
//extern const StationDef STATION_MONACO_PORT_HERCULE;
//extern const StationDef STATION_AJACCIO_ASPRETTO;
//extern const StationDef STATION_CENTURI;
//extern const StationDef STATION_ILE_ROUSSE;
//extern const StationDef STATION_SOLENZARA;
//extern const StationDef STATION_MAJURO;

static const StationDef* const REGISTRY[] = {
    &STATION_LE_PALAIS,
    &STATION_BREST,
    //&STATION_SHEERNESS,
    //&STATION_DOVER,
    //&STATION_BRIGHTON,
    //&STATION_BOURNEMOUTH,
    //&STATION_PENZANCE,
    //&STATION_CARDIFF,
    //&STATION_ISLES_OF_SCILLY,
    //&STATION_OOSTENDE,
    //&STATION_DUNKERQUE,
    //&STATION_GRAND_FORT_PHILIPPE,
    //&STATION_CALAIS,
    //&STATION_BOULOGNE_SUR_MER,
    //&STATION_LE_TOUQUET,
    //&STATION_STELLA_PLAGE,
    //&STATION_BERCK_SUR_MER,
    //&STATION_FORT_MAHON_PLAGE,
    //&STATION_SAINT_VALERY_SUR_SOMME,
    //&STATION_CAYEUX_SUR_MER,
    //&STATION_LE_TREPORT,
    //&STATION_DIEPPE,
    //&STATION_FECAMP,
    //&STATION_YPORT,
    //&STATION_ETRETAT,
    //&STATION_LE_HAVRE,
    //&STATION_DEAUVILLE,
    //&STATION_VILLERS_SUR_MER,
    //&STATION_HOULGATE,
    //&STATION_OUISTREHAM,
    //&STATION_COURSEULLES_SUR_MER,
    //&STATION_PORT_EN_BESSIN,
    //&STATION_GRANDCAMP_MAISY,
    //&STATION_SAINT_VAAST_LA_HOUGUE,
    //&STATION_BARFLEUR,
    //&STATION_CHERBOURG,
    //&STATION_GOURY,
    //&STATION_DIELETTE,
    //&STATION_SAINT_PETER_PORT,
    //&STATION_CARTERET,
    //&STATION_PORTBAIL,
    //&STATION_BLAINVILLE_SUR_MER,
    //&STATION_SAINT_HELIER,
    //&STATION_ILES_CHAUSEY,
    //&STATION_GRANVILLE,
    //&STATION_ST_PAIR_SUR_MER,
    //&STATION_CANCALE,
    //&STATION_SAINT_MALO,
    //&STATION_LANCIEUX,
    //&STATION_SAINT_CAST_LE_GUILDO,
    //&STATION_ERQUY,
    //&STATION_PLENEUF_VAL_ANDRE,
    //&STATION_DAHOUET,
    //&STATION_LE_LEGUE,
    //&STATION_BINIC,
    //&STATION_PAIMPOL,
    //&STATION_BREHAT,
    //&STATION_PORT_BENI,
    //&STATION_PERROS_GUIREC,
    //&STATION_PLOUMANACH,
    //&STATION_TREBEURDEN,
    //&STATION_ST_MICHEL_EN_GREVE,
    //&STATION_TERENEZ,
    //&STATION_ROSCOFF,
    //&STATION_BRIGNOGAN_PLAGE,
    //&STATION_LABERWRACH,
    //&STATION_SAINT_PABU,
    //&STATION_PORTSALL,
    //&STATION_LANILDUT,
    //&STATION_LE_CONQUET,
    //&STATION_CAMARET_SUR_MER,
    //&STATION_MORGAT,
    //&STATION_DOUARNENEZ,
    //&STATION_AUDIERNE,
    //&STATION_TREGUENNEC,
    //&STATION_LE_GUILVINEC,
    //&STATION_LOCTUDY,
    //&STATION_BENODET,
    //&STATION_CONCARNEAU,
    //&STATION_RIEC_SUR_BELON,
    //&STATION_PORT_TUDY_GROIX,
    //&STATION_PORT_LOUIS,
    //&STATION_LORIENT,
    //&STATION_ETEL,
    //&STATION_LE_LOGEO,
    //&STATION_PORTIVY,
    //&STATION_SAUZON,
    //&STATION_LOCMARIA,
    //&STATION_LES_DONNANTS,
    //&STATION_QUIBERON,
    //&STATION_CARNAC,
    //&STATION_LA_TRINITE_SUR_MER,
    //&STATION_AURAY_LE_BONO,
    //&STATION_PORT_NAVALO,
    //&STATION_LE_CROUESTY,
    //&STATION_ARRADON,
    //&STATION_LARMOR_BADEN,
    //&STATION_VANNES_PORT_ANNA_SENE,
    //&STATION_ST_GILDAS_DE_RHUYS,
    //&STATION_PORT_SAINT_JACQUES,
    //&STATION_LE_ROALIGUEN,
    //&STATION_PENERF,
    //&STATION_HOUAT,
    //&STATION_HOEDIC,
    //&STATION_PENESTIN,
    //&STATION_QUIMIAC,
    //&STATION_PIRIAC_SUR_MER,
    //&STATION_LA_TURBALLE,
    //&STATION_LE_CROISIC,
    //&STATION_LE_POULIGUEN,
    //&STATION_PORNICHET,
    //&STATION_SAINT_NAZAIRE,
    //&STATION_LE_PELLERIN,
    //&STATION_CORDEMAIS,
    //&STATION_NANTES_SALORGES,
    //&STATION_NANTES_USINE_BRULEE,
    //&STATION_PORT_LA_GRAVETTE,
    //&STATION_PAIMBOEUF,
    //&STATION_MONTOIR_DE_BRETAGNE,
    //&STATION_PORNIC,
    //&STATION_LHERBAUDIERE,
    //&STATION_PORT_JOINVILLE,
    //&STATION_SAINT_JEAN_DE_MONTS,
    //&STATION_SAINT_GILLES_CROIX_DE_VIE,
    //&STATION_LES_SABLES_DOLONNE,
    //&STATION_ARS_EN_RE,
    //&STATION_ST_MARTIN_EN_RE,
    //&STATION_PORT_DU_PLOMB,
    //&STATION_LA_ROCHELLE,
    //&STATION_SAINT_DENIS_DOLERON,
    //&STATION_LES_SABLES_VIGNIERS,
    //&STATION_LA_CAYENNE,
    //&STATION_LA_COTINIERE,
    //&STATION_ILE_DAIX,
    //&STATION_LA_COUBRE,
    //&STATION_LA_PALMYRE,
    //&STATION_ROYAN,
    //&STATION_ST_GEORGES_DE_DIDONNE,
    //&STATION_CORDOUAN,
    //&STATION_PORT_BLOC,
    //&STATION_SOULAC,
    //&STATION_LAMENA,
    //&STATION_BORDEAUX,
    //&STATION_BLAYE,
    //&STATION_MONTALIVET,
    //&STATION_CAP_FERRET,
    //&STATION_ARCACHON,
    //&STATION_BISCARROSSE,
    //&STATION_MIMIZAN,
    //&STATION_ST_JEAN_DE_LUZ,
    //&STATION_CAPBRETON_HOSSEGOR,
    //&STATION_BOUCAU_BAYONNE,
    //&STATION_BIARRITZ,
    //&STATION_SOCOA,
    //&STATION_BILBAO,
    //&STATION_PORT_VENDRES,
    //&STATION_PORT_LA_NOUVELLE,
    //&STATION_SETE,
    //&STATION_PORT_FERREOL,
    //&STATION_PORT_CAMARGUE,
    //&STATION_FOS_SUR_MER,
    //&STATION_LA_FIGUEIREITTE,
    //&STATION_TOULON,
    //&STATION_MARSEILLE,
    //&STATION_NICE,
    //&STATION_MONACO_FONVIEILLE,
    //&STATION_MONACO_PORT_HERCULE,
    //&STATION_AJACCIO_ASPRETTO,
    //&STATION_CENTURI,
    //&STATION_ILE_ROUSSE,
    //&STATION_SOLENZARA,
    //&STATION_MAJURO,
};
static const int REGISTRY_SIZE = (int)(sizeof(REGISTRY) / sizeof(REGISTRY[0]));

const StationDef* findStation(const char* id) {
    for (int i = 0; i < REGISTRY_SIZE; i++) {
        if (strcmp(REGISTRY[i]->id, id) == 0)
            return REGISTRY[i];
    }
    return nullptr;
}
