#!/bin/bash

# wrapper around SDCC to let the external interface look more gcc-alike
#
# Differences:
# - if a .cpp files is given as input temporarly copy it to .c and compile it
#   as a c file. This will break the dependency check, as it expects the
#   full original filename, but as this happens only for the original .ino
#   file it is not a big loss.
# - generate .rel files, but copy them as .o files as well to satisfy the
#   dependency checker on following builds


VERBOSE=0
USE_COLOR=0
if [ "$1" == "-v" ]; then
	VERBOSE=1;
	shift
elif [ "$1" == "-vv" ]; then
	VERBOSE=2
	set -x
	shift
fi


if [ $USE_COLOR -gt 0 ]; then
# ANSI color codes to beautify the output:
BLACK='\033[0;30m'
RED='\033[0;31m'
GREEN='\033[0;32m'
ORANGE='\033[0;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
LGRAY='\033[0;37m'
DGRAY='\033[1;30m'
LRED='\033[1;31m'
LGREEN='\033[1;32m'
YELLOW='\033[1;33m'
LBLUE='\033[1;34m'
LPURPLE='\033[1;35m'
LCYAN='\033[1;36m'
WHITE='\033[1;37m'
OFF='\033[0m'
fi

# check if cp is in the path using 'command -v' (a builtin POSIX function)
if ! command -v cp > /dev/null; then
	cd "${0%/wrapper/*}"
	PATH=$(pwd)/win:$PATH
fi

# echo the mark id in green and the compiler call in white:
SDCC=$1
SRC=$2
OBJ=$3
REL=${OBJ%.o}.rel
MARK=$4
shift 4

# Filter flags that SDCC does not support (-MF <file>) or that fail on null output (-MMD on /dev/null or nul)
CLEAN_FLAGS=()
SKIP_NEXT=0
MF_FILE=""

for ARG in "$@"; do
    if [ $SKIP_NEXT -eq 1 ]; then
        MF_FILE="$ARG"
        SKIP_NEXT=0
        continue
    fi
    if [ "$ARG" == "-MF" ]; then
        SKIP_NEXT=1
        continue
    fi
    if [[ "$ARG" == -MF* ]]; then
        MF_FILE="${ARG#-MF}"
        continue
    fi
    if [[ ("$OBJ" == "/dev/null" || "$OBJ" == "nul" || "$OBJ" == "NUL") && "$ARG" == "-MMD" ]]; then
        continue
    fi
    CLEAN_FLAGS+=("$ARG")
done

if [ $VERBOSE -gt 0 ]; then
	>&2 echo -ne "${GREEN}Mark $MARK:${OFF}"
	>&2 echo "$SDCC" "${CLEAN_FLAGS[@]}" "$SRC" -o "$OBJ"
fi

case "$SRC" in
	*.c)
		# compile a .c file
		"$SDCC" "${CLEAN_FLAGS[@]}" "$SRC" -o "$OBJ"
		ERR=$?
		;;
	*.cpp)
		# use -x c to compile as c, add a reference to main to pull in main.c
		"$SDCC" "${CLEAN_FLAGS[@]}" -x c --include dummy_variable_main.h "$SRC"  -o "$OBJ"
		ERR=$?
		;;
	*)
		if [ "$MARK" == "cpp" ]; then
			"$SDCC" "${CLEAN_FLAGS[@]}" -x c --include dummy_variable_main.h "$SRC"  -o "$OBJ"
		else
			"$SDCC" "${CLEAN_FLAGS[@]}" -x c "$SRC"  -o "$OBJ"
		fi
		ERR=$?
		;;
esac

# If an -MF dependency file was requested during library detection on null output, create it
if [ -n "$MF_FILE" ] && [[ "$OBJ" == "/dev/null" || "$OBJ" == "nul" || "$OBJ" == "NUL" ]]; then
    touch "$MF_FILE" 2>/dev/null || true
fi

# copy the generated .rel files as an .o file to avoid recompiling the next time
# if OBJ is a .o file we copy back
if [[ ${OBJ%.o} != $OBJ ]]
then
	cp -a "${OBJ}" "${REL}" 2>/dev/null || true
fi

#check rel file size, if it is odd, add by 1 to align code
if [ -f ${REL} ]; then
    #check CSEG size
    CSEG_STR="$(grep -o '^A CSEG size [0-9A-F]\+' ${REL})"
    if [[ -z "$CSEG_STR" ]]; then
        if [ $VERBOSE -gt 0 ]; then
            >&2 echo "CSEG String not found in ${REL}"
        fi
    else
        CSEG_HEX_VAL="$(echo ${CSEG_STR} | grep -o '[^ ]*$')"
        CSEG_DEC_VAL="$(printf '%d' 0x${CSEG_HEX_VAL})"
        if [ $VERBOSE -gt 0 ]; then
            >&2 echo "CSEG of ${REL} is hex: ${CSEG_HEX_VAL}, dec:${CSEG_DEC_VAL}"
        fi
        if [ $((CSEG_DEC_VAL%2)) -eq 1 ]; then
            CSEG_ADDED_HEX_VAL="$(printf '%X' $((CSEG_DEC_VAL+1)))"
            if [ $VERBOSE -gt 0 ]; then
                >&2 echo "Change CSEG value from ${CSEG_HEX_VAL} to ${CSEG_ADDED_HEX_VAL}"
            fi
            if [[ $(uname) == "Darwin" ]]; then
				if [[ $(sed --version 2>/dev/null) != *GNU* ]]; then
					sed -i '' -e "s/${CSEG_STR}/A CSEG size ${CSEG_ADDED_HEX_VAL}/g" ${REL}
				else
					sed -i'' -e "s/${CSEG_STR}/A CSEG size ${CSEG_ADDED_HEX_VAL}/g" ${REL}
				fi
            else
                sed -i'' -e "s/${CSEG_STR}/A CSEG size ${CSEG_ADDED_HEX_VAL}/g" ${REL}
            fi
        fi
    fi
    #check GSINIT size with the same method
    GSINIT_STR="$(grep -o '^A GSINIT size [0-9A-F]\+' ${REL})"
    if [[ -z "$GSINIT_STR" ]]; then
        if [ $VERBOSE -gt 0 ]; then
            >&2 echo "GSINIT String not found in ${REL}"
        fi
    else
        GSINIT_HEX_VAL="$(echo ${GSINIT_STR} | grep -o '[^ ]*$')"
        GSINIT_DEC_VAL="$(printf '%d' 0x${GSINIT_HEX_VAL})"
        if [ $VERBOSE -gt 0 ]; then
            >&2 echo "GSINIT of ${REL} is hex: ${GSINIT_HEX_VAL}, dec:${GSINIT_DEC_VAL}"
        fi
        if [ $((GSINIT_DEC_VAL%2)) -eq 1 ]; then
            GSINIT_ADDED_HEX_VAL="$(printf '%X' $((GSINIT_DEC_VAL+1)))"
            if [ $VERBOSE -gt 0 ]; then
                >&2 echo "Change GSINIT value from ${GSINIT_HEX_VAL} to ${GSINIT_ADDED_HEX_VAL}"
            fi
            if [[ $(uname) == "Darwin" ]]; then
				if [[ $(sed --version 2>/dev/null) != *GNU* ]]; then
					sed -i '' -e "s/${GSINIT_STR}/A GSINIT size ${GSINIT_ADDED_HEX_VAL}/g" ${REL}
				else
					sed -i'' -e "s/${GSINIT_STR}/A GSINIT size ${GSINIT_ADDED_HEX_VAL}/g" ${REL}
				fi
            else
                sed -i'' -e "s/${GSINIT_STR}/A GSINIT size ${GSINIT_ADDED_HEX_VAL}/g" ${REL}
            fi
        fi
    fi
    #check if "A GSFINAL size 3" exists in main, the "ljmp __sdcc_program_startup" is 3 bytes and takes GSFINAL
    if [ -n "$(echo ${REL} | grep -c main.c)" ]; then
        if [[ $(uname) == "Darwin" ]]; then
			if [[ $(sed --version 2>/dev/null) != *GNU* ]]; then
				sed -i '' -e "s/A GSFINAL size 3/A GSFINAL size 4/g" ${REL} 
			else
				sed -i'' -e "s/A GSFINAL size 3/A GSFINAL size 4/g" ${REL}
			fi
        else
            sed -i'' -e "s/A GSFINAL size 3/A GSFINAL size 4/g" ${REL}
        fi
    fi
    if [[ ${OBJ%.o} != $OBJ && -f "${REL}" ]]; then
        cp -a "${REL}" "${OBJ}" 2>/dev/null || true
    fi
fi
# propagate the sdcc exit code
exit $ERR