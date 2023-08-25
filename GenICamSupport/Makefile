TOP=../
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#=============================

INC += GenICam.h
INC += GenICamFwd.h
INC += GenICamVersion.h
INC += _GenICamVersion.h
INC += $(patsubst ../%, %, $(wildcard ../Base/*))
INC += $(patsubst ../%, %, $(wildcard ../GenApi/*.h))
INC += $(patsubst ../%, %, $(wildcard ../TLI/*.h))

# Note, the following files were manually copied from the Spinnaker distribution lib directories on Windows
# and from /usr/lib on Linux.
ifeq (windows-x64, $(findstring windows-x64, $(T_A)))
#LIB_INSTALLS    += ../os/windows-x64/VimbaUSBTL.xml

else ifeq (linux-x86_64, $(findstring linux-x86_64, $(T_A)))
LIB_INSTALLS    += ../os/linux-x86_64/libCLAllSerial_gcc48_v3_3.so
LIB_INSTALLS    += ../os/linux-x86_64/libCLProtocol_gcc48_v3_3.so
LIB_INSTALLS    += ../os/linux-x86_64/libFirmwareUpdate_gcc48_v3_3.so
LIB_INSTALLS    += ../os/linux-x86_64/libGCBase_gcc48_v3_3.so
LIB_INSTALLS    += ../os/linux-x86_64/libGenApi_gcc48_v3_3.so
LIB_INSTALLS    += ../os/linux-x86_64/libLog_gcc48_v3_3.so
LIB_INSTALLS    += ../os/linux-x86_64/libMathParser_gcc48_v3_3.so
LIB_INSTALLS    += ../os/linux-x86_64/libNodeMapData_gcc48_v3_3.so
LIB_INSTALLS    += ../os/linux-x86_64/libXmlParser_gcc48_v3_3.so
LIB_INSTALLS    += ../os/linux-x86_64/liblog4cpp_gcc48_v3_3.so

endif

#=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE

