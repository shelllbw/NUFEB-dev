/*
 * fix_metabolism.h
 *
 *  Created on: 15 Aug 2016
 *      Author: bowen
 */

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

#include <fix_metabolism.h>
#include "math.h"
#include "string.h"
#include "stdlib.h"
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
#include "math_const.h"
#include "memory.h"
#include "error.h"
#include "comm.h"
#include "domain.h"
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <unordered_set>

using namespace LAMMPS_NS;
using namespace FixConst;
using namespace MathConst;

using namespace std;


/* ---------------------------------------------------------------------- */

FixMetabolism::FixMetabolism(LAMMPS *lmp, int narg, char **arg) : Fix(lmp, narg, arg)
{

  if (narg != 7) error->all(FLERR,"Not enough arguments in fix metabolism command");

  nevery = force->inumeric(FLERR,arg[3]);
  if (nevery < 0) error->all(FLERR,"Illegal fix metabolism command");

  var = new char*[3];
  ivar = new int[3];

  for (int i = 0; i < 3; i++) {
    int n = strlen(&arg[4+i][2]) + 1;
    var[i] = new char[n];
    strcpy(var[i],&arg[4+i][2]);
  }

}

/* ---------------------------------------------------------------------- */

FixMetabolism::~FixMetabolism()
{
  int i;
  for (i = 0; i < 3; i++) {
    delete [] var[i];
  }
  delete [] var;
  delete [] ivar;

  memory->destroy(metCoeff);
  memory->destroy(matConsume);
}

/* ---------------------------------------------------------------------- */

int FixMetabolism::setmask()
{
  int mask = 0;
  mask |= PRE_FORCE;
  return mask;
}

/* ---------------------------------------------------------------------- */

void FixMetabolism::init()
{
  if (!atom->radius_flag)
    error->all(FLERR,"Fix requires atom attribute diameter");

  for (int n = 0; n < 3; n++) {
    ivar[n] = input->variable->find(var[n]);
    if (ivar[n] < 0)
      error->all(FLERR,"Variable name for fix metabolism does not exist");
    if (!input->variable->equalstyle(ivar[n]))
      error->all(FLERR,"Variable for fix metabolism is invalid style");
  }

  temp = input->variable->compute_equal(ivar[0]);
  gasTran = input->variable->compute_equal(ivar[1]);
  gvol = input->variable->compute_equal(ivar[2]);

  nnus = atom->nNutrients;
  ntypes = atom->ntypes;
  catCoeff = atom->catCoeff;
  anabCoeff = atom->anabCoeff;
  yield = atom->yield;

  metCoeff_calculus();

  nx = domain->nx;
  ny = domain->ny;
  nz = domain->nz;

  ngrids = nx * ny * nz;

  //Get computational domain size
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

  stepx = (xhi - xlo) / nx;
  stepy = (yhi - ylo) / ny;
  stepz = (zhi - zlo) / nz;

  vol = stepx * stepy * stepz;
}

/* ----------------------------------------------------------------------
  calculate metabolic coefficient for all microbial species
------------------------------------------------------------------------- */

void FixMetabolism::metCoeff_calculus()
{
  metCoeff = memory->create(metCoeff,ntypes+1,nnus+1,"atom:metCoeff");
  matConsume = memory->create(matConsume,ntypes+1,nnus+1,"atom:matCons");

  for (int i = 1; i <= ntypes; i++) {
    for (int j = 1; j <= nnus; j++) {
     // cout << "type = " << atom->nuType[j] << endl;
      if (strcmp(atom->nuName[j], "h") == 0 || strcmp(atom->nuName[j], "h2o") == 0) {
        metCoeff[i][j] = 0;
        matConsume[i][j] = 0;
        continue;
      }
      double pThY = 1/yield[i];
      double coeff = catCoeff[i][j] * pThY + anabCoeff[i][j];
      metCoeff[i][j] = coeff;
      if (coeff < 0) {
        matConsume[i][j] = 1;
      } else {
        matConsume[i][j] = 0;
      }
    }
  }
}

/* ---------------------------------------------------------------------- */

void FixMetabolism::pre_force(int vflag)
{
  if (nevery == 0) return;
  if (update->ntimestep % nevery) return;

  metabolism();
}

void FixMetabolism::metabolism()
{
  int *mask = atom->mask;
  int nlocal = atom->nlocal;
  int nall = nlocal + atom->nghost;
  int* type = atom->type;
  unordered_set<int> posInds;
  nuS = atom->nuS;
  nuR = atom->nuR;

  double *radius = atom->radius;
  double *rmass = atom->rmass;
  double *outerMass = atom->outerMass;
  double *outerRadius = atom->outerRadius;

  double** biomass = new double*[ntypes+1];
  double** minMonod = new double*[ntypes+1];
  int* bacIn = new int[nall]();
  double* growthRate = new double[nall]();

  //initialization
  for (int i = 0; i <= ntypes; i++) {
    minMonod[i] = new double[ngrids];
    biomass[i] = new double[ngrids]();
    for (int j = 0; j < ngrids; j++) {
      minMonod[i][j] = -1;
    }
  }

  for (int i = 0; i <= nnus; i++) {
    for (int j = 0; j < ngrids; j++) {
      nuR[i][j] = 0;
    }
  }

  for (int i = 0; i < nall; i++) {
    if (mask[i] & groupbit) {
      // get index of grid containing atom
      int xpos = (atom->x[i][0] - xlo) / stepx + 1;
      int ypos = (atom->x[i][1] - ylo) / stepy + 1;
      int zpos = (atom->x[i][2] - zlo) / stepz + 1;
      int pos = (xpos - 1) + (ypos - 1) * ny + (zpos - 1) * (nx * ny);

      if (pos >= ngrids) {
         printf("Too big! pos=%d   size = %i\n", pos, ngrids);
      }
      //save non-empty grid
      posInds.insert(pos);
      bacIn[i] = pos;

      //calculate growth rate
      int t = type[i];
      double monod = minMonod[t][pos];
      if (monod < 0) {
        minMonod[t][pos] = minimal_monod(pos, t);
        growthRate[i] = atom->growth[t] * minMonod[t][pos];
      } else {
        growthRate[i] = atom->growth[t] * monod;
      }
      //cout << "monod = " << minMonod[t][pos] << endl;
      //calculate amount of biomass formed
      biomass[t][pos] +=  growthRate[i] * atom->rmass[i];
   //   cout << atom->growth[t] << " ";
    }
  }

  for (const int& pos: posInds) {
    for (int i = 1; i <= nnus; i++) {
      //total consumption of all types per grid
      for (int t = 1; t <= ntypes; t++) {
        double consume = metCoeff[t][i] * biomass[t][pos];
        //cout << "name "<< atom->nuName[i] << " metCoeff" << metCoeff[t][i] << endl;
        if (atom->nuType[i] == 0) {
          //amount of substrate consumed
          nuR[i][pos] += consume;
        } else if(atom->nuType[i] == 1) {
          //calculate liquid concentrations
          double sLiq = consume/(vol*1000);
          nuR[i][pos] += sLiq;
        } else if (atom->nuType[i] == 2) {
          // calculate gas partial pressures
          double pGas = consume*gasTran*temp/gvol;
          nuR[i][pos] += pGas;
        }
      }
    }
  }
//
//  for (int i = 1; i <= ntypes; i++) {
//    cout << atom->typeName[i] << endl;
//    if (atom->nuType[i] == 0){
//      for (int j = 0; j < ngrids; j++) {
//        if (biomass[i][j] > 0.0001)
//          cout << biomass[i][j] << " ";
//      }
//    }
//    cout << endl;
//  }

  double density;
  const double threeQuartersPI = (3.0/(4.0*MY_PI));
  const double fourThirdsPI = 4.0*MY_PI/3.0;
  const double third = 1.0/3.0;

  //update mass and radius
  for (int i = 0; i < nall; i++) {
    if (mask[i] & groupbit) {
      double value = growthRate[i] * update->dt;

      rmass[i] = rmass[i] * (1 + (value * nevery));
      density = rmass[i] / (fourThirdsPI * radius[i]*radius[i]*radius[i]);
      radius[i] = pow(threeQuartersPI * (rmass[i] / density), third);
    }
  }

  for (int i = 0; i <= ntypes; i++) {
    delete [] biomass[i];
    delete [] minMonod[i];
  }

  delete [] biomass;
  delete [] minMonod;
  delete [] bacIn;
  delete [] growthRate;
}

double FixMetabolism::minimal_monod(int pos, int type)
{
  vector<double> mon;
  int size = 0;

  for (int i = 1; i <= nnus; i++ ) {
    if (matConsume[type][i] != 0 && atom->nuType[i] == 0) {
      double v = nuS[i][pos]/(atom->ks[type] + nuS[i][pos]);
      mon.push_back(v);
      size++;
      cout << nuS[i][pos] << " ";
    }
  }
  double min = *min_element(mon.begin(), mon.end());
  return min;
}
