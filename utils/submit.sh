#!/bin/bash

CMDFILE='.commands'

SELECTORS=
COMMAND=

do_select=true
for i in `seq $#`; do
	arg=${!i}
	if $do_select; then
		if [ $arg = "--" ]; then
			do_select=false
		else
			SELECTORS="${SELECTORS} $arg"
		fi
	else
		COMMAND="${COMMAND}'$arg' "
	fi
done


rm -f ${CMDFILE}
touch ${CMDFILE}
for i in $SELECTORS; do
	repl="s/%select/${i}/g"
	if [ $SILENT ]; then
		if [ $SILENT -eq 2 ]; then
			echo $COMMAND "> /dev/null 2>&1" | sed $repl >> "${CMDFILE}"
		else
			echo $COMMAND "> /dev/null" | sed $repl >> "${CMDFILE}"
		fi
	else
		echo $COMMAND | sed $repl >> "${CMDFILE}"
	fi
done
./utils/batchsubmit < "${CMDFILE}"
