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
LIB_INSTALLS    += ../os/windows-x64/CLAllSerial_MD_VC141_v3_3.lib
BIN_INSTALLS    += ../os/windows-x64/CLAllSerial_MD_VC141_v3_3.dll
LIB_INSTALLS    += ../os/windows-x64/CLProtocol_MD_VC141_v3_3.lib
BIN_INSTALLS    += ../os/windows-x64/CLProtocol_MD_VC141_v3_3.dll
LIB_INSTALLS    += ../os/windows-x64/FirmwareUpdate_MD_VC141_v3_3.lib
#BIN_INSTALLS    += ../os/windows-x64/FirmwareUpdate_MD_VC141_v3_3.dll
LIB_INSTALLS    += ../os/windows-x64/GCBase_MD_VC141_v3_3.lib
BIN_INSTALLS    += ../os/windows-x64/GCBase_MD_VC141_v3_3.dll
LIB_INSTALLS    += ../os/windows-x64/GenApi_MD_VC141_v3_3.lib
BIN_INSTALLS    += ../os/windows-x64/GenApi_MD_VC141_v3_3.dll
LIB_INSTALLS    += ../os/windows-x64/Log_MD_VC141_v3_3.lib
BIN_INSTALLS    += ../os/windows-x64/Log_MD_VC141_v3_3.dll
#LIB_INSTALLS    += ../os/windows-x64/MathParser_MD_VC141_v3_3.lib
BIN_INSTALLS    += ../os/windows-x64/MathParser_MD_VC141_v3_3.dll
#LIB_INSTALLS    += ../os/windows-x64/NodeMapData_MD_VC141_v3_3.lib
BIN_INSTALLS    += ../os/windows-x64/NodeMapData_MD_VC141_v3_3.dll
LIB_INSTALLS    += ../os/windows-x64/XmlParser_MD_VC141_v3_3.lib
BIN_INSTALLS    += ../os/windows-x64/XmlParser_MD_VC141_v3_3.dll
LIB_INSTALLS    += ../os/windows-x64/log4cpp_MD_VC141_v3_3.lib
BIN_INSTALLS    += ../os/windows-x64/log4cpp_MD_VC141_v3_3.dll

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

