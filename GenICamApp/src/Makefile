TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

INC += GenICamFeature.h
INC += ADGenICam.h
INC += ADGenICamAPI.h

# This flag is what switches to declspec(dllexport) when building the library
USR_CPPFLAGS += -DBUILDING_ADGenICam_API

LIBRARY_IOC_WIN32 += ADGenICam
LIBRARY_IOC_Linux += ADGenICam
LIBRARY_IOC_Darwin += ADGenICam

DBD += ADGenICamSupport.dbd

LIB_SRCS += GenICamFeature.cpp
LIB_SRCS += ADGenICam.cpp
USR_CXXFLAGS_Linux += -std=c++11

include $(ADCORE)/ADApp/commonLibraryMakefile

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

