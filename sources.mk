# SPDX-License-Identifier: MIT

ifneq ($(HPGP_ROOT),)
	hpgp-basedir := $(HPGP_ROOT)/
endif

HPGP_SRCS := \
$(hpgp-basedir)src/hpgp.c \

HPGP_INCS := $(hpgp-basedir)include
