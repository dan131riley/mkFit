#! /bin/bash

## Use this script to turn auto debug printouts. Warning: debug statements are ifdef'ed and not maintained

## initialize
source xeon_scripts/common-variables.sh
source xeon_scripts/init-env.sh


## Common setup
dir=/data2/slava77/samples/2017/pass-4874f28/initialStep/
subdir=10muPt0p5to10HS
file=memoryFile.fv3.clean.writeAll.recT.072617.bin

## config for debug
nevents=10
maxth=1
maxvu=1
maxev=1

## base executable
exe="./mkFit/mkFit --cmssw-n2seeds --num-thr ${maxth} --num-thr-ev ${maxev} --input-file ${dir}/${subdir}/${file} --num-events ${nevents}"

## Compile once
mOpt="DEBUG:=1 WITH_ROOT:=1 USE_INTRINSICS:=-DMPT_SIZE=${maxvu} AVX_512:=1"
make distclean ${mOpt}
make -j 32 ${mOpt}

## test each build routine to be sure it works!
for bV in "BH bh" "STD std" "CE ce" "FV fv"
do echo ${bV} | while read -r bN bO
    do
	oBase=${val_arch}_${sample}_${bN}
	bExe="${exe} --build-${bO}"

	echo "${oBase}: ${vN} [nTH:${maxth}, nVU:${maxvu}, nEV:${maxev}]"
	${bExe} >& log_${oBase}_NVU${maxvu}_NTH${maxth}_NEV${maxev}_"DEBUG".txt
    done
done

## clean up
make distclean ${mOpt}