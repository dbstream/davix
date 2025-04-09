#!/bin/sh
# A script for importing an updated version of uACPI into the kernel.
# Copyright (C) 2025-present  dbstream
#
# Usage:
#   ./update_uacpi.sh /path/to/uACPI

set -e

if [ 'acpi' != $(basename $(pwd)) ]; then
	echo '*** update_uacpi.sh:  needs to run in the acpi/ directory ***'
	exit 1
fi

pushd -- $1
uacpi_version=$(git log -n 1 --pretty='format:commit %h  (%s)')
popd

echo "New uACPI version is $uacpi_version."
read -r -p 'Are you sure?  [y/N] ' response
case "$response" in
	[yY][eE][sS]|[yY])
		;;
	*)
		echo 'Ok, not updating.'
		exit 1
esac

echo 'Ok, updating.'

if [ -d uacpi ]; then
	rm -r uacpi
fi
sed -i -E -e "s/(Current uACPI version: ).*/\\1$uacpi_version/" README
mkdir uacpi
cp -r $1/LICENSE uacpi/LICENSE
cp -r $1/include uacpi/include
cp -r $1/source uacpi/source
cp Sources_uacpi uacpi/Sources
cp Sources_uacpi_source uacpi/source/Sources
