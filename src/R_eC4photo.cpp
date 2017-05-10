
#include <R.h>
#include <Rinternals.h>
#include <Rmath.h>
#include "BioCro.h"
#include "CanA.h"
#include "eC4photo.h"

extern "C" {

SEXP eCanA(SEXP lai, SEXP Doy, SEXP Hr, SEXP SolarR, SEXP ATemp,
	  SEXP RelH, SEXP WindS, SEXP CA, SEXP OA,
	   SEXP VCMAX, SEXP VPMAX,
	  SEXP VPR, SEXP JMAX, SEXP STOMATAWS)
{
	/* const int NLAYERS = 3;  this should eventually be replaced with
	   an argument coming in from R */
	struct ET_Str direct_et, diffuse_et;

	int i;
	double Idir, Idiff, cosTh;
	double LAIc;
	double IDir, IDiff, Itot, rh, wind_speed;
	double pLeafsun, pLeafshade;
	double Leafsun, Leafshade;

	double TempIdir,TempIdiff,AssIdir,AssIdiff;

	double CanopyA;
	double CanHeight;

	const double cf = 3600 * 1e-6 ;

	int DOY , hr;
	double LAI, solarR, Temp, RH, WindSpeed;
	double Ca, Oa, Vcmax, Vpmax, Vpr, Jmax;
	double upperT=27.5, lowerT=3.0;

	double lat = 40;
	int nlayers = 3;
	double kd = 0.1;
	double chil = 1;
	double stomataws;

	SEXP growth;

	PROTECT(growth = allocVector(REALSXP,1));


	LAI = REAL(lai)[0];
	DOY = INTEGER(Doy)[0];
	hr = INTEGER(Hr)[0];
	solarR = REAL(SolarR)[0];
	Temp = REAL(ATemp)[0];
	RH = REAL(RelH)[0];
	WindSpeed = REAL(WindS)[0];
	Ca = REAL(CA)[0];
	Oa = REAL(OA)[0];
	Vcmax = REAL(VCMAX)[0];
	Vpmax = REAL(VPMAX)[0];
	Vpr = REAL(VPR)[0];
	Jmax = REAL(JMAX)[0];
	stomataws = REAL(STOMATAWS)[0];

	struct Light_model light_model;
	light_model = lightME(lat, DOY, hr);

	Idir = light_model.direct_irradiance_fraction * solarR;
	Idiff = light_model.diffuse_irradiance_fraction * solarR;
	cosTh = light_model.cosine_zenith_angle;

	struct Light_profile light_profile;
	light_profile = sunML(Idir, Idiff, LAI, nlayers, cosTh, kd, chil, 3);

	/* results from multilayer model */
	LAIc = LAI / nlayers;
	double relative_humidity_profile[nlayers];
	RHprof(RH, nlayers, relative_humidity_profile);

	double wind_speed_profile[nlayers];
	WINDprof(WindSpeed, LAI, nlayers, wind_speed_profile);

	/* Next use the EvapoTrans function */
	CanopyA=0.0;
	for(i=0;i<nlayers;i++)
	{
		int current_layer = nlayers - 1 - i;
		rh = relative_humidity_profile[current_layer];
		wind_speed = wind_speed_profile[current_layer];

		IDir = light_profile.direct_irradiance[current_layer];
		Itot = light_profile.total_irradiance[current_layer];
		pLeafsun = light_profile.sunlit_fraction[current_layer];
		CanHeight = light_profile.height[current_layer];
		Leafsun = LAIc * pLeafsun;
		direct_et = EvapoTrans(IDir,Itot,Temp,rh,wind_speed,Leafsun,CanHeight,stomataws,1,39,0.04,0.7,0.83,0.93,0.8,0.01,3,upperT,lowerT, Ca);
		/* not the right thing to do here to add these values at the end of the ET function
		   but just a simple fix for now. The problem is that the eC4photoC function should have its own
		   EvapoTrans function. */
		TempIdir = Temp + direct_et.Deltat;
		AssIdir = eC4photoC(IDir,TempIdir,rh,Ca,Oa,Vcmax,
				Vpmax,Vpr,Jmax);

		IDiff = light_profile.diffuse_irradiance[current_layer];
		pLeafshade = light_profile.shaded_fraction[current_layer];
		Leafshade = LAIc * pLeafshade;
		diffuse_et = EvapoTrans(IDiff,Itot,Temp,rh,wind_speed,Leafshade,CanHeight,stomataws,1,39,0.04,0.7,0.83,0.93,0.8,0.01,3,upperT,lowerT, Ca);
		/* not the right thing to do here to add these values at the end of the ET function
		   but just a simple fix for now*/
		TempIdiff = Temp + diffuse_et.Deltat;
		AssIdiff = eC4photoC(IDiff,TempIdiff,rh,Ca,Oa,Vcmax,
				Vpmax,Vpr,Jmax);

		CanopyA += Leafsun * AssIdir + Leafshade * AssIdiff;
	}
	/* These are micro mols of CO2 per m2 per sec
	   Need to convert to 
	   3600 converts seconds to hours
	   10^-6 converts micro mols to mols
	   */
	REAL(growth)[0] = cf * CanopyA ;
	UNPROTECT(1);  
	return(growth);

}

SEXP eC4photo(SEXP QP, SEXP TEMP, SEXP RH, SEXP CA,
		SEXP OA, SEXP VCMAX, SEXP VPMAX, SEXP VPR,
		SEXP JMAX)
{
  int nqp, i;
  nqp = length(QP);
    /* CONSTANTS */
  double P = 101.3;
  double G1 = 3;
  double G0 = 0.08;

    const double  gs = 3 * 1e-3 ;
    /* mol m-2 s-1 physical conductance to CO2 leakage*/
    /* const double gammaStar = 0.000193  */
    const double gammaStar =  0.000193 ;
    /* half the reciprocal of Rubisco specificity */
    /*    const double go = 0.047 * gs     at 25 C */
    const double alpha = 0.01 ; /* alpha in the notes*/
    double Kp = 80 ; /*  mu bar */
    double theta = 0.7;
    const double R = 0.008314472; /* Gas constant J K-1 mol-1 */
    /* in the units stated should be 8.314 but 
       XGZ determined it is * 1e-3 */ 
    const double Kelvin0 = 273.15;
    const double Kelvin25 = 298.15;
    /* This is 0 and 25 Celsius degrees in the Kelvin scale */

    /* ADDING THE TEMPERATURE RESPONSE FUNCTION */
    // double Epr = 46.8 ; /* Activation energy of PPDK kj/mol*/ unused
    // double ERd = 46.39; unused
    double Ep  = 47.1 ; /* Activation energy of PEPc kj/mol */
    double Erb = 72 ; /* Activation energy of Rubisco kj/mol */
    /*    const double Q10bf = 1.7 */
    double EKc = 79.43;
    double EKo = 36.38;
    double Q10cb = 1.7;

    /* double Ko = 450 ;  mbar at 25 C */
    double Ko2 = 450 ; /* mbar at 25 C */
    /* double Kc = 650   mu bar at 25 C */
    double Kc2 = 650 ; /*  mu bar at 25 C */
    
    /* VAriables */ 
    double Kc, Ko;
    double StomCond;
    double Idir, AirTemp, rh, Ca, Oa;
    double Vcmax1, Vpmax1, Jmax1;
    double Vcmax, Vpmax, Vpr, Jmax;

    // double Q10pr, Q10Rd; unused
    double Q10p, Q10rb, Q10Kc, Q10Ko;
    double Cm, Om;
    double Rd, Rm;
    // double Rs; unused

    double I2, J, Aj;
    double Vp, Ko1 , Om1;
    double a1, b1, c1 , c3, Ac0;
    double AcLCO2, Ac, A;
    double Os, Cs;
    double Cs0, Cs1;

    double aa, bb, cc;

    SEXP names, lists;
    SEXP Assim;
    SEXP Gs;
    SEXP Ci;
    SEXP Oxy;

    PROTECT(lists = allocVector(VECSXP,4));
    PROTECT(names = allocVector(STRSXP,4));

    PROTECT(Assim = allocVector(REALSXP,nqp));
    PROTECT(Gs = allocVector(REALSXP,nqp));
    PROTECT(Ci = allocVector(REALSXP,nqp));
    PROTECT(Oxy = allocVector(REALSXP,nqp));

    Ca = REAL(CA)[0];
    Oa = REAL(OA)[0];
    Vcmax1 = REAL(VCMAX)[0];
    Vpmax1 = REAL(VPMAX)[0];
    Vpr = REAL(VPR)[0];
    Jmax1 = REAL(JMAX)[0];

    for (i = 0; i < nqp; i++) {

        Idir = REAL(QP)[i];
        AirTemp = REAL(TEMP)[i];
        rh = REAL(RH)[i];


        // Q10pr = exp(Epr *(1/(R * Kelvin25)-1/(R * (AirTemp + Kelvin0)))); unused
        // Q10Rd = exp(ERd *(1/(R * Kelvin25)-1/(R * (AirTemp + Kelvin0)))); unused
        Q10p = exp(Ep *(1/(R * Kelvin25)-1/(R * (AirTemp + Kelvin0))));
        Q10rb = exp(Erb *(1/(R * Kelvin25)-1/(R * (AirTemp + Kelvin0))));
        Q10Kc = exp(EKc *(1/(R * Kelvin25)-1/(R * (AirTemp + Kelvin0))));
        Q10Ko = exp(EKo *(1/(R * Kelvin25)-1/(R * (AirTemp + Kelvin0))));

        Vcmax = Vcmax1 * Q10rb;
        Kc = Kc2 * Q10Kc;
        Ko = Ko2 * Q10Ko;
        Vpmax = Vpmax1 * Q10p;        
        Jmax = Jmax1 * pow(Q10cb,(AirTemp-25)/10);

        Cm = 0.4 * Ca ; 
        Om = Oa ;

        /*    Rd = 0.01 * Vcmax; */
        Rd = 0.08;
        Rm = 0.5 * Rd ;
        // Rs = 0.5 * Rd; unused

        /* Light limited */
        I2 = (Idir * 0.85)/2;
        J = (Jmax + I2  - sqrt(pow(Jmax+I2,2) - 4 * theta * I2 * Jmax ))/2*theta;        
        /* Long formula for light limited */

        aa = 1 - (7 * gammaStar * alpha )/3 * 0.047;
        bb = -(((0.4 * J)/2 - Rm + gs * Cm) + ((1-0.4)*J/3-Rd) + 
                gs*(7*gammaStar*Om/3) + 
                (alpha*gammaStar/0.047)*((1-0.04)*J/3 + 7*Rd/3));
        cc = ((0.4 * J)/2 - Rm + gs * Cm)*((1-0.4)*J/3-Rd) -
            gs * gammaStar * Om *((1-0.4)*J/3 + 7*Rd/3);

        Aj = (-bb - sqrt(pow(bb,2)-4*aa*cc))/2*aa;

        /* Other part */
        Vp = (Cm * Vpmax) / (Cm + Kp) ;
        if(Vpr < Vp){
            Vp = Vpr;
        }

        /* Alternative formula */
        Ko1 = Ko * 1e3;
        Om1 = Om * 1e3;

        a1 = 1 - alpha / 0.047 * Kc/Ko1 ;
        b1 = -((Vp - Rm + gs * Cm)+
                (Vcmax - Rd)+
                gs*(Kc*(1+Om1/Ko1))+
                ((alpha/0.047)*(gammaStar*Vcmax+Rd*Kc/Ko1)));
        c1 = (Vcmax - Rd)*(Vp-Rm+gs*Cm)-
            (Vcmax * gs * gammaStar * Om1 + Rd*gs*Kc*(1+Om1/Ko1));

        c3 = pow(b1,2) - 4*a1*c1;
        if(c3 < 0){
            c3 = 0;
        }
        Ac0 = (-b1 - sqrt(c3)) / 2*a1 ;

        AcLCO2 = (Cm * Vpmax/(Cm+Kp)) - Rm + gs * Cm;

        if(Ac0 < AcLCO2){
            Ac = Ac0;
        }else{
            Ac = AcLCO2;
        }

        if(Aj < Ac){
            A = Aj;
        }else{
            A = Ac;
        }

        Os = alpha * A / 0.047 * gs + Om;

        if(Aj <= Ac){ 
            Cs = Cm + (Vp - Aj - Rm)/gs;
        }else{ 
            Cs0 = gammaStar * Os + Kc*(1+Os/Ko)*((Ac+Rd)/Vcmax); 
            Cs1 = 1 - (Ac+Rd)/Vcmax; 
            Cs = Cs0/Cs1;
        }

        /* Calculating Gs */
        if(A < 0){
            StomCond = G0;
        }else{
            StomCond = G1 * (A * rh * P) / (Cm*1e-1) + G0;
        }
        /* Organizing the results */
        REAL(Assim)[i] = A;
        REAL(Gs)[i] = StomCond;
        REAL(Ci)[i] = Cs;
        REAL(Oxy)[i] = Os;
    }

    SET_VECTOR_ELT(lists,0,Assim);
    SET_VECTOR_ELT(lists,1,Gs);
    SET_VECTOR_ELT(lists,2,Ci);
    SET_VECTOR_ELT(lists,3,Oxy);

    SET_STRING_ELT(names,0,mkChar("Assim"));
    SET_STRING_ELT(names,1,mkChar("Gs"));
    SET_STRING_ELT(names,2,mkChar("Ci"));
    SET_STRING_ELT(names,3,mkChar("Oxy"));
    setAttrib(lists,R_NamesSymbol,names);
    UNPROTECT(6);
    return(lists);
}

}  // extern "C"

