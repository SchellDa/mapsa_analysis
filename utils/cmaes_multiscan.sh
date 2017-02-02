#!/bin/bash
#####################
## Helper script for multiple alignment job submission.
## All configuration is passed in environment variables.
##  RUNS
##  NUM_RETRIES
##  LAMBDA_START & LAMBDA_STOP
##  LAMBDA_STEP
##  LAMBDAS
##  NUM_SAMPLES
##  MPA_UTIL_SCRIPT_PATH
##  MPA_UTIL_BIN_PATH
####################

if [ -z ${MPA_UTIL_SCRIPT_PATH} ]; then
	echo "Please point MPA_UTIL_SCRIPT_PATH to utility script directory. Usually \$SOURCES/utils"
	exit 1
fi

if [ -z ${MPA_UTIL_BIN_PATH} ]; then
	echo "Please point MPA_UTIL_BIN_PATH to utility binaries directory. Usually \$SOURCES/build/utils"
	exit 1
fi

# only hard-coded config. Path were MpaCmaesAlign stores data.
OUTPUT_PATH="/scratch/vollmer/mpaAnalysisOutput"

# Apply default values if env is unset
if [ -z "${RUNS}" ]; then
	RUNS="28 63 174"
	echo "Default RUNS=${RUNS}"
fi
if [ -z ${NUM_RETRIES} ]; then
	NUM_RETRIES=1000
	echo "Default NUM_RETRIES=${NUM_RETRIES}"
fi
if [ -z ${NUM_SAMPLES} ]; then
	NUM_SAMPLES=30000000
	echo "Default NUM_SAMPLES=${NUM_SAMPLES}"
fi

if [ -z "${LAMBDAS}" ]; then
	if [ -z ${LAMBDA_START} ] || [ -z ${LAMBDA_STOP} ]; then
		LAMBDA_START=20
		LAMBDA_STOP=20
		echo "Default LAMBDA_START=${LAMBDA_START}"
		echo "    and LAMBDA_STOP=${LAMBDA_STOP}"
	fi
	if [ -z ${LAMBDA_STEP} ]; then
		LAMBDA_STEP=1
		echo "Default LAMBDA_STEP=${LAMBDA_STEP}"
	fi
	LAMBDAS=`seq ${LAMBDA_START} ${LAMBDA_STEP} ${LAMBDA_STOP}`
	echo "LAMBDAS not set, generating sequence from ${LAMBDA_START} to ${LAMBDA_STOP} in ${LAMBDA_STEP} increments."
fi


CMD_FILE=".cmds"
MULTI_PREFIX=`date "+%Y-%m-%d__%H-%M-%S"`

DATA_PATH="${OUTPUT_PATH}/MpaCmaesAlign/${MULTI_PREFIX}"

# Generate job file
rm -f ${CMD_FILE}
num=1
for run in ${RUNS}; do
	echo "Preparing run ${run}..."
	for lambda in ${LAMBDAS}; do
		for retry in `seq ${NUM_RETRIES}`; do
			echo "./analyses/analyses MpaCmaesAlign -C -M ${MULTI_PREFIX} --run ${run} -n ${NUM_SAMPLES} -D \"cmaes_lambda=${lambda}\" -D \"output_prefix=num${num}\" > /dev/null 2>&1" >> ${CMD_FILE}
			num=`expr ${num} + 1`
		done
	done
done

# Execute jobs
time ${MPA_UTIL_BIN_PATH}/batchsubmit < ${CMD_FILE}
pkill analyses

# Merge data in sqlite3 database
python ${MPA_UTIL_SCRIPT_PATH}/cmaes2sqlite.py ${DATA_PATH} ${OUTPUT_PATH}/multirun_${MULTI_PREFIX}.sqldat
echo "Output written to ${OUTPUT_PATH}/multirun_${MULTI_PREFIX}.sqldat"
echo "Done."

