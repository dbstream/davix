# Configuration Makefile.
# Copyright (C) 2025-present  dbstream
#
# This configuration Makefile takes configuration variables from the
# environment, exports them, and adds them to CPPFLAGS.

CONFIG_KTEST ?= y
CONFIG_KTEST_FIREWORKS ?= y
CONFIG_KTEST_VMATREE ?= n

CPPFLAGS-$(CONFIG_KTEST) += -DCONFIG_KTEST
CPPFLAGS-$(CONFIG_KTEST_FIREWORKS) += -DCONFIG_KTEST_FIREWORKS
CPPFLAGS-$(CONFIG_KTEST_VMATREE) += -DCONFIG_KTEST_VMATREE

export CONFIG_KTEST
export CONFIG_KTEST_FIREWORKS
export CONFIG_KTEST_VMATREE

CPPFLAGS += $(CPPFLAGS-y)
