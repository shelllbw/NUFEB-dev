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
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>
#include "atom.h"

using namespace LAMMPS_NS;
using namespace FixConst;
using namespace MathConst;

/* ---------------------------------------------------------------------- */

FixDiffNuGrowth::FixDiffNuGrowth(LAMMPS *lmp, int narg, char **arg) : Fix(lmp, narg, arg)
{
    if (narg != 52) error->all(FLERR,"Not enough arguments in fix diff growth command");
    
    nevery = force->inumeric(FLERR,arg[3]);
    diffevery = force->inumeric(FLERR,arg[4]);
    outputevery = force->inumeric(FLERR,arg[51]);
    if (nevery < 0 || diffevery < 0 || outputevery < 0) error->all(FLERR,"Illegal fix growth command");
    
    var = new char*[42];
    ivar = new int[42];
    
    for (int i = 0; i < 37; i++) {
        int n = strlen(&arg[5+i][2]) + 1;
        var[i] = new char[n];
        strcpy(var[i],&arg[5+i][2]);
    }
    
    //BC concentration
    for(int i = 37; i < 42; i++){
        int n = strlen(&arg[9+i][2]) + 1;
        var[i] = new char[n];
        strcpy(var[i],&arg[9+i][2]);
    }
    
    if(strcmp(arg[45], "dirich") == 0) bflag = 1;
    else if(strcmp(arg[45], "neu") == 0) bflag = 2;
    else if(strcmp(arg[45], "mixed") == 0) bflag = 3;
    else error->all(FLERR,"Illegal boundary condition command");
    
    nx = atoi(arg[42]);
    ny = atoi(arg[43]);
    nz = atoi(arg[44]);
    
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
}

/* ---------------------------------------------------------------------- */

FixDiffNuGrowth::~FixDiffNuGrowth()
{
    int i;
    for (i = 0; i < 42; i++) {
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
    
    for (int n = 0; n < 42; n++) {
        ivar[n] = input->variable->find(var[n]);
        if (ivar[n] < 0)
            error->all(FLERR,"Variable name for fix nugrowth does not exist");
        if (!input->variable->equalstyle(ivar[n]))
            error->all(FLERR,"Variable for fix nugrowth is invalid style");
    }
    
    // Initialise constants from config file
    KsHET = input->variable->compute_equal(ivar[0]);
    Ko2HET = input->variable->compute_equal(ivar[1]);
    Kno2HET = input->variable->compute_equal(ivar[2]);
    Kno3HET = input->variable->compute_equal(ivar[3]);
    Knh4AOB = input->variable->compute_equal(ivar[4]);
    Ko2AOB = input->variable->compute_equal(ivar[5]);
    Kno2NOB = input->variable->compute_equal(ivar[6]);
    Ko2NOB = input->variable->compute_equal(ivar[7]);
    MumHET = input->variable->compute_equal(ivar[8]);
    MumAOB = input->variable->compute_equal(ivar[9]);
    MumNOB = input->variable->compute_equal(ivar[10]);
    etaHET = input->variable->compute_equal(ivar[11]);
    bHET = input->variable->compute_equal(ivar[12]); // R6
    bAOB = input->variable->compute_equal(ivar[13]); // R7
    bNOB = input->variable->compute_equal(ivar[14]); // R8
    bEPS = input->variable->compute_equal(ivar[15]); // R9
    bmHET = input->variable->compute_equal(ivar[16]);
    bmAOB = input->variable->compute_equal(ivar[17]);
    bmNOB =input->variable->compute_equal(ivar[18]);
    bX =input->variable->compute_equal(ivar[19]);
    YHET = input->variable->compute_equal(ivar[20]);
    YAOB = input->variable->compute_equal(ivar[21]);
    YNOB = input->variable->compute_equal(ivar[22]);
    YEPS = input->variable->compute_equal(ivar[23]);
    Y1 = input->variable->compute_equal(ivar[24]);
    EPSdens = input->variable->compute_equal(ivar[25]);
    Do2 = input->variable->compute_equal(ivar[26]);
    Dnh4 = input->variable->compute_equal(ivar[27]);
    Dno2 = input->variable->compute_equal(ivar[28]);
    Dno3 = input->variable->compute_equal(ivar[29]);
    Ds = input->variable->compute_equal(ivar[30]);
    diffT = input->variable->compute_equal(ivar[31]);
    
    //initial concentrations of non-ghost cells
    initsub = input->variable->compute_equal(ivar[32]);
    inito2 = input->variable->compute_equal(ivar[33]);
    initno2 = input->variable->compute_equal(ivar[34]);
    initno3 = input->variable->compute_equal(ivar[35]);
    initnh4 = input->variable->compute_equal(ivar[36]);
    
    //initial concentrations of boundary
    subBC = input->variable->compute_equal(ivar[37]);
    o2BC = input->variable->compute_equal(ivar[38]);
    no2BC = input->variable->compute_equal(ivar[39]);
    no3BC = input->variable->compute_equal(ivar[40]);
    nh4BC = input->variable->compute_equal(ivar[41]);
    
    //total numbers of cells (ghost + non-ghost)
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
    
    //initialise cells
    double i, j, k;
    int cell = 0;
    for (i = xlo - (xstep/2); i < xhi + xstep; i += xstep) {
        for (j = ylo - (ystep/2); j < yhi + ystep; j += ystep) {
            for (k = zlo - (zstep/2); k < zhi + zstep; k += zstep) {
                xCell[cell] = i;
                yCell[cell] = j;
                zCell[cell] = k;
                cellVol[cell] = xstep * ystep * zstep;
                ghost[cell] = false;
                //Initialise concentration values for ghost and std cells
                if (i < xlo || i > xhi || j < ylo ||
                    j > yhi || k < zlo || k > zhi) {
                    ghost[cell] = true;
                    
                    subCell[cell] = subBC;
                    o2Cell[cell] = o2BC;
                    no2Cell[cell] = no2BC;
                    no3Cell[cell] = no3BC;
                    nh4Cell[cell] = nh4BC;
                }else{
                    subCell[cell] = initsub;
                    o2Cell[cell] = inito2;
                    no2Cell[cell] = initno2;
                    no3Cell[cell] = initno3;
                    nh4Cell[cell] = initnh4;
                }
                cell++;
            }
        }
    }
    
    //create folders for concentration data
    int status1, status2;
    
    status1 = system("rm -rf sub o2 no2 no3 nh4 r-values");
    status2 = system("mkdir sub o2 no2 no3 nh4 r-values");
    
    if(status1 < 0 || status2 < 0){
        error->all(FLERR,"Fail to create concentration output dir");
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
    clock_t t0 = clock();
    
    modify->clearstep_compute();
    
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
    
    int* cellIn = new int[nall];
    // By using double[numCells]() this calls the constructor and ensures values are zero
    double* xHET = new double[numCells]();
    double* xAOB = new double[numCells]();
    double* xNOB = new double[numCells]();
    double* xEPS = new double[numCells]();
    double* xDEAD = new double[numCells]();
    double* xTot = new double[numCells]();
    
    //Growth
    double* R1 = new double[numCells];
    double* R2 = new double[numCells];
    double* R3 = new double[numCells];
    double* R4 = new double[numCells];
    double* R5 = new double[numCells];
    
    //Decay and maintenance
    double* R6 = new double[numCells];
    double* R7 = new double[numCells];
    double* R8 = new double[numCells];
    double* R9 = new double[numCells];
    double* R10 = new double[numCells];
    double* R11 = new double[numCells];
    double* R12 = new double[numCells];
    double* R13 = new double[numCells];
    double* R14 = new double[numCells];
    
    double* Rs = new double[numCells];
    double* Ro2 = new double[numCells];
    double* Rnh4 = new double[numCells];
    double* Rno2 = new double[numCells];
    double* Rno3 = new double[numCells];
    
    double* cellDo2 = new double[numCells];
    double* cellDnh4 = new double[numCells];
    double* cellDno2 = new double[numCells];
    double* cellDno3  = new double[numCells];
    double* cellDs  = new double[numCells];
    
    
    clock_t t1 = clock();
    //AtomVec *avec = atom->avec;
    int grid = 0;
    // Figure out which cell each particle is in
    for (i = 0; i < nall; i++) {
        if (mask[i] & groupbit) {
            //printf("nlocal = %i, natom = %f, nghost=%i \n",i, atom->x[i][0], atom->nghost);
            double gHET = 0;
            double gAOB = 0;
            double gNOB = 0;
            double gEPS = 0;
            double gDEAD = 0;
            switch (type[i]) {
                case 1:
                    gHET = 1;
                    break;
                case 2:
                    gAOB = 1;
                    break;
                case 3:
                    gNOB = 1;
                    break;
                case 4:
                    gEPS = 1;
                    break;
                case 6:
                    gDEAD = 1;
            }
            
            int xpos = (atom->x[i][0] - xlo) / xstep + 1;
            int ypos = (atom->x[i][1] - ylo) / ystep + 1;
            int zpos = (atom->x[i][2] - zlo) / zstep + 1;
            int pos = (xpos * (nx+2) + ypos) * (ny+2) + zpos;
            
            if (pos >= numCells) {
                printf("Too big! pos=%d   size = %lu\n", pos, sizeof(xHET));
            }
            cellIn[i] = pos;
            double rmassCellVol = rmass[i]/cellVol[pos];
            xHET[pos] += gHET * rmassCellVol;
            xAOB[pos] += gAOB * rmassCellVol;
            //  xNOB[j] += rmass[i]/cellVol[j];
            xNOB[pos] += gNOB *rmassCellVol;
            xEPS[pos] += gEPS * rmassCellVol;
            xDEAD[pos] += gDEAD * rmassCellVol;
            xTot[pos] += rmassCellVol;
        }
    }
    
    clock_t t2 = clock();
    
    //initialize values
    for (int cell = 0; cell < numCells; cell++) {
        //Growth
        R1[cell] = MumHET*(subCell[cell]/(KsHET+subCell[cell]))*(o2Cell[cell]/(Ko2HET+o2Cell[cell]));
        R2[cell] = MumAOB*(nh4Cell[cell]/(Knh4AOB+nh4Cell[cell]))*(o2Cell[cell]/(Ko2AOB+o2Cell[cell]));
        R3[cell] = MumNOB*(no2Cell[cell]/(Kno2NOB+no2Cell[cell]))*(o2Cell[cell]/(Ko2NOB+o2Cell[cell]));
        R4[cell] = etaHET*MumHET*(subCell[cell]/(KsHET+subCell[cell]))*(no3Cell[cell]/(Kno3HET+no3Cell[cell]))*(Ko2HET/(Ko2HET+o2Cell[cell]));
        R5[cell] = etaHET*MumHET*(subCell[cell]/(KsHET+subCell[cell]))*(no2Cell[cell]/(Kno2HET+no2Cell[cell]))*(Ko2HET/(Ko2HET+o2Cell[cell]));
        
        //Decay and maintenance;
        R6[cell] = bHET;
        R7[cell] = bAOB;
        R8[cell] = bNOB;
        R9[cell] = bEPS;
        
        R10[cell] = bmHET*(o2Cell[cell]/(Ko2HET+o2Cell[cell]));
        R11[cell] = bmAOB*(o2Cell[cell]/(Ko2AOB+o2Cell[cell]));
        R12[cell] = bmNOB*(o2Cell[cell]/(Ko2NOB+o2Cell[cell]));
        R13[cell] = (1/2.86)*bmHET*etaHET*(no3Cell[cell]/(Kno3HET+no3Cell[cell]))*(Ko2HET/(Ko2HET+o2Cell[cell]));
        R14[cell] = (1/1.71)*bmHET*etaHET*(no2Cell[cell]/(Kno2HET+no2Cell[cell]))*(Ko2HET/(Ko2HET+o2Cell[cell]));
        
        if(!(update->ntimestep % diffevery)){
            Rs[cell] = ( (-1/YHET) * ( (R1[cell]+R4[cell]+R5[cell]) * xHET[cell] ) ) + R6[cell]*xHET[cell] + R7[cell]*xAOB[cell] + R8[cell]*xNOB[cell] + R9[cell]*xEPS[cell];
            Ro2[cell] = (-((1-YHET-YEPS)/YHET)*R1[cell]*xHET[cell])-(((3.42-YAOB)/YAOB)*R2[cell]*xAOB[cell])-(((1.15-YNOB)/YNOB)*R3[cell]*xNOB[cell]);
            Rnh4[cell] = -(1/YAOB)*R2[cell]*xAOB[cell];
            Rno2[cell] = ((1/YAOB)*R2[cell]*xAOB[cell])-((1/YNOB)*R3[cell]*xNOB[cell])-(((1-YHET-YEPS)/(1.17*YHET))*R5[cell]*xHET[cell]);
            Rno3[cell] = ((1/YNOB)*R3[cell]*xNOB[cell])-(((1-YHET-YEPS)/(2.86*YHET))*R4[cell]*xHET[cell]);
            //Decay and maintenance
            Rs[cell] += (bX * xDEAD[cell]);
            Ro2[cell] = Ro2[cell] - ((R10[cell] * xHET[cell]) + (R11[cell] * xAOB[cell]) + (R12[cell] * xNOB[cell]));
            Rno2[cell] = Rno2[cell] - (R14[cell] * xHET[cell]);
            Rno3[cell] = Rno3[cell] - (R13[cell] * xHET[cell]);
            
            //			 printf("cell= %e, rs=%e, ro2=%e, rnh4=%e, Rno2=%e, forw=%e, Rno3=%e,  \n",cell, Rs[cell],
            //					 Ro2[cell] , Rnh4[cell], Rno2[cell], Rno3[cell]);
            
            //						 printf("cell= %i, rs=%e, \n",cell, xDEAD[cell]);
            
            double diffusionFunction = 1 - ((0.43 * pow(xTot[cell], 0.92))/(11.19+0.27*pow(xTot[cell], 0.99)));
            
            cellDo2[cell] = diffusionFunction * Do2;
            cellDnh4[cell] = diffusionFunction * Dnh4;
            cellDno2[cell] = diffusionFunction * Dno2;
            cellDno3[cell] = diffusionFunction * Dno3;
            cellDs[cell] = diffusionFunction * Ds;
        }
    }
    
    clock_t t3 = clock();
    
    if(!(update->ntimestep % diffevery)) {
        
        double* subPrev  = new double[numCells];
        double* subCellCurrent = subCell;
        double* subCellLast = subPrev;
        double* o2Prev  = new double[numCells];
        double* o2CellCurrent = o2Cell;
        double* o2CellLast = o2Prev;
        double* no2Prev  = new double[numCells];
        double* no2CellCurrent = no2Cell;
        double* no2CellLast = no2Prev;
        double* no3Prev  = new double[numCells];
        double* no3CellCurrent = no3Cell;
        double* no3CellLast = no3Prev;
        double* nh4Prev  = new double[numCells];
        double* nh4CellCurrent = nh4Cell;
        double* nh4CellLast = nh4Prev;
        
        bool subConvergence = false;
        bool o2Convergence = false;
        bool no2Convergence = false;
        bool no3Convergence = false;
        bool nh4Convergence = false;
        
        bool convergence = false;
        
        // Copy onece only current to last:
        for (int cell = 0; cell < numCells; cell++) {
            subCellLast[cell] = subCellCurrent[cell];
            o2CellLast[cell] = o2CellCurrent[cell];
            no2CellLast[cell] = no2CellCurrent[cell];
            no3CellLast[cell] = no3CellCurrent[cell];
            nh4CellLast[cell] = nh4CellCurrent[cell];
        }
        
        int iteration = 0;
        
        double tol = 1e-6; // Tolerance for convergence criteria for nutrient balance equation
        
        // Outermost while loop for the convergence criterion
        while (!convergence) {
            iteration ++;
            
            //for (int cell = 0; cell < numCells; cell++) {
            //sub Prev[cell] = sub Cell[cell];
            //o2 Prev[cell] = o2 Cell[cell];
            //nh4Prev[cell] = nh4Cell[cell];
            //no2Prev[cell] = no2Cell[cell];
            //no3 Prev[cell] = no3 Cell[cell];
            //}
            
            // Swap subCellCurrent to point at the other one
            if (subCellCurrent == subCell) {
                subCellCurrent = subPrev;
                subCellLast = subCell;
                
                o2CellCurrent = o2Prev;
                o2CellLast = o2Cell;
                
                no2CellCurrent = no2Prev;
                no2CellLast = no2Cell;
                
                no3CellCurrent = no3Prev;
                no3CellLast = no3Cell;
                
                nh4CellCurrent = nh4Prev;
                nh4CellLast = nh4Cell;
            }
            else {
                subCellCurrent = subCell;
                subCellLast = subPrev;
                
                o2CellCurrent = o2Cell;
                o2CellLast = o2Prev;
                
                no2CellCurrent = no2Cell;
                no2CellLast = no2Prev;
                
                no3CellCurrent = no3Cell;
                no3CellLast = no3Prev;
                
                nh4CellCurrent = nh4Cell;
                nh4CellLast = nh4Prev;
            }
            
            for (int cell = 0; cell < numCells; cell++) {
                R1[cell] = MumHET*(subCellLast[cell]/(KsHET+subCellLast[cell]))*(o2CellLast[cell]/(Ko2HET+o2CellLast[cell]));
                R2[cell] = MumAOB*(nh4CellLast[cell]/(Knh4AOB+nh4CellLast[cell]))*(o2CellLast[cell]/(Ko2AOB+o2CellLast[cell]));
                R3[cell] = MumNOB*(no2CellLast[cell]/(Kno2NOB+no2CellLast[cell]))*(o2CellLast[cell]/(Ko2NOB+o2CellLast[cell]));
                R4[cell] = etaHET*MumHET*(subCellLast[cell]/(KsHET+subCellLast[cell]))*(no3CellLast[cell]/(Kno3HET+no3CellLast[cell]))*(Ko2HET/(Ko2HET+o2CellLast[cell]));
                R5[cell] = etaHET*MumHET*(subCellLast[cell]/(KsHET+subCellLast[cell]))*(no2CellLast[cell]/(Kno2HET+no2CellLast[cell]))*(Ko2HET/(Ko2HET+o2CellLast[cell]));
                //Decay and maintenance
                R10[cell] = bmHET*(o2CellLast[cell]/(Ko2HET+o2CellLast[cell]));
                R11[cell] = bmAOB*(o2CellLast[cell]/(Ko2AOB+o2CellLast[cell]));
                R12[cell] = bmNOB*(o2CellLast[cell]/(Ko2NOB+o2CellLast[cell]));
                R13[cell] = (1/2.86)*bmHET*etaHET*(no3CellLast[cell]/(Kno3HET+no3CellLast[cell]))*(Ko2HET/(Ko2HET+o2CellLast[cell]));
                R14[cell] = (1/1.71)*bmHET*etaHET*(no2CellLast[cell]/(Kno2HET+no2CellLast[cell]))*(Ko2HET/(Ko2HET+o2CellLast[cell]));
                
                Rs[cell] = ( (-1/YHET) * ( (R1[cell]+R4[cell]+R5[cell]) * xHET[cell] ) ) + R6[cell]*xHET[cell] + R7[cell]*xAOB[cell] + R8[cell]*xNOB[cell] + R9[cell]*xEPS[cell];
                Ro2[cell] = (-((1-YHET-YEPS)/YHET)*R1[cell]*xHET[cell])-(((3.42-YAOB)/YAOB)*R2[cell]*xAOB[cell])-(((1.15-YNOB)/YNOB)*R3[cell]*xNOB[cell]);
                Rnh4[cell] = -(1/YAOB)*R2[cell]*xAOB[cell];
                Rno2[cell] = ((1/YAOB)*R2[cell]*xAOB[cell])-((1/YNOB)*R3[cell]*xNOB[cell])-(((1-YHET-YEPS)/(1.17*YHET))*R5[cell]*xHET[cell]);
                Rno3[cell] = ((1/YNOB)*R3[cell]*xNOB[cell])-(((1-YHET-YEPS)/(2.86*YHET))*R4[cell]*xHET[cell]);
                //Decay and maintenance
                Rs[cell] +=  (bX * xDEAD[cell]);
                Ro2[cell] = Ro2[cell] - ((R10[cell] * xHET[cell]) + (R11[cell] * xAOB[cell]) + (R12[cell] * xNOB[cell]));
                Rno2[cell] = Rno2[cell] - (R14[cell] * xHET[cell]);
                Rno3[cell] = Rno3[cell] -(R13[cell] * xHET[cell]);
                
                if(!subConvergence) compute_flux(cellDs, subCellCurrent, subCellLast, subBC, Rs[cell], diffT, cell);
                if(!o2Convergence) compute_flux(cellDo2, o2CellCurrent, o2CellLast, o2BC, Ro2[cell], diffT, cell);
                if(!nh4Convergence) compute_flux(cellDnh4, nh4CellCurrent, nh4CellLast, nh4BC, Rnh4[cell], diffT, cell);
                if(!no2Convergence) compute_flux(cellDno2, no2CellCurrent, no2CellLast, no2BC, Rno2[cell], diffT, cell);
                if(!no3Convergence) compute_flux(cellDno3, no3CellCurrent, no3CellLast, no3BC, Rno3[cell], diffT, cell);
            }
            
            if(is_convergence(subCellCurrent, subCellLast, subBC, tol))	subConvergence = true;
            if(is_convergence(o2CellCurrent, o2CellLast, o2BC, tol))	o2Convergence = true;
            if(is_convergence(nh4CellCurrent, nh4CellLast, nh4BC, tol)) nh4Convergence = true;
            if(is_convergence(no2CellCurrent, no2CellLast, no2BC, tol)) no2Convergence = true;
            if(is_convergence(no3CellCurrent, no3CellLast, no3BC, tol)) no3Convergence = true;
            
            if((subConvergence && o2Convergence && nh4Convergence && no2Convergence && no3Convergence) || iteration == 10000) {
                convergence = true;
            }
        }
        
        fprintf(stdout, "Number of iterations:  %i {%d}\n", iteration, numCells);
        
        
        // Fix the two copies of subCell so that subCell contains the value
        if (subCell != subCellCurrent) {
            for (int cell = 0; cell < numCells; cell++) {
                subCell[cell] = subCellCurrent[cell];
                o2Cell[cell] = o2CellCurrent[cell];
                no2Cell[cell] = no2CellCurrent[cell];
                no3Cell[cell] = no3CellCurrent[cell];
                nh4Cell[cell] = nh4CellCurrent[cell];
            }
        }
        
        delete [] subPrev;
        delete [] o2Prev;
        delete [] nh4Prev;
        delete [] no2Prev;
        delete [] no3Prev;
    }
    
    clock_t t4 = clock();
    
    const double threeQuartersPI = (3.0/(4.0*MY_PI));
    const double fourThirdsPI = 4.0*MY_PI/3.0;
    const double third = 1.0/3.0;

//#define one
#ifdef one
    for (i = 0; i < nall; i++) {
        if (mask[i] & groupbit) {
            double value = 0;
            double value2 = 0;
            switch (type[i]) {
                case 1:
                    value =   R1[cellIn[i]] + R4[cellIn[i]] + R5[cellIn[i]] - R6[cellIn[i]] - R10[cellIn[i]] - R13[cellIn[i]] - R14[cellIn[i]];
                    value2 = (R1[cellIn[i]] + R4[cellIn[i]] + R5[cellIn[i]]) * update->dt * (YEPS/YHET);
                    break;
                case 2:
                    value = (R2[cellIn[i]]-R7[cellIn[i]]-R11[cellIn[i]]);
                    break;
                case 3:
                    value = (R3[cellIn[i]]-R8[cellIn[i]]-R12[cellIn[i]]);
                    break;
                case 4:
                    value = bEPS;
                    break;
                case 6:
                    value = bX;
            }
            
            sub[i] = subCell[cellIn[i]];
            o2[i] = o2Cell[cellIn[i]];
            nh4[i] = nh4Cell[cellIn[i]];
            no2[i] = no2Cell[cellIn[i]];
            no3[i] = no3Cell[cellIn[i]];
            
            value *= update->dt;
            
            density = rmass[i] / (fourThirdsPI * radius[i]*radius[i]*radius[i]);
            rmass[i] = rmass[i]*(1 + (value*nevery));
            
            if (type[i] == 1) {
                outerMass[i] = fourThirdsPI * (outerRadius[i] * outerRadius[i] * outerRadius[i] - radius[i] * radius[i] * radius[i]) * EPSdens
                + value2 * nevery * rmass[i];
                
                outerRadius[i] = pow(threeQuartersPI * (rmass[i] / density + outerMass[i] / EPSdens), third);
                radius[i] = pow(threeQuartersPI * rmass[i] / density, third);
            }
            else {
                radius[i] = pow(threeQuartersPI * rmass[i] / density, third);
                outerMass[i] = 0.0;
                outerRadius[i] = radius[i];
            }
        }
    }
#endif
#ifndef one
    // build up arrays of cells
    double* caseOneOne = new double[numCells];
    double* caseOneTwo = new double[numCells];
    double* caseTwo = new double[numCells];
    double* caseThree = new double[numCells];
    
    for (int j = 0; j < numCells; j++) {
        caseOneOne[j] = (R1[j] + R4[j] + R5[j] - R6[j] - R10[j] - R13[j] - R14[j]) * update->dt;
        caseOneTwo[j] = (R1[i] + R4[i] + R5[i]) * update->dt * (YEPS/YHET);
        caseTwo[j] = (R2[j] - R7[j] - R11[j]) * update->dt;
        caseThree[j] = (R3[j] - R8[j] - R12[j]) * update->dt;
    }
    
    for (i = 0; i < nall; i++) {
        if (mask[i] & groupbit) {
            double value = 0;
            double value2 = 0;
            if (cellIn[i] >= numCells) {
                printf("PANIC! outside of range!\n");
                exit(44);
            }
            switch (type[i]) {
                case 1:
                    value = caseOneOne[cellIn[i]];
                    value2 = caseOneTwo[cellIn[i]];
                    break;
                case 2:
                    value = caseTwo[cellIn[i]];
                    break;
                case 3:
                    value = caseThree[cellIn[i]];
                    break;
                case 4:
                    value = bEPS;
                    break;
                case 6:
                    value = bX;
            }
            
            sub[i] = subCell[cellIn[i]];
            o2[i] = o2Cell[cellIn[i]];
            nh4[i] = nh4Cell[cellIn[i]];
            no2[i] = no2Cell[cellIn[i]];
            no3[i] = no3Cell[cellIn[i]];
            
            density = rmass[i] / (fourThirdsPI * radius[i]*radius[i]*radius[i]);
            rmass[i] = rmass[i]*(1 + (value*nevery));
            
            if (type[i] == 1) {
                outerMass[i] = fourThirdsPI * (outerRadius[i] * outerRadius[i] * outerRadius[i] - radius[i] * radius[i] * radius[i]) * EPSdens
                + value2 * nevery * rmass[i];
                
                outerRadius[i] = pow(threeQuartersPI * (rmass[i] / density + outerMass[i] / EPSdens), third);
                radius[i] = pow(threeQuartersPI * rmass[i] / density, third);
            }
            else {
                radius[i] = pow(threeQuartersPI * rmass[i] / density, third);
                outerMass[i] = 0.0;
                outerRadius[i] = radius[i];
            }
        }
    }
    
    delete [] caseOneOne;
    delete [] caseOneTwo;
    delete [] caseTwo;
    delete [] caseThree;
#endif

    clock_t t5 = clock();
    
    //output data
    if(!(update->ntimestep % outputevery)){
        output_data(outputevery,1);
        output_data(outputevery,2);
        output_data(outputevery,3);
        output_data(outputevery,4);
        output_data(outputevery,5);
        compute_Rvalues(Rs, Ro2, Rno2, Rno3, Rnh4);
        output_data(outputevery,6);
    }
    
    modify->addstep_compute(update->ntimestep + nevery);
    
    delete [] cellIn;
    delete [] xHET;
    delete [] xAOB;
    delete [] xNOB;
    delete [] xEPS;
    delete [] xDEAD;
    delete [] xTot;
    
    delete [] R1;
    delete [] R2;
    delete [] R3;
    delete [] R4;
    delete [] R5;
    delete [] R6;
    delete [] R7;
    delete [] R8;
    delete [] R9;
    delete [] R10;
    delete [] R11;
    delete [] R12;
    delete [] R13;
    delete [] R14;
    
    delete [] Rs;
    delete [] Ro2;
    delete [] Rnh4;
    delete [] Rno3;
    delete [] Rno2;
    
    delete [] cellDo2;
    delete [] cellDnh4;
    delete [] cellDno2;
    delete [] cellDno3;
    delete [] cellDs;
    
    clock_t t6 = clock();
    double t01 = (double) (t1-t0) / CLOCKS_PER_SEC * 1000.0;
    double t12 = (double) (t2-t1) / CLOCKS_PER_SEC * 1000.0;
    double t23 = (double) (t3-t2) / CLOCKS_PER_SEC * 1000.0;
    double t34 = (double) (t4-t3) / CLOCKS_PER_SEC * 1000.0;
    double t45 = (double) (t5-t4) / CLOCKS_PER_SEC * 1000.0;
    double t56 = (double) (t6-t5) / CLOCKS_PER_SEC * 1000.0;
    
    double totalT = 100/(t01 + t12 + t23 + t34 + t45 + t56);
    //fprintf(stdout, "t01 = %f  t12 = %f  t23 = %f  t34 = %f  t45 = %f   t56 = %f\n", t01*totalT, t12*totalT, t23*totalT, t34*totalT, t45*totalT, t56*totalT);
    //fprintf(stdout, "##, %f, %f, %f, %f, %f, %f\n", t01*totalT, t12*totalT, t23*totalT, t34*totalT, t45*totalT, t56*totalT);
}

bool FixDiffNuGrowth::is_convergence(double *nuCell, double *prevNuCell, double nuBC, double tol) {
    for(int cell = 0; cell < numCells; cell++){
        if(!ghost[cell]){
            
            double rate = nuCell[cell]/nuBC;
            double prevRate = prevNuCell[cell]/nuBC;
            
            if(fabs(rate - prevRate) >= tol) return false;
        }
    }
    return true;
}

void FixDiffNuGrowth::compute_flux(double *cellDNu, double *nuCell, double *nuPrev, double nuBC, double rateNu, double diffT, int cell) {
    // fprintf(stdout, "nuCell= %f, nuPrev = %f, rateNu = %f\n", nuCell[cell], nuPrev[cell], rateNu);
    //for nx = ny = nz = 1 grids
    //2  11  20 			5  14  23				8  17  26
    //1  10  19       4  13  22       7  16  25
    //0  9   18       3  12  21       6  15  24
    int leftCell = cell - (nz+2)*(ny+2); // x direction
    int rightCell = cell + (nz+2)*(ny+2); // x direction
    int downCell = cell - (nz+2); // y direction
    int upCell = cell + (nz+2); // y direction
    int backwardCell = cell - 1; // z direction
    int forwardCell = cell + 1; // z direction
    
    // assign values to the ghost-cells according to the boundary conditions.
    // If ghostcells are Neu then take the values equal from the adjacent cells.
    // if ghostcells are dirich then take the values equal to negative of the adjacent cells.
    // if ghostcells are mixed then zlo ghost cells are nuemann, zhi ghost cells are dirichlet, other four surfaces are periodic BC.
    if (ghost[cell]) {
        // fprintf(stdout, "Ghost Cell: %i\n", cell);
        if (zCell[cell] < zlo && !ghost[forwardCell]) {
            if (bflag == 1) {
                nuCell[cell] = 2*nuBC - nuPrev[forwardCell];
            } else if(bflag == 2 || bflag == 3) {
                nuCell[cell] = nuPrev[forwardCell];
            }
        }
        else if (zCell[cell] > zhi && !ghost[backwardCell]) {
            if (bflag == 1 || bflag == 3) {
                nuCell[cell] = 2*nuBC - nuPrev[backwardCell];
            } else if (bflag == 2) {
                nuCell[cell] = nuPrev[backwardCell];
            }
        }
        else if (yCell[cell] < ylo && !ghost[upCell]) {
            if (bflag == 1) {
                nuCell[cell] = 2*nuBC - nuPrev[upCell];
            } else if (bflag == 2) {
                nuCell[cell] = nuPrev[upCell];
            } else if (bflag == 3) {
                int yhiCell = cell + (nz+2)*ny;
                nuCell[cell] = nuPrev[yhiCell];
            }
        }
        else if (yCell[cell] > yhi && !ghost[downCell]) {
            if (bflag == 1) {
                nuCell[cell] = 2*nuBC - nuPrev[downCell];
            } else if (bflag == 2) {
                nuCell[cell] = nuPrev[downCell];
            } else if (bflag == 3) {
                int yloCell = cell - (nz+2)*ny;
                nuCell[cell] = nuPrev[yloCell];
            }
        }
        else if (xCell[cell] < xlo && !ghost[rightCell]) {
            if (bflag == 1) {
                nuCell[cell] = 2*nuBC - nuPrev[rightCell];
            } else if (bflag == 2) {
                nuCell[cell] = nuPrev[rightCell];
            } else if (bflag == 3) {
                int xhiCell = cell + (ny+2)*(nz+2)*nx;
                nuCell[cell] = nuPrev[xhiCell];
            }
        }
        else if (xCell[cell] > xhi && !ghost[leftCell]) {
            if (bflag == 1) {
                nuCell[cell] = 2*nuBC - nuPrev[leftCell];
            } else if (bflag == 2) {
                nuCell[cell] = nuPrev[leftCell];
            } else if (bflag == 3) {
                int xloCell = cell - (ny+2)*(nz+2)*nx;
                nuCell[cell] = nuPrev[xloCell];
            }
        }
    }
    else {
        
        double dRight = (cellDNu[cell] + cellDNu[rightCell]) / 2;
        double jRight = dRight*(nuPrev[rightCell] - nuPrev[cell])/xstep;
        double dLeft = (cellDNu[cell] + cellDNu[leftCell]) / 2;
        double jLeft = dLeft*(nuPrev[cell] - nuPrev[leftCell])/xstep;
        double jX = (jRight - jLeft)/xstep;
        
        double dUp = (cellDNu[cell] + cellDNu[upCell]) / 2;
        double jUp = dUp*(nuPrev[upCell] - nuPrev[cell])/ystep;
        double dDown = (cellDNu[cell] + cellDNu[downCell]) / 2;
        double jDown = dDown*(nuPrev[cell] - nuPrev[downCell])/ystep;
        double jY = (jUp - jDown)/ystep;
        
        double dForward = (cellDNu[cell] + cellDNu[forwardCell]) / 2;
        double jForward = dForward*(nuPrev[forwardCell] - nuPrev[cell])/zstep;
        double dBackward = (cellDNu[cell] + cellDNu[backwardCell]) / 2;
        double jBackward = dBackward*(nuPrev[cell] - nuPrev[backwardCell])/zstep;
        double jZ = (jForward - jBackward)/zstep;
        
        // Adding fluxes in all the directions and the uptake rate (RHS side of the equation)
        double Ratesub = jX + jY + jZ + rateNu;
        //Updating the value: Ratesub*diffT + nuCell[cell](previous)
        nuCell[cell] += Ratesub*diffT;
        
        // printf("cell= %i, right=%i, left=%i, up=%i, down=%i, forw=%i, back=%i,  \n",cell, rightCell, leftCell, upCell, downCell, forwardCell, backwardCell);
        if(nuCell[cell] <= 1e-20){
            nuCell[cell] = 1e-20;
        }
    }
}

void FixDiffNuGrowth::compute_Rvalues(double* Rs, double* Ro2, double* Rno2, double* Rno3, double* Rnh4){
    sumRs = 0.0;
    sumRo2 = 0.0;
    sumRno2 = 0.0;
    sumRno3 = 0.0;
    sumRnh4 = 0.0;
    
    for (int cell = 0; cell < numCells; cell++) {
        if(!ghost[cell]){
            sumRs += (Rs[cell] * cellVol[cell]);
            sumRo2 += (Ro2[cell] * cellVol[cell]);
            sumRno2 += (Rno2[cell] * cellVol[cell]);
            sumRno3 += (Rno3[cell] * cellVol[cell]);
            sumRnh4 += (Rnh4[cell] * cellVol[cell]);
        }
    }
}

void FixDiffNuGrowth::output_data(int every, int n){
    if(!(update->ntimestep%every)){
        FILE* pFile;
        std::string str;
        std::ostringstream stm;
        stm << update->ntimestep;
        
        switch(n) {
            case 1 :
                str = "./sub/sub.csv."+ stm.str();
                break;
            case 2 :
                str = "./o2/o2.csv."+ stm.str();
                break;
            case 3 :
                str = "./no2/no2.csv."+ stm.str();
                break;
            case 4 :
                str = "./no3/no3.csv."+ stm.str();
                break;
            case 5 :
                str = "./nh4/nh4.csv."+ stm.str();
                break;
            case 6 :
                str = "./r-values/r-values.txt";
                break;
        }
        
        if(n == 6){
            pFile = fopen (str.c_str(), "a");
            if(update->ntimestep == every) fprintf(pFile, "time \t sumRs \t sumRo2 \t sumRno2 \t sumRno3 \t sumRnh4 \n");
            fprintf(pFile, "%lli,\t%e,\t%e,\t%e,\t%e,\t%e\n",update->ntimestep, sumRs, sumRo2, sumRno2, sumRno3, sumRnh4);
        }else{
            pFile = fopen (str.c_str(), "w");
            fprintf(pFile, ",x,y,z,scalar,1,1,1,0.5\n");
            for(int i = 0; i < numCells; i++){
                if(!ghost[i]){
                    
                    switch(n) {
                        case 1 :
                            fprintf(pFile, "%i,\t%f,\t%f,\t%f,\t%f\n",i, xCell[i], yCell[i], zCell[i], subCell[i]);
                            break;
                        case 2 :
                            fprintf(pFile, "%i,\t%f,\t%f,\t%f,\t%f\n",i, xCell[i], yCell[i], zCell[i], o2Cell[i]);
                            break;
                        case 3 :
                            fprintf(pFile, "%i,\t%f,\t%f,\t%f,\t%f\n",i, xCell[i], yCell[i], zCell[i], no2Cell[i]);
                            break;
                        case 4 :
                            fprintf(pFile, "%i,\t%f,\t%f,\t%f,\t%f\n",i, xCell[i], yCell[i], zCell[i], no3Cell[i]);
                            break;
                        case 5 :
                            fprintf(pFile, "%i,\t%f,\t%f,\t%f,\t%f\n",i, xCell[i], yCell[i], zCell[i], nh4Cell[i]);
                            break;
                    }
                }
            }
        }
        fclose(pFile);
    }
}
