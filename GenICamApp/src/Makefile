TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

INC += GenICamFeature.h
INC += ADGenICam.h
INC += ADGenICamAPI.h

# This flag is what switches to declspec(dllexport) when building the library
USR_CPPFLAGS += -DBUILDING_ADGenICam_API

LIBRARY_IOC += ADGenICam

LIB_SRCS += GenICamFeature.cpp
LIB_SRCS += ADGenICam.cpp

include $(ADCORE)/ADApp/commonLibraryMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

