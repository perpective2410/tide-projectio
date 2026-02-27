#pragma once

#include "StationDef.h"
#include <string.h>

// ── Station Registry (Header-Only) ──────────────────────────────────────────
// This header is included by Tides.h, so it's processed in the sketch's
// translation unit. This allows #define statements in the sketch to control
// which stations are compiled:
//
//   #define INCLUDE_LE_PALAIS
//   #define INCLUDE_SHEERNESS
//   #include <Tides.h>

// Always include Brest (default station)
#define INCLUDE_BREST

#ifdef INCLUDE_BREST
extern const StationDef STATION_BREST;
#endif
#ifdef INCLUDE_LE_PALAIS
extern const StationDef STATION_LE_PALAIS;
#endif
#ifdef INCLUDE_SHEERNESS
extern const StationDef STATION_SHEERNESS;
#endif
#ifdef INCLUDE_DOVER
extern const StationDef STATION_DOVER;
#endif
#ifdef INCLUDE_BRIGHTON
extern const StationDef STATION_BRIGHTON;
#endif
#ifdef INCLUDE_BOURNEMOUTH
extern const StationDef STATION_BOURNEMOUTH;
#endif
#ifdef INCLUDE_PENZANCE
extern const StationDef STATION_PENZANCE;
#endif
#ifdef INCLUDE_CARDIFF
extern const StationDef STATION_CARDIFF;
#endif
#ifdef INCLUDE_ISLES_OF_SCILLY
extern const StationDef STATION_ISLES_OF_SCILLY;
#endif
#ifdef INCLUDE_OOSTENDE
extern const StationDef STATION_OOSTENDE;
#endif
#ifdef INCLUDE_DUNKERQUE
extern const StationDef STATION_DUNKERQUE;
#endif
#ifdef INCLUDE_GRAND_FORT_PHILIPPE
extern const StationDef STATION_GRAND_FORT_PHILIPPE;
#endif
#ifdef INCLUDE_CALAIS
extern const StationDef STATION_CALAIS;
#endif
#ifdef INCLUDE_BOULOGNE_SUR_MER
extern const StationDef STATION_BOULOGNE_SUR_MER;
#endif
#ifdef INCLUDE_LE_TOUQUET
extern const StationDef STATION_LE_TOUQUET;
#endif
#ifdef INCLUDE_STELLA_PLAGE
extern const StationDef STATION_STELLA_PLAGE;
#endif
#ifdef INCLUDE_BERCK_SUR_MER
extern const StationDef STATION_BERCK_SUR_MER;
#endif
#ifdef INCLUDE_FORT_MAHON_PLAGE
extern const StationDef STATION_FORT_MAHON_PLAGE;
#endif
#ifdef INCLUDE_SAINT_VALERY_SUR_SOMME
extern const StationDef STATION_SAINT_VALERY_SUR_SOMME;
#endif
#ifdef INCLUDE_CAYEUX_SUR_MER
extern const StationDef STATION_CAYEUX_SUR_MER;
#endif
#ifdef INCLUDE_LE_TREPORT
extern const StationDef STATION_LE_TREPORT;
#endif
#ifdef INCLUDE_DIEPPE
extern const StationDef STATION_DIEPPE;
#endif
#ifdef INCLUDE_FECAMP
extern const StationDef STATION_FECAMP;
#endif
#ifdef INCLUDE_YPORT
extern const StationDef STATION_YPORT;
#endif
#ifdef INCLUDE_ETRETAT
extern const StationDef STATION_ETRETAT;
#endif
#ifdef INCLUDE_LE_HAVRE
extern const StationDef STATION_LE_HAVRE;
#endif
#ifdef INCLUDE_DEAUVILLE
extern const StationDef STATION_DEAUVILLE;
#endif
#ifdef INCLUDE_VILLERS_SUR_MER
extern const StationDef STATION_VILLERS_SUR_MER;
#endif
#ifdef INCLUDE_HOULGATE
extern const StationDef STATION_HOULGATE;
#endif
#ifdef INCLUDE_OUISTREHAM
extern const StationDef STATION_OUISTREHAM;
#endif
#ifdef INCLUDE_COURSEULLES_SUR_MER
extern const StationDef STATION_COURSEULLES_SUR_MER;
#endif
#ifdef INCLUDE_PORT_EN_BESSIN
extern const StationDef STATION_PORT_EN_BESSIN;
#endif
#ifdef INCLUDE_GRANDCAMP_MAISY
extern const StationDef STATION_GRANDCAMP_MAISY;
#endif
#ifdef INCLUDE_SAINT_VAAST_LA_HOUGUE
extern const StationDef STATION_SAINT_VAAST_LA_HOUGUE;
#endif
#ifdef INCLUDE_BARFLEUR
extern const StationDef STATION_BARFLEUR;
#endif
#ifdef INCLUDE_CHERBOURG
extern const StationDef STATION_CHERBOURG;
#endif
#ifdef INCLUDE_GOURY
extern const StationDef STATION_GOURY;
#endif
#ifdef INCLUDE_DIELETTE
extern const StationDef STATION_DIELETTE;
#endif
#ifdef INCLUDE_SAINT_PETER_PORT
extern const StationDef STATION_SAINT_PETER_PORT;
#endif
#ifdef INCLUDE_CARTERET
extern const StationDef STATION_CARTERET;
#endif
#ifdef INCLUDE_PORTBAIL
extern const StationDef STATION_PORTBAIL;
#endif
#ifdef INCLUDE_BLAINVILLE_SUR_MER
extern const StationDef STATION_BLAINVILLE_SUR_MER;
#endif
#ifdef INCLUDE_SAINT_HELIER
extern const StationDef STATION_SAINT_HELIER;
#endif
#ifdef INCLUDE_ILES_CHAUSEY
extern const StationDef STATION_ILES_CHAUSEY;
#endif
#ifdef INCLUDE_GRANVILLE
extern const StationDef STATION_GRANVILLE;
#endif
#ifdef INCLUDE_ST_PAIR_SUR_MER
extern const StationDef STATION_ST_PAIR_SUR_MER;
#endif
#ifdef INCLUDE_CANCALE
extern const StationDef STATION_CANCALE;
#endif
#ifdef INCLUDE_SAINT_MALO
extern const StationDef STATION_SAINT_MALO;
#endif
#ifdef INCLUDE_LANCIEUX
extern const StationDef STATION_LANCIEUX;
#endif
#ifdef INCLUDE_SAINT_CAST_LE_GUILDO
extern const StationDef STATION_SAINT_CAST_LE_GUILDO;
#endif
#ifdef INCLUDE_ERQUY
extern const StationDef STATION_ERQUY;
#endif
#ifdef INCLUDE_PLENEUF_VAL_ANDRE
extern const StationDef STATION_PLENEUF_VAL_ANDRE;
#endif
#ifdef INCLUDE_DAHOUET
extern const StationDef STATION_DAHOUET;
#endif
#ifdef INCLUDE_LE_LEGUE
extern const StationDef STATION_LE_LEGUE;
#endif
#ifdef INCLUDE_BINIC
extern const StationDef STATION_BINIC;
#endif
#ifdef INCLUDE_PAIMPOL
extern const StationDef STATION_PAIMPOL;
#endif
#ifdef INCLUDE_BREHAT
extern const StationDef STATION_BREHAT;
#endif
#ifdef INCLUDE_PORT_BENI
extern const StationDef STATION_PORT_BENI;
#endif
#ifdef INCLUDE_PERROS_GUIREC
extern const StationDef STATION_PERROS_GUIREC;
#endif
#ifdef INCLUDE_PLOUMANACH
extern const StationDef STATION_PLOUMANACH;
#endif
#ifdef INCLUDE_TREBEURDEN
extern const StationDef STATION_TREBEURDEN;
#endif
#ifdef INCLUDE_ST_MICHEL_EN_GREVE
extern const StationDef STATION_ST_MICHEL_EN_GREVE;
#endif
#ifdef INCLUDE_TERENEZ
extern const StationDef STATION_TERENEZ;
#endif
#ifdef INCLUDE_ROSCOFF
extern const StationDef STATION_ROSCOFF;
#endif
#ifdef INCLUDE_BRIGNOGAN_PLAGE
extern const StationDef STATION_BRIGNOGAN_PLAGE;
#endif
#ifdef INCLUDE_LABERWRACH
extern const StationDef STATION_LABERWRACH;
#endif
#ifdef INCLUDE_SAINT_PABU
extern const StationDef STATION_SAINT_PABU;
#endif
#ifdef INCLUDE_PORTSALL
extern const StationDef STATION_PORTSALL;
#endif
#ifdef INCLUDE_LANILDUT
extern const StationDef STATION_LANILDUT;
#endif
#ifdef INCLUDE_LE_CONQUET
extern const StationDef STATION_LE_CONQUET;
#endif
#ifdef INCLUDE_CAMARET_SUR_MER
extern const StationDef STATION_CAMARET_SUR_MER;
#endif
#ifdef INCLUDE_MORGAT
extern const StationDef STATION_MORGAT;
#endif
#ifdef INCLUDE_DOUARNENEZ
extern const StationDef STATION_DOUARNENEZ;
#endif
#ifdef INCLUDE_AUDIERNE
extern const StationDef STATION_AUDIERNE;
#endif
#ifdef INCLUDE_TREGUENNEC
extern const StationDef STATION_TREGUENNEC;
#endif
#ifdef INCLUDE_LE_GUILVINEC
extern const StationDef STATION_LE_GUILVINEC;
#endif
#ifdef INCLUDE_LOCTUDY
extern const StationDef STATION_LOCTUDY;
#endif
#ifdef INCLUDE_BENODET
extern const StationDef STATION_BENODET;
#endif
#ifdef INCLUDE_CONCARNEAU
extern const StationDef STATION_CONCARNEAU;
#endif
#ifdef INCLUDE_RIEC_SUR_BELON
extern const StationDef STATION_RIEC_SUR_BELON;
#endif
#ifdef INCLUDE_PORT_TUDY_GROIX
extern const StationDef STATION_PORT_TUDY_GROIX;
#endif
#ifdef INCLUDE_PORT_LOUIS
extern const StationDef STATION_PORT_LOUIS;
#endif
#ifdef INCLUDE_LORIENT
extern const StationDef STATION_LORIENT;
#endif
#ifdef INCLUDE_ETEL
extern const StationDef STATION_ETEL;
#endif
#ifdef INCLUDE_LE_LOGEO
extern const StationDef STATION_LE_LOGEO;
#endif
#ifdef INCLUDE_PORTIVY
extern const StationDef STATION_PORTIVY;
#endif
#ifdef INCLUDE_SAUZON
extern const StationDef STATION_SAUZON;
#endif
#ifdef INCLUDE_LOCMARIA
extern const StationDef STATION_LOCMARIA;
#endif
#ifdef INCLUDE_LES_DONNANTS
extern const StationDef STATION_LES_DONNANTS;
#endif
#ifdef INCLUDE_QUIBERON
extern const StationDef STATION_QUIBERON;
#endif
#ifdef INCLUDE_CARNAC
extern const StationDef STATION_CARNAC;
#endif
#ifdef INCLUDE_LA_TRINITE_SUR_MER
extern const StationDef STATION_LA_TRINITE_SUR_MER;
#endif
#ifdef INCLUDE_AURAY_LE_BONO
extern const StationDef STATION_AURAY_LE_BONO;
#endif
#ifdef INCLUDE_PORT_NAVALO
extern const StationDef STATION_PORT_NAVALO;
#endif
#ifdef INCLUDE_LE_CROUESTY
extern const StationDef STATION_LE_CROUESTY;
#endif
#ifdef INCLUDE_ARRADON
extern const StationDef STATION_ARRADON;
#endif
#ifdef INCLUDE_LARMOR_BADEN
extern const StationDef STATION_LARMOR_BADEN;
#endif
#ifdef INCLUDE_VANNES_PORT_ANNA_SENE
extern const StationDef STATION_VANNES_PORT_ANNA_SENE;
#endif
#ifdef INCLUDE_ST_GILDAS_DE_RHUYS
extern const StationDef STATION_ST_GILDAS_DE_RHUYS;
#endif
#ifdef INCLUDE_PORT_SAINT_JACQUES
extern const StationDef STATION_PORT_SAINT_JACQUES;
#endif
#ifdef INCLUDE_LE_ROALIGUEN
extern const StationDef STATION_LE_ROALIGUEN;
#endif
#ifdef INCLUDE_PENERF
extern const StationDef STATION_PENERF;
#endif
#ifdef INCLUDE_HOUAT
extern const StationDef STATION_HOUAT;
#endif
#ifdef INCLUDE_HOEDIC
extern const StationDef STATION_HOEDIC;
#endif
#ifdef INCLUDE_PENESTIN
extern const StationDef STATION_PENESTIN;
#endif
#ifdef INCLUDE_QUIMIAC
extern const StationDef STATION_QUIMIAC;
#endif
#ifdef INCLUDE_PIRIAC_SUR_MER
extern const StationDef STATION_PIRIAC_SUR_MER;
#endif
#ifdef INCLUDE_LA_TURBALLE
extern const StationDef STATION_LA_TURBALLE;
#endif
#ifdef INCLUDE_LE_CROISIC
extern const StationDef STATION_LE_CROISIC;
#endif
#ifdef INCLUDE_LE_POULIGUEN
extern const StationDef STATION_LE_POULIGUEN;
#endif
#ifdef INCLUDE_PORNICHET
extern const StationDef STATION_PORNICHET;
#endif
#ifdef INCLUDE_SAINT_NAZAIRE
extern const StationDef STATION_SAINT_NAZAIRE;
#endif
#ifdef INCLUDE_LE_PELLERIN
extern const StationDef STATION_LE_PELLERIN;
#endif
#ifdef INCLUDE_CORDEMAIS
extern const StationDef STATION_CORDEMAIS;
#endif
#ifdef INCLUDE_NANTES_SALORGES
extern const StationDef STATION_NANTES_SALORGES;
#endif
#ifdef INCLUDE_NANTES_USINE_BRULEE
extern const StationDef STATION_NANTES_USINE_BRULEE;
#endif
#ifdef INCLUDE_PORT_LA_GRAVETTE
extern const StationDef STATION_PORT_LA_GRAVETTE;
#endif
#ifdef INCLUDE_PAIMBOEUF
extern const StationDef STATION_PAIMBOEUF;
#endif
#ifdef INCLUDE_MONTOIR_DE_BRETAGNE
extern const StationDef STATION_MONTOIR_DE_BRETAGNE;
#endif
#ifdef INCLUDE_PORNIC
extern const StationDef STATION_PORNIC;
#endif
#ifdef INCLUDE_LHERBAUDIERE
extern const StationDef STATION_LHERBAUDIERE;
#endif
#ifdef INCLUDE_PORT_JOINVILLE
extern const StationDef STATION_PORT_JOINVILLE;
#endif
#ifdef INCLUDE_SAINT_JEAN_DE_MONTS
extern const StationDef STATION_SAINT_JEAN_DE_MONTS;
#endif
#ifdef INCLUDE_SAINT_GILLES_CROIX_DE_VIE
extern const StationDef STATION_SAINT_GILLES_CROIX_DE_VIE;
#endif
#ifdef INCLUDE_LES_SABLES_DOLONNE
extern const StationDef STATION_LES_SABLES_DOLONNE;
#endif
#ifdef INCLUDE_ARS_EN_RE
extern const StationDef STATION_ARS_EN_RE;
#endif
#ifdef INCLUDE_ST_MARTIN_EN_RE
extern const StationDef STATION_ST_MARTIN_EN_RE;
#endif
#ifdef INCLUDE_PORT_DU_PLOMB
extern const StationDef STATION_PORT_DU_PLOMB;
#endif
#ifdef INCLUDE_LA_ROCHELLE
extern const StationDef STATION_LA_ROCHELLE;
#endif
#ifdef INCLUDE_SAINT_DENIS_DOLERON
extern const StationDef STATION_SAINT_DENIS_DOLERON;
#endif
#ifdef INCLUDE_LES_SABLES_VIGNIERS
extern const StationDef STATION_LES_SABLES_VIGNIERS;
#endif
#ifdef INCLUDE_LA_CAYENNE
extern const StationDef STATION_LA_CAYENNE;
#endif
#ifdef INCLUDE_LA_COTINIERE
extern const StationDef STATION_LA_COTINIERE;
#endif
#ifdef INCLUDE_ILE_DAIX
extern const StationDef STATION_ILE_DAIX;
#endif
#ifdef INCLUDE_LA_COUBRE
extern const StationDef STATION_LA_COUBRE;
#endif
#ifdef INCLUDE_LA_PALMYRE
extern const StationDef STATION_LA_PALMYRE;
#endif
#ifdef INCLUDE_ROYAN
extern const StationDef STATION_ROYAN;
#endif
#ifdef INCLUDE_ST_GEORGES_DE_DIDONNE
extern const StationDef STATION_ST_GEORGES_DE_DIDONNE;
#endif
#ifdef INCLUDE_CORDOUAN
extern const StationDef STATION_CORDOUAN;
#endif
#ifdef INCLUDE_PORT_BLOC
extern const StationDef STATION_PORT_BLOC;
#endif
#ifdef INCLUDE_SOULAC
extern const StationDef STATION_SOULAC;
#endif
#ifdef INCLUDE_LAMENA
extern const StationDef STATION_LAMENA;
#endif
#ifdef INCLUDE_BORDEAUX
extern const StationDef STATION_BORDEAUX;
#endif
#ifdef INCLUDE_BLAYE
extern const StationDef STATION_BLAYE;
#endif
#ifdef INCLUDE_MONTALIVET
extern const StationDef STATION_MONTALIVET;
#endif
#ifdef INCLUDE_CAP_FERRET
extern const StationDef STATION_CAP_FERRET;
#endif
#ifdef INCLUDE_ARCACHON
extern const StationDef STATION_ARCACHON;
#endif
#ifdef INCLUDE_BISCARROSSE
extern const StationDef STATION_BISCARROSSE;
#endif
#ifdef INCLUDE_MIMIZAN
extern const StationDef STATION_MIMIZAN;
#endif
#ifdef INCLUDE_ST_JEAN_DE_LUZ
extern const StationDef STATION_ST_JEAN_DE_LUZ;
#endif
#ifdef INCLUDE_CAPBRETON_HOSSEGOR
extern const StationDef STATION_CAPBRETON_HOSSEGOR;
#endif
#ifdef INCLUDE_BOUCAU_BAYONNE
extern const StationDef STATION_BOUCAU_BAYONNE;
#endif
#ifdef INCLUDE_BIARRITZ
extern const StationDef STATION_BIARRITZ;
#endif
#ifdef INCLUDE_SOCOA
extern const StationDef STATION_SOCOA;
#endif
#ifdef INCLUDE_BILBAO
extern const StationDef STATION_BILBAO;
#endif
#ifdef INCLUDE_PORT_VENDRES
extern const StationDef STATION_PORT_VENDRES;
#endif
#ifdef INCLUDE_PORT_LA_NOUVELLE
extern const StationDef STATION_PORT_LA_NOUVELLE;
#endif
#ifdef INCLUDE_SETE
extern const StationDef STATION_SETE;
#endif
#ifdef INCLUDE_PORT_FERREOL
extern const StationDef STATION_PORT_FERREOL;
#endif
#ifdef INCLUDE_PORT_CAMARGUE
extern const StationDef STATION_PORT_CAMARGUE;
#endif
#ifdef INCLUDE_FOS_SUR_MER
extern const StationDef STATION_FOS_SUR_MER;
#endif
#ifdef INCLUDE_LA_FIGUEIREITTE
extern const StationDef STATION_LA_FIGUEIREITTE;
#endif
#ifdef INCLUDE_TOULON
extern const StationDef STATION_TOULON;
#endif
#ifdef INCLUDE_MARSEILLE
extern const StationDef STATION_MARSEILLE;
#endif
#ifdef INCLUDE_NICE
extern const StationDef STATION_NICE;
#endif
#ifdef INCLUDE_MONACO_FONVIEILLE
extern const StationDef STATION_MONACO_FONVIEILLE;
#endif
#ifdef INCLUDE_MONACO_PORT_HERCULE
extern const StationDef STATION_MONACO_PORT_HERCULE;
#endif
#ifdef INCLUDE_AJACCIO_ASPRETTO
extern const StationDef STATION_AJACCIO_ASPRETTO;
#endif
#ifdef INCLUDE_CENTURI
extern const StationDef STATION_CENTURI;
#endif
#ifdef INCLUDE_ILE_ROUSSE
extern const StationDef STATION_ILE_ROUSSE;
#endif
#ifdef INCLUDE_SOLENZARA
extern const StationDef STATION_SOLENZARA;
#endif
#ifdef INCLUDE_MAJURO
extern const StationDef STATION_MAJURO;
#endif

static const StationDef* const REGISTRY[] = {
#ifdef INCLUDE_LE_PALAIS
    &STATION_LE_PALAIS,
#endif
#ifdef INCLUDE_BREST
    &STATION_BREST,
#endif
#ifdef INCLUDE_SHEERNESS
    &STATION_SHEERNESS,
#endif
#ifdef INCLUDE_DOVER
    &STATION_DOVER,
#endif
#ifdef INCLUDE_BRIGHTON
    &STATION_BRIGHTON,
#endif
#ifdef INCLUDE_BOURNEMOUTH
    &STATION_BOURNEMOUTH,
#endif
#ifdef INCLUDE_PENZANCE
    &STATION_PENZANCE,
#endif
#ifdef INCLUDE_CARDIFF
    &STATION_CARDIFF,
#endif
#ifdef INCLUDE_ISLES_OF_SCILLY
    &STATION_ISLES_OF_SCILLY,
#endif
#ifdef INCLUDE_OOSTENDE
    &STATION_OOSTENDE,
#endif
#ifdef INCLUDE_DUNKERQUE
    &STATION_DUNKERQUE,
#endif
#ifdef INCLUDE_GRAND_FORT_PHILIPPE
    &STATION_GRAND_FORT_PHILIPPE,
#endif
#ifdef INCLUDE_CALAIS
    &STATION_CALAIS,
#endif
#ifdef INCLUDE_BOULOGNE_SUR_MER
    &STATION_BOULOGNE_SUR_MER,
#endif
#ifdef INCLUDE_LE_TOUQUET
    &STATION_LE_TOUQUET,
#endif
#ifdef INCLUDE_STELLA_PLAGE
    &STATION_STELLA_PLAGE,
#endif
#ifdef INCLUDE_BERCK_SUR_MER
    &STATION_BERCK_SUR_MER,
#endif
#ifdef INCLUDE_FORT_MAHON_PLAGE
    &STATION_FORT_MAHON_PLAGE,
#endif
#ifdef INCLUDE_SAINT_VALERY_SUR_SOMME
    &STATION_SAINT_VALERY_SUR_SOMME,
#endif
#ifdef INCLUDE_CAYEUX_SUR_MER
    &STATION_CAYEUX_SUR_MER,
#endif
#ifdef INCLUDE_LE_TREPORT
    &STATION_LE_TREPORT,
#endif
#ifdef INCLUDE_DIEPPE
    &STATION_DIEPPE,
#endif
#ifdef INCLUDE_FECAMP
    &STATION_FECAMP,
#endif
#ifdef INCLUDE_YPORT
    &STATION_YPORT,
#endif
#ifdef INCLUDE_ETRETAT
    &STATION_ETRETAT,
#endif
#ifdef INCLUDE_LE_HAVRE
    &STATION_LE_HAVRE,
#endif
#ifdef INCLUDE_DEAUVILLE
    &STATION_DEAUVILLE,
#endif
#ifdef INCLUDE_VILLERS_SUR_MER
    &STATION_VILLERS_SUR_MER,
#endif
#ifdef INCLUDE_HOULGATE
    &STATION_HOULGATE,
#endif
#ifdef INCLUDE_OUISTREHAM
    &STATION_OUISTREHAM,
#endif
#ifdef INCLUDE_COURSEULLES_SUR_MER
    &STATION_COURSEULLES_SUR_MER,
#endif
#ifdef INCLUDE_PORT_EN_BESSIN
    &STATION_PORT_EN_BESSIN,
#endif
#ifdef INCLUDE_GRANDCAMP_MAISY
    &STATION_GRANDCAMP_MAISY,
#endif
#ifdef INCLUDE_SAINT_VAAST_LA_HOUGUE
    &STATION_SAINT_VAAST_LA_HOUGUE,
#endif
#ifdef INCLUDE_BARFLEUR
    &STATION_BARFLEUR,
#endif
#ifdef INCLUDE_CHERBOURG
    &STATION_CHERBOURG,
#endif
#ifdef INCLUDE_GOURY
    &STATION_GOURY,
#endif
#ifdef INCLUDE_DIELETTE
    &STATION_DIELETTE,
#endif
#ifdef INCLUDE_SAINT_PETER_PORT
    &STATION_SAINT_PETER_PORT,
#endif
#ifdef INCLUDE_CARTERET
    &STATION_CARTERET,
#endif
#ifdef INCLUDE_PORTBAIL
    &STATION_PORTBAIL,
#endif
#ifdef INCLUDE_BLAINVILLE_SUR_MER
    &STATION_BLAINVILLE_SUR_MER,
#endif
#ifdef INCLUDE_SAINT_HELIER
    &STATION_SAINT_HELIER,
#endif
#ifdef INCLUDE_ILES_CHAUSEY
    &STATION_ILES_CHAUSEY,
#endif
#ifdef INCLUDE_GRANVILLE
    &STATION_GRANVILLE,
#endif
#ifdef INCLUDE_ST_PAIR_SUR_MER
    &STATION_ST_PAIR_SUR_MER,
#endif
#ifdef INCLUDE_CANCALE
    &STATION_CANCALE,
#endif
#ifdef INCLUDE_SAINT_MALO
    &STATION_SAINT_MALO,
#endif
#ifdef INCLUDE_LANCIEUX
    &STATION_LANCIEUX,
#endif
#ifdef INCLUDE_SAINT_CAST_LE_GUILDO
    &STATION_SAINT_CAST_LE_GUILDO,
#endif
#ifdef INCLUDE_ERQUY
    &STATION_ERQUY,
#endif
#ifdef INCLUDE_PLENEUF_VAL_ANDRE
    &STATION_PLENEUF_VAL_ANDRE,
#endif
#ifdef INCLUDE_DAHOUET
    &STATION_DAHOUET,
#endif
#ifdef INCLUDE_LE_LEGUE
    &STATION_LE_LEGUE,
#endif
#ifdef INCLUDE_BINIC
    &STATION_BINIC,
#endif
#ifdef INCLUDE_PAIMPOL
    &STATION_PAIMPOL,
#endif
#ifdef INCLUDE_BREHAT
    &STATION_BREHAT,
#endif
#ifdef INCLUDE_PORT_BENI
    &STATION_PORT_BENI,
#endif
#ifdef INCLUDE_PERROS_GUIREC
    &STATION_PERROS_GUIREC,
#endif
#ifdef INCLUDE_PLOUMANACH
    &STATION_PLOUMANACH,
#endif
#ifdef INCLUDE_TREBEURDEN
    &STATION_TREBEURDEN,
#endif
#ifdef INCLUDE_ST_MICHEL_EN_GREVE
    &STATION_ST_MICHEL_EN_GREVE,
#endif
#ifdef INCLUDE_TERENEZ
    &STATION_TERENEZ,
#endif
#ifdef INCLUDE_ROSCOFF
    &STATION_ROSCOFF,
#endif
#ifdef INCLUDE_BRIGNOGAN_PLAGE
    &STATION_BRIGNOGAN_PLAGE,
#endif
#ifdef INCLUDE_LABERWRACH
    &STATION_LABERWRACH,
#endif
#ifdef INCLUDE_SAINT_PABU
    &STATION_SAINT_PABU,
#endif
#ifdef INCLUDE_PORTSALL
    &STATION_PORTSALL,
#endif
#ifdef INCLUDE_LANILDUT
    &STATION_LANILDUT,
#endif
#ifdef INCLUDE_LE_CONQUET
    &STATION_LE_CONQUET,
#endif
#ifdef INCLUDE_CAMARET_SUR_MER
    &STATION_CAMARET_SUR_MER,
#endif
#ifdef INCLUDE_MORGAT
    &STATION_MORGAT,
#endif
#ifdef INCLUDE_DOUARNENEZ
    &STATION_DOUARNENEZ,
#endif
#ifdef INCLUDE_AUDIERNE
    &STATION_AUDIERNE,
#endif
#ifdef INCLUDE_TREGUENNEC
    &STATION_TREGUENNEC,
#endif
#ifdef INCLUDE_LE_GUILVINEC
    &STATION_LE_GUILVINEC,
#endif
#ifdef INCLUDE_LOCTUDY
    &STATION_LOCTUDY,
#endif
#ifdef INCLUDE_BENODET
    &STATION_BENODET,
#endif
#ifdef INCLUDE_CONCARNEAU
    &STATION_CONCARNEAU,
#endif
#ifdef INCLUDE_RIEC_SUR_BELON
    &STATION_RIEC_SUR_BELON,
#endif
#ifdef INCLUDE_PORT_TUDY_GROIX
    &STATION_PORT_TUDY_GROIX,
#endif
#ifdef INCLUDE_PORT_LOUIS
    &STATION_PORT_LOUIS,
#endif
#ifdef INCLUDE_LORIENT
    &STATION_LORIENT,
#endif
#ifdef INCLUDE_ETEL
    &STATION_ETEL,
#endif
#ifdef INCLUDE_LE_LOGEO
    &STATION_LE_LOGEO,
#endif
#ifdef INCLUDE_PORTIVY
    &STATION_PORTIVY,
#endif
#ifdef INCLUDE_SAUZON
    &STATION_SAUZON,
#endif
#ifdef INCLUDE_LOCMARIA
    &STATION_LOCMARIA,
#endif
#ifdef INCLUDE_LES_DONNANTS
    &STATION_LES_DONNANTS,
#endif
#ifdef INCLUDE_QUIBERON
    &STATION_QUIBERON,
#endif
#ifdef INCLUDE_CARNAC
    &STATION_CARNAC,
#endif
#ifdef INCLUDE_LA_TRINITE_SUR_MER
    &STATION_LA_TRINITE_SUR_MER,
#endif
#ifdef INCLUDE_AURAY_LE_BONO
    &STATION_AURAY_LE_BONO,
#endif
#ifdef INCLUDE_PORT_NAVALO
    &STATION_PORT_NAVALO,
#endif
#ifdef INCLUDE_LE_CROUESTY
    &STATION_LE_CROUESTY,
#endif
#ifdef INCLUDE_ARRADON
    &STATION_ARRADON,
#endif
#ifdef INCLUDE_LARMOR_BADEN
    &STATION_LARMOR_BADEN,
#endif
#ifdef INCLUDE_VANNES_PORT_ANNA_SENE
    &STATION_VANNES_PORT_ANNA_SENE,
#endif
#ifdef INCLUDE_ST_GILDAS_DE_RHUYS
    &STATION_ST_GILDAS_DE_RHUYS,
#endif
#ifdef INCLUDE_PORT_SAINT_JACQUES
    &STATION_PORT_SAINT_JACQUES,
#endif
#ifdef INCLUDE_LE_ROALIGUEN
    &STATION_LE_ROALIGUEN,
#endif
#ifdef INCLUDE_PENERF
    &STATION_PENERF,
#endif
#ifdef INCLUDE_HOUAT
    &STATION_HOUAT,
#endif
#ifdef INCLUDE_HOEDIC
    &STATION_HOEDIC,
#endif
#ifdef INCLUDE_PENESTIN
    &STATION_PENESTIN,
#endif
#ifdef INCLUDE_QUIMIAC
    &STATION_QUIMIAC,
#endif
#ifdef INCLUDE_PIRIAC_SUR_MER
    &STATION_PIRIAC_SUR_MER,
#endif
#ifdef INCLUDE_LA_TURBALLE
    &STATION_LA_TURBALLE,
#endif
#ifdef INCLUDE_LE_CROISIC
    &STATION_LE_CROISIC,
#endif
#ifdef INCLUDE_LE_POULIGUEN
    &STATION_LE_POULIGUEN,
#endif
#ifdef INCLUDE_PORNICHET
    &STATION_PORNICHET,
#endif
#ifdef INCLUDE_SAINT_NAZAIRE
    &STATION_SAINT_NAZAIRE,
#endif
#ifdef INCLUDE_LE_PELLERIN
    &STATION_LE_PELLERIN,
#endif
#ifdef INCLUDE_CORDEMAIS
    &STATION_CORDEMAIS,
#endif
#ifdef INCLUDE_NANTES_SALORGES
    &STATION_NANTES_SALORGES,
#endif
#ifdef INCLUDE_NANTES_USINE_BRULEE
    &STATION_NANTES_USINE_BRULEE,
#endif
#ifdef INCLUDE_PORT_LA_GRAVETTE
    &STATION_PORT_LA_GRAVETTE,
#endif
#ifdef INCLUDE_PAIMBOEUF
    &STATION_PAIMBOEUF,
#endif
#ifdef INCLUDE_MONTOIR_DE_BRETAGNE
    &STATION_MONTOIR_DE_BRETAGNE,
#endif
#ifdef INCLUDE_PORNIC
    &STATION_PORNIC,
#endif
#ifdef INCLUDE_LHERBAUDIERE
    &STATION_LHERBAUDIERE,
#endif
#ifdef INCLUDE_PORT_JOINVILLE
    &STATION_PORT_JOINVILLE,
#endif
#ifdef INCLUDE_SAINT_JEAN_DE_MONTS
    &STATION_SAINT_JEAN_DE_MONTS,
#endif
#ifdef INCLUDE_SAINT_GILLES_CROIX_DE_VIE
    &STATION_SAINT_GILLES_CROIX_DE_VIE,
#endif
#ifdef INCLUDE_LES_SABLES_DOLONNE
    &STATION_LES_SABLES_DOLONNE,
#endif
#ifdef INCLUDE_ARS_EN_RE
    &STATION_ARS_EN_RE,
#endif
#ifdef INCLUDE_ST_MARTIN_EN_RE
    &STATION_ST_MARTIN_EN_RE,
#endif
#ifdef INCLUDE_PORT_DU_PLOMB
    &STATION_PORT_DU_PLOMB,
#endif
#ifdef INCLUDE_LA_ROCHELLE
    &STATION_LA_ROCHELLE,
#endif
#ifdef INCLUDE_SAINT_DENIS_DOLERON
    &STATION_SAINT_DENIS_DOLERON,
#endif
#ifdef INCLUDE_LES_SABLES_VIGNIERS
    &STATION_LES_SABLES_VIGNIERS,
#endif
#ifdef INCLUDE_LA_CAYENNE
    &STATION_LA_CAYENNE,
#endif
#ifdef INCLUDE_LA_COTINIERE
    &STATION_LA_COTINIERE,
#endif
#ifdef INCLUDE_ILE_DAIX
    &STATION_ILE_DAIX,
#endif
#ifdef INCLUDE_LA_COUBRE
    &STATION_LA_COUBRE,
#endif
#ifdef INCLUDE_LA_PALMYRE
    &STATION_LA_PALMYRE,
#endif
#ifdef INCLUDE_ROYAN
    &STATION_ROYAN,
#endif
#ifdef INCLUDE_ST_GEORGES_DE_DIDONNE
    &STATION_ST_GEORGES_DE_DIDONNE,
#endif
#ifdef INCLUDE_CORDOUAN
    &STATION_CORDOUAN,
#endif
#ifdef INCLUDE_PORT_BLOC
    &STATION_PORT_BLOC,
#endif
#ifdef INCLUDE_SOULAC
    &STATION_SOULAC,
#endif
#ifdef INCLUDE_LAMENA
    &STATION_LAMENA,
#endif
#ifdef INCLUDE_BORDEAUX
    &STATION_BORDEAUX,
#endif
#ifdef INCLUDE_BLAYE
    &STATION_BLAYE,
#endif
#ifdef INCLUDE_MONTALIVET
    &STATION_MONTALIVET,
#endif
#ifdef INCLUDE_CAP_FERRET
    &STATION_CAP_FERRET,
#endif
#ifdef INCLUDE_ARCACHON
    &STATION_ARCACHON,
#endif
#ifdef INCLUDE_BISCARROSSE
    &STATION_BISCARROSSE,
#endif
#ifdef INCLUDE_MIMIZAN
    &STATION_MIMIZAN,
#endif
#ifdef INCLUDE_ST_JEAN_DE_LUZ
    &STATION_ST_JEAN_DE_LUZ,
#endif
#ifdef INCLUDE_CAPBRETON_HOSSEGOR
    &STATION_CAPBRETON_HOSSEGOR,
#endif
#ifdef INCLUDE_BOUCAU_BAYONNE
    &STATION_BOUCAU_BAYONNE,
#endif
#ifdef INCLUDE_BIARRITZ
    &STATION_BIARRITZ,
#endif
#ifdef INCLUDE_SOCOA
    &STATION_SOCOA,
#endif
#ifdef INCLUDE_BILBAO
    &STATION_BILBAO,
#endif
#ifdef INCLUDE_PORT_VENDRES
    &STATION_PORT_VENDRES,
#endif
#ifdef INCLUDE_PORT_LA_NOUVELLE
    &STATION_PORT_LA_NOUVELLE,
#endif
#ifdef INCLUDE_SETE
    &STATION_SETE,
#endif
#ifdef INCLUDE_PORT_FERREOL
    &STATION_PORT_FERREOL,
#endif
#ifdef INCLUDE_PORT_CAMARGUE
    &STATION_PORT_CAMARGUE,
#endif
#ifdef INCLUDE_FOS_SUR_MER
    &STATION_FOS_SUR_MER,
#endif
#ifdef INCLUDE_LA_FIGUEIREITTE
    &STATION_LA_FIGUEIREITTE,
#endif
#ifdef INCLUDE_TOULON
    &STATION_TOULON,
#endif
#ifdef INCLUDE_MARSEILLE
    &STATION_MARSEILLE,
#endif
#ifdef INCLUDE_NICE
    &STATION_NICE,
#endif
#ifdef INCLUDE_MONACO_FONVIEILLE
    &STATION_MONACO_FONVIEILLE,
#endif
#ifdef INCLUDE_MONACO_PORT_HERCULE
    &STATION_MONACO_PORT_HERCULE,
#endif
#ifdef INCLUDE_AJACCIO_ASPRETTO
    &STATION_AJACCIO_ASPRETTO,
#endif
#ifdef INCLUDE_CENTURI
    &STATION_CENTURI,
#endif
#ifdef INCLUDE_ILE_ROUSSE
    &STATION_ILE_ROUSSE,
#endif
#ifdef INCLUDE_SOLENZARA
    &STATION_SOLENZARA,
#endif
#ifdef INCLUDE_MAJURO
    &STATION_MAJURO,
#endif
};

static const StationDef* findStation(const char* id) {
    static const int size = (int)(sizeof(REGISTRY) / sizeof(REGISTRY[0]));
    for (int i = 0; i < size; i++) {
        if (strcmp(REGISTRY[i]->id, id) == 0)
            return REGISTRY[i];
    }
    return nullptr;
}
