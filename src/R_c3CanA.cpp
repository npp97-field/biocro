/*
 *  BioCro/src/c3CanA.c by Fernando Ezequiel Miguez  Copyright (C) 2010
 *
 */

#include <R.h>
#include <Rinternals.h>
#include "BioCro.h"
#include "modules.h"

extern "C" {

    SEXP c3CanA(SEXP Lai,
            SEXP Doy,
            SEXP HR,
            SEXP SOLAR,
            SEXP TEMP,
            SEXP ReH,
            SEXP windspeed,
            SEXP LAT,
            SEXP NLAYERS, 
            SEXP PHOTOPS, 
            SEXP KD, 
            SEXP HEIGHTF, 
            SEXP LNPS,
            SEXP STOMATAWS,
            SEXP WS)
    {
        double LAI = REAL(Lai)[0];
        int DOY = INTEGER(Doy)[0];
        int hr = INTEGER(HR)[0];
        double solarR = REAL(SOLAR)[0];
        double Temp = REAL(TEMP)[0];
        double RH = REAL(ReH)[0];
        double WindSpeed = REAL(windspeed)[0];
        double lat = REAL(LAT)[0];
        int nlayers = INTEGER(NLAYERS)[0];
        double vmax = REAL(PHOTOPS)[0];
        double jmax = REAL(PHOTOPS)[1];
        double Rd = REAL(PHOTOPS)[2];
        double Catm = REAL(PHOTOPS)[3];
        double o2 = REAL(PHOTOPS)[4];
        double b0 = REAL(PHOTOPS)[5];
        double b1 = REAL(PHOTOPS)[6];
        double theta = REAL(PHOTOPS)[7];
        double kd = REAL(KD)[0];
        double heightf = REAL(HEIGHTF)[0];
        double leafN = REAL(LNPS)[0];
        double kpLN = REAL(LNPS)[1];
        double lnb0 = REAL(LNPS)[2];
        double lnb1 = REAL(LNPS)[3];
        int lnfun = REAL(LNPS)[4]; /* Coercing a double to integer */
        double StomataWS = REAL(STOMATAWS)[0];
        int ws = INTEGER(WS)[0];

        SEXP lists;
        SEXP names;
        SEXP growth;
        SEXP trans;
        SEXP Ggrowth;

        PROTECT(lists = allocVector(VECSXP,5));
        PROTECT(names = allocVector(STRSXP,5));
        PROTECT(growth = allocVector(REALSXP,1));
        PROTECT(trans = allocVector(REALSXP,1));
        PROTECT(Ggrowth = allocVector(REALSXP,1));

        state_map s;
        s["lai"] = LAI;
        s["doy"] = DOY;
        s["hour"] = hr;
        s["solarr"] = solarR;
        s["temp"] = Temp;
        s["rh"] = RH;
        s["windspeed"] = WindSpeed;
        s["lat"] = lat;
        s["nlayers"] = nlayers;
        s["vmax"] = vmax;
        s["jmax"] = jmax;
        s["rd"] = Rd;
        s["catm"] = Catm;
        s["o2"] = o2;
        s["b0"] = b0;
        s["b1"] = b1;
        s["theta"] = theta;
        s["kd"] = kd;
        s["heightf"] = heightf;
        s["leafn"] = leafN;
        s["kpln"] = kpLN;
        s["lnb0"] = lnb0;
        s["lnb1"] = lnb1;
        s["lnfun"] = lnfun;
        s["stomataws"] = StomataWS;
        s["ws"] = ws;

        c3_canopy canopy;
        vector<string> missing_state = canopy.state_requirements_are_met(s);
        if (!missing_state.empty()) {
            Rprintf("The following state variables are required for c3CanA but are missing: ");
            for(vector<string>::iterator it = missing_state.begin(); it != missing_state.end() - 1; ++it) {
                Rprintf("%s, ", it->c_str());
            }
            Rprintf("%s.\n", missing_state.back().c_str());
            error("This function cannot continue unless all state variables are set.");
        }

        state_map ans;
        ans = canopy.run(s);

        if(ISNAN(ans.at("Assim"))) {
            error("Something is NA \n");
        }

        REAL(growth)[0] = ans.at("Assim");
        REAL(trans)[0] = ans.at("Trans");
        REAL(Ggrowth)[0] = ans.at("GrossAssim");

        SET_VECTOR_ELT(lists, 0, growth);
        SET_VECTOR_ELT(lists, 1, trans);
        SET_VECTOR_ELT(lists, 2, Ggrowth);

        SET_STRING_ELT(names, 0, mkChar("CanopyAssim"));
        SET_STRING_ELT(names, 1, mkChar("CanopyTrans"));
        SET_STRING_ELT(names, 2, mkChar("GrossCanopyAssim"));
        setAttrib(lists,R_NamesSymbol,names);

        UNPROTECT(5);
        return(lists);
    }
}  // extern "C"

