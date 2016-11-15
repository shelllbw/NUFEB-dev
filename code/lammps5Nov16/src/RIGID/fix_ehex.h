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


#ifdef FIX_CLASS

FixStyle(ehex,FixEHEX)

#else

#ifndef LMP_FIX_EHEX_H
#define LMP_FIX_EHEX_H

#include "fix.h"
#include "fix_shake.h"
#include "region.h"
#define EHEX_DEBUG 0

namespace LAMMPS_NS {

class FixEHEX : public Fix {

 public:
  FixEHEX(class LAMMPS *, int, char **);
  ~FixEHEX();
  int  setmask();
  void init();
  void end_of_step();
  void   rescale();
  double compute_scalar();
  double memory_usage();
  void update_scalingmask();
  void com_properties(double *, double *, double *, double*, double *, double*);
  bool rescale_atom(int i, Region*region);
  virtual void grow_arrays(int nmax);
  bool check_cluster(tagint *shake_atom, int n, Region * region);

 private:
  int iregion;
  double heat_input;
  double masstotal;
  double scale;
  char *idregion;
  int me;
 
  double ** x;              // coordinates
  double ** f;              // forces
  double ** v;              // velocities
  double *  mass;           // masses
  double *  rmass;          // reduced masses
  int    *  type;           // atom types
  int   nlocal;             // number of local atoms
  FixShake * fshake;        // pointer to fix_shake/fix_rattle
  int constraints;          // constraints (0/1)
  int cluster;              // rescaling entire clusters (0/1)
  int hex;                  // HEX mode (0/1)
  bool * scalingmask;       // scalingmask[i] determines whether 
                            // the velocity of atom i is to be rescaled
};

}

#endif
#endif

/* ERROR/WARNING messages:

E: Illegal fix ehex command: wrong number of parameters 

Self-explanatory.  Check the input script syntax and compare to the
documentation for the command.  You can use -echo screen as a
command-line option when running LAMMPS to see the offending line.

E: Illegal fix ehex command: integer value expected

Self-explanatory. Check the value for nevery.

E: Region ID for fix ehex does not exist

Self-explanatory.

E: You can only use the keyword 'com' together with the keyword 'constrain' .

Self-explanatory.

E: Illegal fix ehex keyword

Self-explanatory.

E: Fix ehex group has no atoms

Self-explanatory.

E: Multiple instances of fix shake/rattle detected (not supported yet)

You can only have one instance of fix rattle/shake at the moment.

E: Fix ehex was configured with keyword constrain, but shake/rattle was not defined

The option constrain requires either fix shake or fix rattle which is missing in the input script.

E: Fix heat kinetic energy went negative

This will cause the velocity rescaling about to be performed by fix
heat to be invalid.

E: Fix heat kinetic energy of an atom went negative

This will cause the velocity rescaling about to be performed by fix
heat to be invalid.

E: Internal error: shake_flag[m] has to be between 1 and 4 for m in nlist

Contact developers.

E: Fix ehex error mass of region is close to zero

Check your configuration.

*/
