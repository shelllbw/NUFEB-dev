/* ----------------------------------------------------------------------
   LAMMPS - Large-scale Atomic/Molecular Massively Parallel Simulator
   http://lammps.sandia.gov, Sandia National Laboratories
   Steve Plimpton, sjplimp@sandia.gov

   Copyright (2003) Sandia Corporation.  Under the terms of Contract
   DE-AC04-94AL85000 with Sandia Corporation, the U.S. Government retains
   certain rights in this software.  This software is distributed under
   the GNU General Public License.

   See the README file in the top-level LAMMPS directory.
------------------------------------------------------------------------- */

#include "math.h"
#include "string.h"
#include "stdlib.h"
#include "fix_diffnugrowth.h"
#include "atom.h"
#include "update.h"
#include "group.h"
#include "modify.h"
#include "force.h"
#include "pair.h"
#include "pair_hybrid.h"
#include "kspace.h"
#include "fix_store.h"
#include "input.h"
#include "variable.h"
#include "respa.h"
#include "domain.h"
#include "math_const.h"
#include "memory.h"
#include "error.h"

using namespace LAMMPS_NS;
using namespace FixConst;
using namespace MathConst;

/* ---------------------------------------------------------------------- */

FixDiffNuGrowth::FixDiffNuGrowth(LAMMPS *lmp, int narg, char **arg) : Fix(lmp, narg, arg)
{
  if (narg != 48) error->all(FLERR,"Not enough arguments in fix diff growth command");

  nevery = force->inumeric(FLERR,arg[3]);
  if (nevery < 0) error->all(FLERR,"Illegal fix growth command");

  var = new char*[33];
  ivar = new int[33];

  int i;
  for (i = 0; i < 33; i++) {
    int n = strlen(&arg[4+i][2]) + 1;
    var[i] = new char[n];
    strcpy(var[i],&arg[4+i][2]);
  }

  nx = atoi(arg[37]);
  ny = atoi(arg[38]);
  nz = atoi(arg[39]);

  if (domain->triclinic == 0) {
    xlo = domain->boxlo[0];
    xhi = domain->boxhi[0];
    ylo = domain->boxlo[1];
    yhi = domain->boxhi[1];
    zlo = domain->boxlo[2];
    zhi = domain->boxhi[2];
  }
  else {
    xlo = domain->boxlo_bound[0];
    xhi = domain->boxhi_bound[0];
    ylo = domain->boxlo_bound[1];
    yhi = domain->boxhi_bound[1];
    zlo = domain->boxlo_bound[2];
    zhi = domain->boxhi_bound[2];
  }

  xloDirch = false;
  xhiDirch = false;
  yloDirch = false;
  yhiDirch = false;
  zloDirch = false;
  zhiDirch = false;

  if (strcmp(arg[40],"dirch") == 0) {
  	i = 41;
  	while (strcmp(arg[i],"neu") != 0) {
  		if (strcmp(arg[i],"xlo") == 0) {
  			xloDirch = true;
  		}
  		else if (strcmp(arg[i],"xhi") == 0) {
  			xhiDirch = true;
  		}
  		else if (strcmp(arg[i],"ylo") == 0) {
  			yloDirch = true;
  		}
  		else if (strcmp(arg[i],"yhi") == 0) {
  			yhiDirch = true;
  		}
  		else if (strcmp(arg[i],"zlo") == 0) {
  			zloDirch = true;
  		}
  		else if (strcmp(arg[i],"zhi") == 0) {
  			zhiDirch = true;
  		}
  		i++;
  	}
  }
  else if (strcmp(arg[40],"neu") == 0) {
  	i = 41;
  	while (strcmp(arg[i],"dirch") != 0) {
  		i++;
  	}
  	for (;i < narg; i++) {
  		if (strcmp(arg[i],"xlo") == 0) {
  			xloDirch = true;
  		}
  		else if (strcmp(arg[i],"xhi") == 0) {
  			xhiDirch = true;
  		}
  		else if (strcmp(arg[i],"ylo") == 0) {
  			yloDirch = true;
  		}
  		else if (strcmp(arg[i],"yhi") == 0) {
  			yhiDirch = true;
  		}
  		else if (strcmp(arg[i],"zlo") == 0) {
  			zloDirch = true;
  		}
  		else if (strcmp(arg[i],"zhi") == 0) {
  			zhiDirch = true;
  		}
  	}
  }
}

/* ---------------------------------------------------------------------- */

FixDiffNuGrowth::~FixDiffNuGrowth()
{
  int i;
  for (i = 0; i < 33; i++) {
    delete [] var[i];
  }
  delete [] var;
  delete [] ivar;
  delete [] xCell;
  delete [] yCell;
  delete [] zCell;
  delete [] cellVol;
  delete [] ghost;
  delete [] subCell;
  delete [] o2Cell;
  delete [] nh4Cell;
  delete [] no2Cell;
  delete [] no3Cell;
}

/* ---------------------------------------------------------------------- */

int FixDiffNuGrowth::setmask()
{
  int mask = 0;
  mask |= PRE_FORCE;
  return mask;
}

/* ---------------------------------------------------------------------- */

void FixDiffNuGrowth::init()
{
  if (!atom->radius_flag)
    error->all(FLERR,"Fix growth requires atom attribute diameter");

  int n;
  for (n = 0; n < 33; n++) {
    ivar[n] = input->variable->find(var[n]);
    if (ivar[n] < 0)
      error->all(FLERR,"Variable name for fix nugrowth does not exist");
    if (!input->variable->equalstyle(ivar[n]))
      error->all(FLERR,"Variable for fix nugrowth is invalid style");
  }

  double sub = input->variable->compute_equal(ivar[28]);
  double o2 = input->variable->compute_equal(ivar[29]);
  double no2 = input->variable->compute_equal(ivar[30]);
  double no3 = input->variable->compute_equal(ivar[31]);
  double nh4 = input->variable->compute_equal(ivar[32]);

  numCells = (nx+2)*(ny+2)*(nz+2);

  xCell = new double[numCells];
  yCell = new double[numCells];
  zCell = new double[numCells];
  cellVol = new double[numCells];
  ghost = new bool[numCells];
  subCell = new double[numCells];
  o2Cell = new double[numCells];
  nh4Cell = new double[numCells];
  no2Cell = new double[numCells];
  no3Cell = new double[numCells];


  xstep = (xhi - xlo) / nx;
  ystep = (yhi - ylo) / ny;
  zstep = (zhi - zlo) / nz;


  double i, j, k;
  int cell = 0;
  for (i = xlo - (xstep/2); i < xhi + xstep; i += xstep) {
    for (j = ylo - (ystep/2); j < yhi + ystep; j += ystep) {
      for (k = zlo - (zstep/2); k < zhi + zstep; k += zstep) {
        xCell[cell] = i;
        yCell[cell] = j;
        zCell[cell] = k;
        subCell[cell] = sub;
        o2Cell[cell] = o2;
        no2Cell[cell] = no2;
        no3Cell[cell] = no3;
        nh4Cell[cell] = nh4;
        cellVol[cell] = xstep * ystep * zstep;
        ghost[cell] = false;
        if (i < xlo || i > xhi || j < ylo ||
        	j > yhi || k < zlo || k > zhi) {
        	ghost[cell] = true;
        }
        cell++;
      }
    }
  }
}

/* ---------------------------------------------------------------------- */

void FixDiffNuGrowth::pre_force(int vflag)
{
  if (nevery == 0) return;
  if (update->ntimestep % nevery) return;
  change_dia();
}

/* ---------------------------------------------------------------------- */

void FixDiffNuGrowth::change_dia()
{

  modify->clearstep_compute();

  double KsHET = input->variable->compute_equal(ivar[0]);
  double Ko2HET = input->variable->compute_equal(ivar[1]);
  double Kno2HET = input->variable->compute_equal(ivar[2]);
  double Kno3HET = input->variable->compute_equal(ivar[3]);
  double Knh4AOB = input->variable->compute_equal(ivar[4]);
  double Ko2AOB = input->variable->compute_equal(ivar[5]);
  double Kno2NOB = input->variable->compute_equal(ivar[6]);
  double Ko2NOB = input->variable->compute_equal(ivar[7]);
  double MumHET = input->variable->compute_equal(ivar[8]);
  double MumAOB = input->variable->compute_equal(ivar[9]);
  double MumNOB = input->variable->compute_equal(ivar[10]);
  double etaHET = input->variable->compute_equal(ivar[11]);
  double bHET = input->variable->compute_equal(ivar[12]); // R6
  double bAOB = input->variable->compute_equal(ivar[13]); // R7
  double bNOB = input->variable->compute_equal(ivar[14]); // R8
  double bEPS = input->variable->compute_equal(ivar[15]); // R9
  double YHET = input->variable->compute_equal(ivar[16]);
  double YAOB = input->variable->compute_equal(ivar[17]);
  double YNOB = input->variable->compute_equal(ivar[18]);
  double YEPS = input->variable->compute_equal(ivar[19]);
  double Y1 = input->variable->compute_equal(ivar[20]);
  double EPSdens = input->variable->compute_equal(ivar[21]);
  double Do2 = input->variable->compute_equal(ivar[22]);
  double Dnh4 = input->variable->compute_equal(ivar[23]);
  double Dno2 = input->variable->compute_equal(ivar[24]);
  double Dno3 = input->variable->compute_equal(ivar[25]);
  double Ds = input->variable->compute_equal(ivar[26]);
  double diffT = input->variable->compute_equal(ivar[27]);
  double subVal = input->variable->compute_equal(ivar[28]);
  double o2Val = input->variable->compute_equal(ivar[29]);
  double no2Val = input->variable->compute_equal(ivar[30]);
  double no3Val = input->variable->compute_equal(ivar[31]);
  double nh4Val = input->variable->compute_equal(ivar[32]);

  double density;

  double *radius = atom->radius;
  double *rmass = atom->rmass;
  double *outerMass = atom->outerMass;
  double *outerRadius = atom->outerRadius;
  double *sub = atom->sub;
  double *o2 = atom->o2;
  double *nh4 = atom->nh4;
  double *no2 = atom->no2;
  double *no3 = atom->no3;
  int *mask = atom->mask;
  int *type = atom->type;
  int nlocal = atom->nlocal;
  int nall = nlocal + atom->nghost;
  int i;

  int cellIn[nall];
  double xHET[numCells];
  double xAOB[numCells];
  double xNOB[numCells];
  double xEPS[numCells];
  double xTot[numCells];
  for (int cell = 0; cell < numCells; cell++) {
  	xHET[cell] = 0.0;
  	xAOB[cell] = 0.0;
  	xNOB[cell] = 0.0;
  	xEPS[cell] = 0.0;
  	xTot[cell] = 0.0;
  }

  double R1[numCells];
  double R2[numCells];
  double R3[numCells];
  double R4[numCells];
  double R5[numCells];
  double initR1[numCells];
  double initR2[numCells];
  double initR3[numCells];
  double initR4[numCells];
  double initR5[numCells];
  // double R6[numCells] = bHET;
  // double R7[numCells] = bAOB;
  // double R8[numCells] = bNOB;
  // double R9[numCells] = bEPS;
  double Rs[numCells];
  double Ro2[numCells];
  double Rnh4[numCells];
  double Rno2[numCells];
  double Rno3[numCells];
  double cellDo2[numCells];
  double cellDnh4[numCells];
  double cellDno2[numCells];
  double cellDno3[numCells];
  double cellDs[numCells];

  // Figure out which cell each particle is in
  for (i = 0; i < nall; i++) {
    if (mask[i] & groupbit) {
      double gHET = 0;
      double gAOB = 0;
      double gNOB = 0;
      double gEPS = 0;
      if (type[i] == 1) {
        gHET = 1;
      }
      if (type[i] == 2) {
        gAOB = 1;
      }
      if (type[i] == 3) {
        gNOB = 1;
      }
      if (type[i] == 4) {
        gEPS = 1;
      }

      int j;
      for (j = 0; j < numCells; j ++) {
        if ((xCell[j] - xstep/2) <= atom->x[i][0] &&
            (xCell[j] + xstep/2) >= atom->x[i][0] &&
            (yCell[j] - ystep/2) <= atom->x[i][1] &&
            (yCell[j] + ystep/2) >= atom->x[i][1] &&
            (zCell[j] - zstep/2) <= atom->x[i][2] &&
            (zCell[j] + zstep/2) >= atom->x[i][2]) {
          cellIn[i] = j;
          xHET[j] += (gHET * rmass[i])/cellVol[j];
          xAOB[j] += (gAOB * rmass[i])/cellVol[j];

        //  xNOB[j] += rmass[i]/cellVol[j];
          xNOB[j] += (gNOB *rmass[i])/cellVol[j];

                     // fprintf(stdout, "xNOBcell[%i]: %e\n", j, xNOB[j]);


          xEPS[j] += (gEPS * rmass[i])/cellVol[j];
          xTot[j] += rmass[i]/cellVol[j];
        }
      }
    }
  }

  for (int cell = 0; cell < numCells; cell++) {    
    initR1[cell] = MumHET*(subCell[cell]/(KsHET+subCell[cell]))*(o2Cell[cell]/(Ko2HET+o2Cell[cell]));
    initR2[cell] = MumAOB*(nh4Cell[cell]/(Knh4AOB+nh4Cell[cell]))*(o2Cell[cell]/(Ko2AOB+o2Cell[cell]));
    initR3[cell] = MumNOB*(no2Cell[cell]/(Kno2NOB+no2Cell[cell]))*(o2Cell[cell]/(Ko2NOB+o2Cell[cell]));
    initR4[cell] = etaHET*MumHET*(subCell[cell]/(KsHET+subCell[cell]))*(no3Cell[cell]/(Kno3HET+no3Cell[cell]))*(Ko2HET/(Ko2HET+o2Cell[cell]));
    initR5[cell] = etaHET*MumHET*(subCell[cell]/(KsHET+subCell[cell]))*(no2Cell[cell]/(Kno2HET+no2Cell[cell]))*(Ko2HET/(Ko2HET+o2Cell[cell]));
  }

  double subSum;
  double o2Sum;
  double no2Sum;
  double no3Sum;
  double nh4Sum;

  double prevsubSum;
  double prevo2Sum;
  double prevno2Sum;
  double prevno3Sum;
  double prevnh4Sum;
  prevsubSum = 0.0;
  prevo2Sum = 0.0;
  prevno2Sum = 0.0;
  prevno3Sum = 0.0;
  prevnh4Sum = 0.0;

  bool convergence = false;

  int iteration = 0;

  double tol = 1e-3; // Tolerance for convergence criteria for nutrient balance equation

  double dtRatio = 0.002; // Ratio of physical time step divided by time step of diffusion

  // Outermost while loop for the convergence criterion 

  while (!convergence) {

  	iteration ++;


  	subSum = 0.0;
  	o2Sum = 0.0;
  	no2Sum = 0.0;
  	no3Sum = 0.0;
  	nh4Sum = 0.0;



  	for (int cell = 0; cell < numCells; cell++) {
    	double diffusionFunction = 1 - ((0.43 * pow(xTot[cell], 0.92))/(11.19+0.27*pow(xTot[cell], 0.99)));

    	cellDo2[cell] = diffusionFunction * Do2; 
    	cellDnh4[cell] = diffusionFunction * Dnh4; 
    	cellDno2[cell] = diffusionFunction * Dno2; 
    	cellDno3[cell] = diffusionFunction * Dno3; 
    	cellDs[cell] = diffusionFunction * Ds;
  	
    	R1[cell] = MumHET*(subCell[cell]/(KsHET+subCell[cell]))*(o2Cell[cell]/(Ko2HET+o2Cell[cell]));
    	R2[cell] = MumAOB*(nh4Cell[cell]/(Knh4AOB+nh4Cell[cell]))*(o2Cell[cell]/(Ko2AOB+o2Cell[cell]));
    	R3[cell] = MumNOB*(no2Cell[cell]/(Kno2NOB+no2Cell[cell]))*(o2Cell[cell]/(Ko2NOB+o2Cell[cell]));
    	R4[cell] = etaHET*MumHET*(subCell[cell]/(KsHET+subCell[cell]))*(no3Cell[cell]/(Kno3HET+no3Cell[cell]))*(Ko2HET/(Ko2HET+o2Cell[cell]));
    	R5[cell] = etaHET*MumHET*(subCell[cell]/(KsHET+subCell[cell]))*(no2Cell[cell]/(Kno2HET+no2Cell[cell]))*(Ko2HET/(Ko2HET+o2Cell[cell]));

             

         //  fprintf(stdout, "1/cell volume[%i]: %e\n", cell, 1/cellVol[cell]);

        // if (xNOB[cell] != xNOB[cell]) 
         //       {
       // 	       fprintf(stdout, "xNOBcell[%i]: %e\n", cell, xNOB[cell]);
      	
      	//       }


    //	if (cell == 39) {
    //		fprintf(stdout, "(-1/YHET): %f\n", cell, (-1/YHET));
    //  		fprintf(stdout, "R1[%i]: %e\n", cell, R1[cell]);
    //  		fprintf(stdout, "R4[%i]: %e\n", cell, R4[cell]);
    //  		fprintf(stdout, "R5[%i]: %e\n", cell, R5[cell]);
    //  		fprintf(stdout, "xHET[%i]: %e\n", cell, xHET[cell]);
    //  		fprintf(stdout, "xAOB[%i]: %e\n", cell, xAOB[cell]);
    //  		fprintf(stdout, "xNOB[%i]: %e\n", cell, xNOB[cell]);
     // 		fprintf(stdout, "xEPS[%i]: %e\n", cell, xEPS[cell]);
    //  		fprintf(stdout, "bHET[%i]: %e\n", cell, bHET);
    //  		fprintf(stdout, "bAOB[%i]: %e\n", cell, bAOB);
    //  		fprintf(stdout, "bNOB[%i]: %e\n", cell, bNOB);
   // 	}




    	Rs[cell] = ( (-1/YHET) * ( (R1[cell]+R4[cell]+R5[cell]) * xHET[cell] ) ) + ( (1-Y1) * ( bHET*xHET[cell]+bAOB*xAOB[cell]+bNOB*xNOB[cell] ) ) + ( bEPS*xEPS[cell] );
    	Ro2[cell] = (((1-YHET-YEPS)/YHET)*R1[cell]*xHET[cell])-(((3.42-YAOB)/YAOB)*R2[cell]*xAOB[cell])-(((1.15-YNOB)/YNOB)*R3[cell]*xNOB[cell]);
    	Rnh4[cell] = -(1/YAOB)*R2[cell]*xAOB[cell];
    	Rno2[cell] = ((1/YAOB)*R2[cell]*xAOB[cell])-((1/YNOB)*R3[cell]*xNOB[cell])-(((1-YHET-YEPS)/(1.17*YHET))*R5[cell]*xHET[cell]);
    	Rno3[cell] = ((1/YNOB)*R3[cell]*xNOB[cell])-(((1-YHET-YEPS)/(2.86*YHET))*R4[cell]*xHET[cell]);

//    	if (cell == 39) {
//    		fprintf(stdout, "Rs[%i]: %e\n", cell, Rs[cell]);
//      		fprintf(stdout, "Ro2[%i]: %e\n", cell, Ro2[cell]);
//      		fprintf(stdout, "Rno2[%i]: %e\n", cell, Rno2[cell]);
//      		fprintf(stdout, "Rno3[%i]: %e\n", cell, Rno3[cell]);
//    		fprintf(stdout, "Rnh4[%i]: %e\n", cell, Rnh4[cell]);
//    	}
      	



    	subCell[cell] -= Rs[cell] * update->dt;
    	o2Cell[cell] -= Ro2[cell] * update->dt;
    	no2Cell[cell] -= Rno2[cell] * update->dt;
    	no3Cell[cell] -= Rno3[cell] * update->dt;
    	nh4Cell[cell] -= Rnh4[cell] * update->dt;

      	// if (nh4Cell[cell] != nh4Cell[cell]) {
       //  	fprintf(stdout, "nh4Cell[%i]: %e\n", cell, nh4Cell[cell]);
      	// }

      	//fprintf(stdout, "Rno3[%i] %f\n", cell, Rno3[cell]);

    	computeFlux(cellDs, subCell, subVal, Rs[cell], diffT, cell);
    	computeFlux(cellDo2, o2Cell, o2Val, Ro2[cell], diffT, cell);
    	computeFlux(cellDnh4, nh4Cell, nh4Val, Rnh4[cell], diffT, cell);
    	computeFlux(cellDno2, no2Cell, no2Val, Rno2[cell], diffT, cell);
    	computeFlux(cellDno3, no3Cell, no3Val, Rno3[cell], diffT, cell);

      	if (!ghost[cell]) {
        // fprintf(stdout, "subCell[%i]: %f\n", cell, subCell[cell]);
        	subSum += subCell[cell];
    		o2Sum += o2Cell[cell];
    		no2Sum += no2Cell[cell];
    		no3Sum += no3Cell[cell];
    		nh4Sum += nh4Cell[cell];
      	}
      
    	// add all the subcell values and calculate the difference from previous iteration
    	// End of the convergence loop.    

  	}

  	// fprintf(stdout, "subSum: %f\n", subSum);
  	// fprintf(stdout, "prevsubSum: %f\n", prevsubSum);
  	// fprintf(stdout, "subSum - prevsubSum: %f\n", subSum - prevsubSum);
  	// fprintf(stdout, "abs(subSum - prevsubSum): %f\n", abs(subSum - prevsubSum));
  	// fprintf(stdout, "abs((o2Sum - prevo2Sum)): %f\n", abs((o2Sum - prevo2Sum)));
  	// fprintf(stdout, "abs((no2Sum - prevno2Sum)): %f\n", abs((no2Sum - prevno2Sum)));
  	// fprintf(stdout, "abs((no3Sum - prevno3Sum)): %f\n", abs((no3Sum - prevno3Sum)));
  	// fprintf(stdout, "abs((nh4Sum - prevnh4Sum)): %f\n", abs((nh4Sum - prevnh4Sum)));
  	// fprintf(stdout, "(iteration*diffT)/update->dt: %f\n", (iteration*diffT)/update->dt);
  	// fprintf(stdout, "tol: %e\n", tol);
  	// fprintf(stdout, "dtRatio: %f\n", dtRatio);
  	// fprintf(stdout, "abs(subSum - prevsubSum) < tol: %i\n", abs(subSum - prevsubSum) < tol);

  	double abssubDiff = subSum - prevsubSum;
  	double abso2Diff = o2Sum - prevo2Sum;
  	double absno2Diff = no2Sum - prevno2Sum;
  	double absno3Diff = no3Sum - prevno3Sum;
  	double absnh4Diff = nh4Sum - prevnh4Sum;

  	if(abssubDiff < 0) {
  		abssubDiff = -abssubDiff;
  	}
  	if(abso2Diff < 0) {
  		abso2Diff = -abso2Diff;
  	}
  	if(absno2Diff < 0) {
  		absno2Diff = -absno2Diff;
  	}
  	if(absno3Diff < 0) {
  		absno3Diff = -absno3Diff;
  	}
  	if(absnh4Diff < 0) {
  		absnh4Diff = -absnh4Diff;
  	}

  	// fprintf(stdout, "subSum - prevsubSum: %e\n", subSum - prevsubSum);
  	// fprintf(stdout, "abs(subSum - prevsubSum): %e\n", abs(subSum - prevsubSum));
  	// fprintf(stdout, "abssubDiff/tol: %f\n", abssubDiff/tol);
  	// fprintf(stdout, "tol: %f\n", tol);
  	// fprintf(stdout, "abssubDiff < tol: %i\n", abssubDiff < tol);
  	

  	if ((abssubDiff < tol &&
  		abso2Diff < tol &&
  		absno2Diff < tol &&
  		absno3Diff < tol &&
  		absnh4Diff < tol) ||
  		(iteration*diffT)/update->dt > dtRatio) {
  		convergence = true;
  	}
  	else {
  		prevsubSum = subSum;
  		prevo2Sum = o2Sum;
  		prevno2Sum = no2Sum;
  		prevno3Sum = no3Sum;
  		prevnh4Sum = nh4Sum;
  	}

  }

  // fprintf(stdout, "Number of iterations for substrate nutrient mass balance:  %i\n", iteration);

  for (i = 0; i < nall; i++) {
    if (mask[i] & groupbit) {
      double gHET = 0;
      double gAOB = 0;
      double gNOB = 0;
      double gEPS = 0;
      if (type[i] == 1) {
        gHET = 1;
      }
      if (type[i] == 2) {
        gAOB = 1;
      }
      if (type[i] == 3) {
        gNOB = 1;
      }
      if (type[i] == 4) {
        gEPS = 1;
      }

      sub[i] = subCell[cellIn[i]];
      o2[i] = o2Cell[cellIn[i]];
      nh4[i] = nh4Cell[cellIn[i]];
      no2[i] = no2Cell[cellIn[i]];
      no3[i] = no3Cell[cellIn[i]];

      // int j;
      // for (j = 0; j < numCells; j ++) {
      //   if ((xCell[j] - xstep/2) <= atom->x[i][0] &&
      //       (xCell[j] + xstep/2) >= atom->x[i][0] &&
      //       (yCell[j] - ystep/2) <= atom->x[i][1] &&
      //       (yCell[j] + ystep/2) >= atom->x[i][1] &&
      //       (zCell[j] - zstep/2) <= atom->x[i][2] &&
      //       (zCell[j] + zstep/2) >= atom->x[i][2]) {
      //   	cellIn[i] = j;
      //   	xHET[j] += (gHET * rmass[i])/cellVol[j];
      //   	xAOB[j] += (gAOB * rmass[i])/cellVol[j];
      //   	xNOB[j] += (gNOB * rmass[i])/cellVol[j];
      //   	xEPS[j] += (gEPS * rmass[i])/cellVol[j];
      //    xTot[j] += rmass[i]/cellVol[j];
      //   }
      // }

      // double R1 = MumHET*(sub[i]/(KsHET+sub[i]))*(o2[i]/(Ko2HET+o2[i]));
      // double R2 = MumAOB*(nh4[i]/(Knh4AOB+nh4[i]))*(o2[i]/(Ko2AOB+o2[i]));
      // double R3 = MumNOB*(no2[i]/(Kno2NOB+no2[i]))*(o2[i]/(Ko2NOB+o2[i]));
      // double R4 = etaHET*MumHET*(sub[i]/(KsHET+sub[i]))*(no3[i]/(Kno3HET+no3[i]))*(Ko2HET/(Ko2HET+o2[i]));
      // double R5 = etaHET*MumHET*(sub[i]/(KsHET+sub[i]))*(no2[i]/(Kno2HET+no2[i]))*(Ko2HET/(Ko2HET+o2[i]));
      // double R6 = bHET;
      // double R7 = bAOB;
      // double R8 = bNOB;
      // double R9 = bEPS;

      double value = update->dt * (gHET*(initR1[cellIn[i]]+initR4[cellIn[i]]+initR5[cellIn[i]]) + gAOB*initR2[cellIn[i]] + gNOB*initR3[cellIn[i]] - gEPS*bEPS);

      density = rmass[i] / (4.0*MY_PI/3.0 *
                      radius[i]*radius[i]*radius[i]);
      double oldMass = rmass[i];
      rmass[i] = rmass[i]*(1 + (value*nevery));
      if (rmass[i] <= 0) {
        rmass[i] = oldMass;
      }

      // fprintf(stdout, "Radius: %e Outer Radius: %e\n", radius[i], outerRadius[i]);
      // fprintf(stdout, "ID: %i Type: %i Outer Mass: %e\n", atom->tag[i], atom->type[i], outerMass[i]);
      

      double value2 = update->dt * (YEPS/YHET)*(initR1[cellIn[i]]+initR4[cellIn[i]]+initR5[cellIn[i]]);
      if (type[i] == 1) {
        outerMass[i] = (((4.0*MY_PI/3.0)*((outerRadius[i]*outerRadius[i]*outerRadius[i])-(radius[i]*radius[i]*radius[i])))*EPSdens)+(value2*nevery*rmass[i]);

        outerRadius[i] = pow((3.0/(4.0*MY_PI))*((rmass[i]/density)+(outerMass[i]/EPSdens)),(1.0/3.0));
        radius[i] = pow((3.0/(4.0*MY_PI))*(rmass[i]/density),(1.0/3.0));
      }
      else {
        radius[i] = pow((3.0/(4.0*MY_PI))*(rmass[i]/density),(1.0/3.0));
        outerMass[i] = 0.0;
        outerRadius[i] = radius[i];

      }
      
    }

  }

  modify->addstep_compute(update->ntimestep + nevery);
}

void FixDiffNuGrowth::computeFlux(double *cellDNu, double *nuCell, double nuVal, double rateNu, double diffT, int cell) {
	int leftCell = cell - (nz+2)*(ny+2); // x direction
    int rightCell = cell + (nz+2)*(ny+2); // x direction
    int downCell = cell - (nz+2); // y direction
    int upCell = cell + (nz+2); // y direction
    int backwardCell = cell - 1; // z direction
    int forwardCell = cell + 1; // z direction


    // fprintf(stdout, "nuVal: %f\n", nuVal);
    // fprintf(stdout, "rateNu: %f\n", rateNu);
    // fprintf(stdout, "diffT: %f\n", diffT);

    // fprintf(stdout, "cellDNu[%i]: %f\n", cell, cellDNu[cell]);
    // fprintf(stdout, "cellDNu[%i]: %f\n", leftCell, cellDNu[leftCell]);
    // fprintf(stdout, "cellDNu[%i]: %f\n", rightCell, cellDNu[rightCell]);
    // fprintf(stdout, "cellDNu[%i]: %f\n", downCell, cellDNu[downCell]);
    // fprintf(stdout, "cellDNu[%i]: %f\n", upCell, cellDNu[upCell]);
    // fprintf(stdout, "cellDNu[%i]: %f\n", backwardCell, cellDNu[backwardCell]);
    // fprintf(stdout, "cellDNu[%i]: %f\n", forwardCell, cellDNu[forwardCell]);

    // fprintf(stdout, "nuCell[%i]: %f\n", cell, nuCell[cell]);
    // fprintf(stdout, "nuCell[%i]: %f\n", leftCell, nuCell[leftCell]);
    // fprintf(stdout, "nuCell[%i]: %f\n", rightCell, nuCell[rightCell]);
    // fprintf(stdout, "nuCell[%i]: %f\n", downCell, nuCell[downCell]);
    // fprintf(stdout, "nuCell[%i]: %f\n", upCell, nuCell[upCell]);
    // fprintf(stdout, "nuCell[%i]: %f\n", backwardCell, nuCell[backwardCell]);
    // fprintf(stdout, "nuCell[%i]: %f\n", forwardCell, nuCell[forwardCell]);

    // assign values to the ghost-cells according to the boundary conditions. 
    // If ghostcells are Neu then take the values equal from the adjacent cells.
    // if ghostcells are dirich then take the values equal to negative of the adjacent cells.  
    //fprintf(stdout, "Num Cells: %i\n", numCells);
    if (ghost[cell]) {
    	// fprintf(stdout, "Ghost Cell: %i\n", cell);
    	if (zCell[cell] < zlo && !ghost[forwardCell]) {
    		// fprintf(stdout, "Forward Cell: %i\n", forwardCell);
    		if (zloDirch) {
    			nuCell[cell] = 2*nuVal - nuCell[forwardCell]; // if you want zero nutrient at the boundary, remove "2*nuVal"
    		}
    		else {
    			nuCell[cell] = nuCell[forwardCell];
    		}
    	}
    	else if (zCell[cell] > zhi && !ghost[backwardCell]) {
    		// fprintf(stdout, "Backward Cell: %i\n", backwardCell);
    		if (zhiDirch) {
    			nuCell[cell] = 2*nuVal - nuCell[backwardCell]; // if you want zero nutrient at the boundary, remove "2*nuVal"
    		}
    		else {
    			nuCell[cell] = nuCell[backwardCell];
    		}
    	}
    	else if (yCell[cell] < ylo && !ghost[upCell]) {
    		// fprintf(stdout, "Up Cell: %i\n", upCell);
    		if (yloDirch) {
    			nuCell[cell] = 2*nuVal - nuCell[upCell]; // if you want zero nutrient at the boundary, remove "2*nuVal"
    		}
    		else {
    			nuCell[cell] = nuCell[upCell];
    		}
    	}
    	else if (yCell[cell] > yhi && !ghost[downCell]) {
    		// fprintf(stdout, "Down Cell: %i\n", downCell);
    		if (yhiDirch) {
    			nuCell[cell] = 2*nuVal - nuCell[downCell]; // if you want zero nutrient at the boundary, remove "2*nuVal"
    		}
    		else {
    			nuCell[cell] = nuCell[downCell];
    		}
    	}
    	else if (xCell[cell] < xlo && !ghost[rightCell]) {
    		// fprintf(stdout, "Right Cell: %i\n", rightCell);
    		if (xloDirch) {
    			nuCell[cell] = 2*nuVal - nuCell[rightCell]; // if you want zero nutrient at the boundary, remove "2*nuVal"
    		}
    		else {
    			nuCell[cell] = nuCell[rightCell];
    		}
    	}
    	else if (xCell[cell] > xhi && !ghost[leftCell]) {
    		// fprintf(stdout, "Left Cell: %i\n", leftCell);
    		if (xhiDirch) {
    			nuCell[cell] = 2*nuVal - nuCell[leftCell]; // if you want zero nutrient at the boundary, remove "2*nuVal"
    		}
    		else {
    			nuCell[cell] = nuCell[leftCell];
    		}
    	}
    	else {
    		if (zCell[cell] < zlo) {
    			if (zloDirch) {
    				nuCell[cell] = 2*nuVal - nuCell[forwardCell]; // if you want zero nutrient at the boundary, remove "2*nuVal"
    			}
	    		else {
	    			nuCell[cell] = nuCell[forwardCell];
	    		}
	    	}
	    	else if (zCell[cell] > zhi) {
	    		if (zhiDirch) {
	    			nuCell[cell] = 2*nuVal - nuCell[backwardCell]; // if you want zero nutrient at the boundary, remove "2*nuVal"
	    		}
	    		else {
	    			nuCell[cell] = nuCell[backwardCell];
	    		}
	    	}
	    	else if (yCell[cell] < ylo) {
	    		if (yloDirch) {
	    			nuCell[cell] = 2*nuVal - nuCell[upCell]; // if you want zero nutrient at the boundary, remove "2*nuVal"
	    		}
	    		else {
	    			nuCell[cell] = nuCell[upCell];
	    		}
	    	}
	    	else if (yCell[cell] > yhi) {
	    		if (yhiDirch) {
	    			nuCell[cell] = 2*nuVal - nuCell[downCell]; // if you want zero nutrient at the boundary, remove "2*nuVal"
	    		}
	    		else {
	    			nuCell[cell] = nuCell[downCell];
	    		}
	    	}
	    	else if (xCell[cell] < xlo) {
	    		if (xloDirch) {
	    			nuCell[cell] = 2*nuVal - nuCell[rightCell]; // if you want zero nutrient at the boundary, remove "2*nuVal"
	    		}
	    		else {
	    			nuCell[cell] = nuCell[rightCell];
	    		}
	    	}
	    	else if (xCell[cell] > xhi) {
	    		if (xhiDirch) {
	    			nuCell[cell] = 2*nuVal - nuCell[leftCell]; // if you want zero nutrient at the boundary, remove "2*nuVal"
	    		}
	    		else {
	    			nuCell[cell] = nuCell[leftCell];
	    		}
	    	}
    	}
    }
    else {
		double dRight = (cellDNu[cell] + cellDNu[rightCell]) / 2;
    	double jRight = dRight*(nuCell[rightCell] - nuCell[cell])/xstep;
    	double dLeft = (cellDNu[cell] + cellDNu[leftCell]) / 2;
    	double jLeft = dLeft*(nuCell[cell] - nuCell[leftCell])/xstep;
    	double jX = (jRight - jLeft)/xstep;

    	double dUp = (cellDNu[cell] + cellDNu[upCell]) / 2;
    	double jUp = dUp*(nuCell[upCell] - nuCell[cell])/ystep;
    	double dDown = (cellDNu[cell] + cellDNu[downCell]) / 2;
    	double jDown = dDown*(nuCell[cell] - nuCell[downCell])/ystep;
    	double jY = (jUp - jDown)/ystep;

    	double dForward = (cellDNu[cell] + cellDNu[forwardCell]) / 2;
    	double jForward = dForward*(nuCell[forwardCell] - nuCell[cell])/zstep;
    	double dBackward = (cellDNu[cell] + cellDNu[backwardCell]) / 2;
    	double jBackward = dBackward*(nuCell[cell] - nuCell[backwardCell])/zstep;
    	double jZ = (jForward - jBackward)/zstep;

    	// Adding fluxes in all the directions and the uptake rate (RHS side of the equation)
    	double Ratesub = jX + jY + jZ + rateNu;

    	//Updating the value: Ratesub*diffT + nuCell[cell](previous)
   		nuCell[cell] += Ratesub*diffT;
   	}
}