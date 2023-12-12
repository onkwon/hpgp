# SPDX-License-Identifier: MIT

COMPONENT_NAME = HPGP

SRC_FILES = \
	../src/hpgp.c \

TEST_SRC_FILES = \
	src/hpgp_test.cpp \
	src/test_all.cpp \

INCLUDE_DIRS = \
	$(CPPUTEST_HOME)/include \
	../include \

MOCKS_SRC_DIRS =
CPPUTEST_CPPFLAGS = -DUNIT_TEST

include runners/MakefileRunner
