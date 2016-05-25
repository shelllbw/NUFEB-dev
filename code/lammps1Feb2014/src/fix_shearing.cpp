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

#include "string.h"
#include "stdlib.h"
#include "fix_shearing.h"
#include "atom.h"
#include "update.h"
#include "domain.h"
#include "respa.h"
#include "error.h"
#include "force.h"
#include "math.h"
#include "mpi.h"
#include "comm.h"
#include "memory.h"
#include "input.h"
#include "variable.h"
#include "math_const.h"

using namespace LAMMPS_NS;
using namespace FixConst;
using namespace MathConst;

/* ---------------------------------------------------------------------- */

FixShearing::FixShearing(LAMMPS *lmp, int narg, char **arg) :
  Fix(lmp, narg, arg)
{
  if (narg != 9) error->all(FLERR,"Illegal fix shearing command");

  tmin = force->inumeric(FLERR,arg[7]);
  tmax = force->inumeric(FLERR,arg[8]);
  nevery = force->inumeric(FLERR,arg[3]);
  if (nevery < 0 || tmin < 0 || tmax < 0) error->all(FLERR,"Illegal fix shearing command: calling steps should be positive integer");
  if (tmin > tmax) error->all(FLERR, "Illegal fix shearing command: maximal calling step should be greater than minimal calling step");

  if(strcmp(arg[6], "zx") == 0) dflag = 1;
  else if(strcmp(arg[6], "zy") == 0) dflag = 2;
  else error->all(FLERR,"Illegal force vector command");

  var = new char*[2];
  ivar = new int[2];

  for (int i = 0; i < 2; i++) {
    int n = strlen(&arg[4+i][2]) + 1;
    var[i] = new char[n];
    strcpy(var[i],&arg[4+i][2]);
  }
}

FixShearing::~FixShearing()
{
  for (int i = 0; i < 2; i++) {
    delete [] var[i];
  }
  delete [] var;
  delete [] ivar;
}

/* ---------------------------------------------------------------------- */

int FixShearing::setmask()
{
  int mask = 0;
  mask |= POST_FORCE;
  return mask;
}

/* ---------------------------------------------------------------------- */

void FixShearing::init()
{
  for (int i = 0; i < 2; i++) {
    ivar[i] = input->variable->find(var[i]);
    if (ivar[i] < 0)
      error->all(FLERR,"Variable name for fix shearing does not exist");
    if (!input->variable->equalstyle(ivar[i]))
      error->all(FLERR,"Variable for fix shearing is invalid style");
  }
}

/* ---------------------------------------------------------------------- */

void FixShearing::post_force(int vflag)
{
  if (nevery == 0) return;
  if (update->ntimestep % nevery) return;
  if (update->ntimestep < tmin) return;
  if (update->ntimestep > tmax) return;

  double **f = atom->f;
  double **x = atom->x;
  double *radius = atom->radius;
  double viscosity = input->variable->compute_equal(ivar[0]);
  double shearRate = input->variable->compute_equal(ivar[1]);

  //argument test:
  //printf("nevery=%i, tmin=%i, tmax=%i, viscosity=%f, rate=%f \n", nevery, tmin, tmax, viscosity, shearRate);

  int nlocal = atom->nlocal;
  int nall = nlocal + atom->nghost;

  for (int i = 0; i < nall; i++) {
  	double diameter = 2 * radius[i];
  	if (dflag == 1) {
  		f[i][0] = MY_3PI * viscosity * diameter * shearRate * x[i][2];
  	} else {
  		f[i][1] = MY_3PI * viscosity * diameter * shearRate * x[i][2];
  	}
  }
}
