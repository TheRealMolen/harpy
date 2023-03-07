# Project Name
TARGET = harpy

# Sources
CPP_SOURCES = harpy.cpp fm_operator.cpp ks.cpp voice.cpp daisyclock.cpp

# Library Locations
LIBDAISY_DIR = ../DaisyExamples/libDaisy
DAISYSP_DIR = ../DaisyExamples/DaisySP

#C_DEFS = -DUSE_ARM_DSP
CPP_STANDARD = -std=c++20

# Core location, and generic Makefile.
SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile
