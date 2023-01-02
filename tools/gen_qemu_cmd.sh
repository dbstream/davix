#!/bin/bash
# SPDX-License-Identifier: MIT
# This is a tool that can be used to generate `qemu-system-x86_64` arguments.
# It accepts a number of arguments that affect the behaviour of the virtual
# machine.

_help() {
	echo "Usage: $0 [options]"
	echo 'options:'
	echo '  -h, --help                      Show this information.'
	echo ''
	echo '  -d, --debug                     Equivalent of `qemu-system-x86_64 -S -s`.'
	echo ''
	echo '  -X <option>                     Pass <option> directly to QEMU.'
	echo ''
	echo '  -m, --mem=<memory>              Set the amount of memory provided to a guest.'
	echo '                                  (default: 256M)'
	echo ''
	echo '      --kvm                       Enable KVM acceleration of the guest. (enabled'
	echo '                                  by default)'
	echo '      --no-kvm                    Disable KVM acceleration of the guest.'
	echo ''
	echo '      --cpu=<cpu>                 Select CPU model of the guest.'
	echo ''
	echo '      --reboot={normal,shutdown}  Machine reboot action, also affects triple'
	echo '                                  faults. (default: shutdown)'
	echo ''
	echo '      --bios'
	echo '      --uefi'
	echo '      --firmware={BIOS, UEFI}     Switch between BIOS and UEFI firmware.'
	echo '                                  (default: UEFI)'
	echo '                                  By default, OVMF firmware is searched for in'
	echo '                                  the directory "/usr/share/ovmf/x64". This can'
	echo '                                  be overridden by providing the environment'
	echo '                                  variable "OVMF_PATH".'
}

getopt -T
if [ $? -ne 4 ]; then
	echo "GNU enhanced getopt(1) is required for long options support." >&2
	exit 1
fi

TEMP=$(getopt -o 'hdX:m:' --long 'help,debug,kvm,no-kvm,cpu:,mem:,reboot:,bios,uefi,firmware:' -n "$0" -- "$@")
if [ $? -ne 0 ]; then
	_help >&2
	exit 1
fi

eval set -- "$TEMP"
unset TEMP

DEBUG="n"
USE_KVM="y"
QMEM="256M"
QEXTRA=""
QCPU="host"
NO_REBOOT="y"
FIRMWARE="UEFI"

while true; do
	case "$1" in
		'-h'|'--help')
			_help >&2
			exit 1
		;;
		'-d'|'--debug')
			DEBUG="y"
			shift
		;;
		'-X')
			QEXTRA="$QEXTRA $2"
			shift 2
		;;
		'--kvm')
			USE_KVM='y'
			shift
		;;
		'--no-kvm')
			USE_KVM='n'
			shift
		;;
		'--cpu')
			QCPU="$2"
			shift 2
		;;
		'-m'|'--mem')
			QMEM="$2"
			shift 2
		;;
		'--reboot')
			if [ "$2" == "normal" ]; then
				NO_REBOOT="n"
			elif [ "$2" == "shutdown" ]; then
				NO_REBOOT="y"
			else
				echo "Unrecognized reboot behaviour: \"$2\"" >&2
				_help
				exit 1
			fi
			shift 2
		;;
		'--bios')
			FIRMWARE="BIOS"
			shift
		;;
		'--uefi')
			FIRMWARE='UEFI'
			shift
		;;
		'--firmware')
			case "$2" in
				'bios'|'BIOS')
					FIRMWARE="BIOS"
				;;
				'uefi'|'UEFI')
					FIRMWARE="UEFI"
				;;
				*)
					echo "Unrecognized firmware type: \"$2\" (supported are: bios, uefi)" >&2
					_help >&2
					exit 1
				;;
			esac
			shift 2
		;;
		'--')
			shift
			break
		;;
		*)
			echo "getopt(1) error (Are you using the GNU enhanced version?)" >&2
			exit 1
		;;
	esac
done

QEMU_COMMAND="qemu-system-x86_64"

if [ $USE_KVM = "y" ]; then
	QEMU_COMMAND="$QEMU_COMMAND -enable-kvm"
fi

if [ $NO_REBOOT = "y" ]; then
	QEMU_COMMAND="$QEMU_COMMAND -no-reboot"
fi

if [ $FIRMWARE = "UEFI" ]; then
	if [ ! -n "$OVMF_PATH" ]; then
		OVMF_PATH="/usr/share/ovmf/x64"
	fi
	QEMU_COMMAND="$QEMU_COMMAND -drive file=$OVMF_PATH/OVMF.fd,if=pflash,index=0,read-only=on"
fi

QEMU_COMMAND="$QEMU_COMMAND -m $QMEM"

QEMU_COMMAND="$QEMU_COMMAND -cpu $QCPU"

if [ $DEBUG = "y" ]; then
	QEMU_COMMAND="$QEMU_COMMAND -S -s"
fi

echo "$QEMU_COMMAND$QEXTRA"
