#! /bin/bash
current_row=-1
tot_stat_row=1
mypath="../results/$1-torus"
rm -rf $mypath
mkdir $mypath
filename="sample-torus"
../build/Garnet_standalone/gem5.opt ../configs/example/garnet_synth_traffic.py \
--num-passes=2 \
--net=sample_torus_net \
--sys=sample_torus_sys \
--workload=DLRM_HybridParallel \
--compute-scale=1.0 \
--comm-scale=0.5 \
--path="$mypath/" \
--total-stat-rows=1 \
--stat-row=0 \
--run-name="$filename" \
--synthetic=training
