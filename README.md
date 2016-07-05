# nufeb

Github repo for the NUFEB project

Separate directories for the use of each science team

git reminders in "git-reminders.txt"


## Getting Started Guide
This is a quick guide on how to compile and run a simple example of the LAMMPS code.

To compile the code:

1. Change to the directory nufeb/code/lammps1Feb2014/src  
```
cd nufeb/code/lammps1Feb2014/src
```
2. Change to directory STUBS  
```
cd STUBS
```
3. Make the STUBS  
```
make clean
make
```
4. Change back up to src directory  
```
cd ..
```
5. Clean things up  
```
make clean-all
```
6. Set up some build states
```
make no-USER-ATC
make yes-GRANULAR
```
7. Set up to use NUFEB Work
```
make yes-USER-NUFEB
```
8. Make the executable (only the sequential version is presented here)  ```
make serial
```

To run the work you can use the example file in nufeb/code/input/examples/Inputscript.lammps:

1. Change to directory nufeb/code/input/examples
```
cd nufeb/code/input/examples
```
2. run the code with:
```
../../lammps1Feb2014/src/lmp_serial < Inputscript.lammps
```

Testing things have run correctly:

If the code runs successfully the output should finish with something like:

```
Loop time of 7.72733 on 1 procs for 10000 steps with 1013 atoms

Pair  time (%) = 0.33114 (4.28531)
Neigh time (%) = 0.233242 (3.0184)
Comm  time (%) = 0.156616 (2.02678)
Outpt time (%) = 0.0148082 (0.191634)
Other time (%) = 6.99152 (90.4779)

Nlocal:    1013 ave 1013 max 1013 min
Histogram: 1 0 0 0 0 0 0 0 0 0
Nghost:    0 ave 0 max 0 min
Histogram: 1 0 0 0 0 0 0 0 0 0
Neighs:    5941 ave 5941 max 5941 min
Histogram: 1 0 0 0 0 0 0 0 0 0

Total # of neighbors = 5941
Ave neighs/atom = 5.86476
Neighbor list builds = 89
Dangerous builds = 0
```