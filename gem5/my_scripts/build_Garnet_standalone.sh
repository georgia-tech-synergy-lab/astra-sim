#!/bin/tcsh
source my_scripts/set_env.cshrc
python `which scons` debug=1 -j 24 build/Garnet_standalone/gem5.opt
