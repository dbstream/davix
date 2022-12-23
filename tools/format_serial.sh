#!/bin/bash
# SPDX-License-Identifier: MIT
# This utility script wraps printk() output on the serial console. It
# color-codes output according to their loglevel and color-codes
# subsystem names suffixed with a colon, ':'.
#
# A printk() log occupies a single line. The format of a printk() log
# is as follows:
#         "<loglevel>[<subsystem name>: ]<message><newline>"

exec sed \
	-E -e 's/^0([][a-zA-Z0-9 ,.+?*^<>/\()_-]*:)?(.*)$/\x1b\[33m\1\x1b\[0;90m\2\x1b\[0m/g' \
	-E -e 's/^1([][a-zA-Z0-9 ,.+?*^<>/\()_-]*:)?(.*)$/\x1b\[33m\1\x1b\[0;0m\2\x1b\[0m/g' \
	-E -e 's/^2([][a-zA-Z0-9 ,.+?*^<>/\()_-]*:)?(.*)$/\x1b\[33m\1\x1b\[0;93m\2\x1b\[0m/g' \
	-E -e 's/^3([][a-zA-Z0-9 ,.+?*^<>/\()_-]*:)?(.*)$/\x1b\[33m\1\x1b\[0;31m\2\x1b\[0m/g' \
	-E -e 's/^4([][a-zA-Z0-9 ,.+?*^<>/\()_-]*:)?(.*)$/\x1b\[33m\1\x1b\[1;31m\2\x1b\[0m/g'
