#!/bin/sh
# Your job name
#$ -N olu2
# Use current working directory
#$ -cwd
# Join stdout and stderr
#$ -j y
#Run int this project
#$ -P science
# job run time hours:min:sec (always over estimate )
#$ -l s_rt=45:00:00
# Run job through bash shell
#$ -V
#$ -S /bin/bash

#cd /data/noo11/test2  
R CMD BATCH rnumber.R result1.txt
##############run the lammp code
cd /data/noo11/test2/input1; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript1.lammps & cd /data/noo11/test2/input2; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript2.lammps & cd /data/noo11/test2/input3; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript3.lammps & cd /data/noo11/test2/input4; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript4.lammps & cd /data/noo11/test2/input5; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript5.lammps & cd /data/noo11/test2/input6; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript6.lammps & cd /data/noo11/test2/input7; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript7.lammps & cd /data/noo11/test2/input8; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript8.lammps & cd /data/noo11/test2/input9; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript9.lammps & cd /data/noo11/test2/input10; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript10.lammps &

cd /data/noo11/test2/input11; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript11.lammps & cd /data/noo11/test2/input12; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript12.lammps & cd /data/noo11/test2/input13; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript13.lammps & cd /data/noo11/test2/input14; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript14.lammps & cd /data/noo11/test2/input15; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript15.lammps & cd /data/noo11/test2/input16; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript16.lammps & cd /data/noo11/test2/input17; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript17.lammps & cd /data/noo11/test2/input18; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript18.lammps & cd /data/noo11/test2/input19; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript19.lammps & cd /data/noo11/test2/input20; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript20.lammps &

cd /data/noo11/test2/input21; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript21.lammps & cd /data/noo11/test2/input22; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript22.lammps & cd /data/noo11/test2/input23; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript23.lammps & cd /data/noo11/test2/input24; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript24.lammps & cd /data/noo11/test2/input25; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript25.lammps & cd /data/noo11/test2/input26; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript26.lammps & cd /data/noo11/test2/input27; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript27.lammps & cd /data/noo11/test2/input28; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript28.lammps & cd /data/noo11/test2/input29; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript29.lammps & cd /data/noo11/test2/input30; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript30.lammps &

cd /data/noo11/test2/input31; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript31.lammps & cd /data/noo11/test2/input32; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript32.lammps & cd /data/noo11/test2/input33; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript33.lammps & cd /data/noo11/test2/input34; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript34.lammps & cd /data/noo11/test2/input35; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript35.lammps & cd /data/noo11/test2/input36; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript36.lammps & cd /data/noo11/test2/input37; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript37.lammps & cd /data/noo11/test2/input38; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript38.lammps & cd /data/noo11/test2/input39; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript39.lammps & cd /data/noo11/test2/input40; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript40.lammps &

cd /data/noo11/test2/input41; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript41.lammps & cd /data/noo11/test2/input42; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript42.lammps & cd /data/noo11/test2/input43; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript43.lammps & cd /data/noo11/test2/input44; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript44.lammps & cd /data/noo11/test2/input45; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript45.lammps & cd /data/noo11/test2/input46; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript46.lammps & cd /data/noo11/test2/input47; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript47.lammps & cd /data/noo11/test2/input48; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript48.lammps & cd /data/noo11/test2/input49; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript49.lammps & cd /data/noo11/test2/input50; /home/noo11/Lammps-NUFEB/lammps1Feb2014/lmp_serial<Inputscript50.lammps &
##################
##################################post-processing with matlab
cd /data/noo11/test2/input1; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input2; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input3;
matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input4; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input5; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input6;
matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input7; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input8; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input9;
matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input10; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" &

cd /data/noo11/test2/input11; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input12; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input13;
matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input14; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input15; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input16;
matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input17; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input18; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input19;
matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input20; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" &

cd /data/noo11/test2/input21; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input22; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input23;
matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input24; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input25; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input26;
matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input27; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input28; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input29;
matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input30; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" &

cd /data/noo11/test2/input31; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input32; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input33;
matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input34; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input35; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input36;
matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input37; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input38; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input39;
matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input40; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" &

cd /data/noo11/test2/input41; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input42; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input43;
matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input44; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input45; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input46;
matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input47; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input48; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input49;
matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" & cd /data/noo11/test2/input50; matlab -nosplash -nodisplay -nodesktop -r "run ./readfile.m ; quit;" &

