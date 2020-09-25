/*
 * ADGenICam.cpp
 *
 * This is a base class driver for GenICam cameras 
 *
 * Author: Mark Rivers
 *         University of Chicago
 *
 * Created: October 7, 2018
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <set>

#include <epicsEvent.h>
#include <epicsTime.h>
#include <epicsThread.h>
#include <cantProceed.h>
#include <epicsString.h>
#include <epicsExit.h>

#include "ADDriver.h"

#include <epicsExport.h>
#include "GenICamFeature.h"
#include "ADGenICam.h"

static const char *driverName = "ADGenICam";


/** Constructor for the ADGenICam class
 * \param[in] portName asyn port name to assign to the camera.
 * \param[in] maxMemory Maximum memory (in bytes) that this driver is allowed to allocate.
 * \param[in] priority The EPICS thread priority for this driver.  0=use asyn default.
 * \param[in] stackSize The size of the stack for the EPICS port thread. 0=use asyn default.
 */
ADGenICam::ADGenICam(const char *portName, size_t maxMemory, int priority, int stackSize)
    : ADDriver(portName, 1, 0, 0, maxMemory,
            asynInt64Mask | asynEnumMask, 
            asynInt64Mask | asynEnumMask,
            ASYN_CANBLOCK, 1, priority, stackSize),
    mGCFeatureSet(this, pasynUserSelf), mFirstDrvUserCreateCall(true)
{
    //static const char *functionName = "ADGenICam";

    createParam(GCFrameRateString,       asynParamFloat64, &GCFrameRate);
    mFirstParam = GCFrameRate;
    createParam(GCFrameRateEnableString, asynParamInt32,   &GCFrameRateEnable);
    createParam(GCTriggerSourceString,   asynParamInt32,   &GCTriggerSource);
    createParam(GCTriggerOverlapString,  asynParamInt32,   &GCTriggerOverlap);
    createParam(GCTriggerSoftwareString, asynParamInt32,   &GCTriggerSoftware);
    createParam(GCExposureModeString,    asynParamInt32,   &GCExposureMode);
    createParam(GCExposureAutoString,    asynParamInt32,   &GCExposureAuto);
    createParam(GCGainAutoString,        asynParamInt32,   &GCGainAuto);
    createParam(GCPixelFormatString,     asynParamInt32,   &GCPixelFormat);
    
    /* Set initial values of some parameters */
    setIntegerParam(NDDataType, NDUInt8);
    setIntegerParam(NDColorMode, NDColorModeMono);
    setIntegerParam(NDArraySizeZ, 0);
    setIntegerParam(ADMinX, 0);
    setIntegerParam(ADMinY, 0);
    setStringParam(ADStringToServer, "<not used by driver>");
    setStringParam(ADStringFromServer, "<not used by driver>");
    
    return;
}

/** Sets an int32 parameter.
  * \param[in] pasynUser asynUser structure that contains the function code in pasynUser->reason. 
  * \param[in] value The value for this parameter 
  *
  * Takes action if the function code requires it.  ADAcquire, ADSizeX, and many other
  * function codes make calls to the underlying library from this function. */

asynStatus ADGenICam::writeInt32( asynUser *pasynUser, epicsInt32 value)
{
    asynStatus status = asynSuccess;
    int function = pasynUser->reason;
    static const char *functionName = "writeInt32";

    // Set the value in the parameter library.  This may change later but that's OK
    status = setIntegerParam(function, value);

    if (function < mFirstParam) {
        // If this parameter belongs to a base class call its method
        status = ADDriver::writeInt32(pasynUser, value);
    } 

    if (function == ADAcquire) {
        if (value) {
            // start acquisition
            status = startCapture();
        } else {
            status = stopCapture();
        }
    } 
    else if ((function == ADSizeX)       ||
             (function == ADSizeY)       ||
             (function == ADMinX)        ||
             (function == ADMinY)        ||
             (function == ADBinX)        ||
             (function == ADBinY)) {    
        status = setImageParams();
    }
    else if (function == ADReadStatus) {
        status = readStatus();
    } 
   if ((function == GCPixelFormat) ||
       (function == ADNumImages)) {
       pauseAcquisition();
       epicsThreadSleep(0.1);
    }
    GenICamFeature *pFeature = mGCFeatureSet.getByIndex(function);
    if (pFeature) {
        if (pFeature->getFeatureType() == GCFeatureTypeInteger) {
            epicsInt64 i64value = value;
            pFeature->write(&i64value, NULL, true);
        } else {
            pFeature->write(&value, NULL, true);
        }
        mGCFeatureSet.readAll();
    }
   if ((function == GCPixelFormat) ||
       (function == ADNumImages)) {
       epicsThreadSleep(0.1);
       resumeAcquisition();
    }

    asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER, 
        "%s::%s function=%d, value=%d, status=%d\n",
        driverName, functionName, function, value, status);
            
    callParamCallbacks();
    return status;
}

/** Sets an int64 parameter.
  * \param[in] pasynUser asynUser structure that contains the function code in pasynUser->reason. 
  * \param[in] value The value for this parameter 
  */

asynStatus ADGenICam::writeInt64( asynUser *pasynUser, epicsInt64 value)
{
    asynStatus status = asynSuccess;
    int function = pasynUser->reason;
    static const char *functionName = "writeInt64";

    // Set the value in the parameter library.  This may change later but that's OK
    status = setInteger64Param(function, value);

    if (function < mFirstParam) {
        // If this parameter belongs to a base class call its method
        status = ADDriver::writeInt64(pasynUser, value);
    } 

    GenICamFeature *pFeature = mGCFeatureSet.getByIndex(function);
    if (pFeature) {
        pFeature->write(&value, NULL, true);
        mGCFeatureSet.readAll();
    }

    asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER, 
        "%s::%s function=%d, value=%lld, status=%d\n",
        driverName, functionName, function, value, status);
            
    callParamCallbacks();
    return status;
}


/** Sets an float64 parameter.
  * \param[in] pasynUser asynUser structure that contains the function code in pasynUser->reason. 
  * \param[in] value The value for this parameter 
  *
  * Takes action if the function code requires it. */

asynStatus ADGenICam::writeFloat64( asynUser *pasynUser, epicsFloat64 value)
{
    asynStatus status = asynSuccess;
    int function = pasynUser->reason;
    static const char *functionName = "writeFloat64";
    
   // Set the value in the parameter library.  This may change later but that's OK
    status = setDoubleParam(function, value);

    if (function < mFirstParam) {
        // If this parameter belongs to a base class call its method
        status = ADDriver::writeFloat64(pasynUser, value);
    } 

    GenICamFeature *pFeature = mGCFeatureSet.getByIndex(function);
    if (pFeature) {
        pFeature->write(&value, NULL, true);
        mGCFeatureSet.readAll();
    }

    asynPrint(pasynUser, ASYN_TRACEIO_DRIVER, 
        "%s::%s function=%d, value=%f, status=%d\n",
        driverName, functionName, function, value, status);
    callParamCallbacks();
    return status;
}

asynStatus ADGenICam::readEnum(asynUser *pasynUser, char *strings[], int values[], int severities[], 
                               size_t nElements, size_t *nIn)
{
    int function = pasynUser->reason;
    int numEnums;
    int i;
    std::vector<std::string> enumStrings;
    std::vector<int> enumValues;
    //static const char *functionName = "readEnum";

    GenICamFeature *pFeature = mGCFeatureSet.getByIndex(function);

    if (pFeature == 0) {
        return asynError;
    }
    if ((pFeature->getFeatureType() != GCFeatureTypeEnum) && 
        (pFeature->getFeatureType() != GCFeatureTypeUnknown)) {
        return asynError;
    }
    // There are a few enums we don't want to autogenerate the values
    if (function == ADImageMode) {
        return asynError;
    }

    *nIn = 0;
    if (!pFeature->isImplemented() || !pFeature->isAvailable()) {
        if (strings[0]) free(strings[0]);
        strings[0] = epicsStrDup("N.A.");
        values[0] = 0;
        *nIn = 1;
        return asynSuccess;
    }

    pFeature->readEnumChoices(enumStrings, enumValues);

    numEnums = (int)enumStrings.size();

    for (i=0; ((i<numEnums) && (i<(int)nElements)); i++) {
        if (pFeature->isAvailable() && pFeature->isReadable()) {
            if (strings[*nIn]) free(strings[*nIn]);
            strings[*nIn] = epicsStrDup(enumStrings[i].c_str());
            values[*nIn] = enumValues[i];
            severities[*nIn] = 0;
            (*nIn)++;
        }
    }
    return asynSuccess;   
}

asynStatus ADGenICam::pauseAcquisition()
{
    int acquiring;
    getIntegerParam(ADAcquire, &acquiring);
    mWasAcquiring = acquiring ? true : false;
    if (mWasAcquiring) {
        stopCapture();
    }
    return asynSuccess;
}

asynStatus ADGenICam::resumeAcquisition()
{
    if (mWasAcquiring) {
        startCapture();
    }
    return asynSuccess;
}

asynStatus ADGenICam::setImageParams()
{
    std::vector<int> paramIndices;
    GenICamFeature *pFeature;
    //static const char *functionName = "setImageParams";
    //bool resumeAcquire;
    unsigned i;
    //if (!pCamera_) return asynError;
    
    pauseAcquisition();
    paramIndices.push_back(ADSizeX);
    paramIndices.push_back(ADSizeY);
    paramIndices.push_back(ADMinX);
    paramIndices.push_back(ADMinY);
    paramIndices.push_back(ADBinX);
    paramIndices.push_back(ADBinY);

    for(i=0; i<paramIndices.size(); i++) {
        pFeature = mGCFeatureSet.getByIndex(paramIndices[i]);
        // On some cameras some features will not be writeable, so don't try if not
        if (pFeature && pFeature->isWritable()) pFeature->write(0, 0, false);
    }

    for(i=0; i<paramIndices.size(); i++) {
        pFeature = mGCFeatureSet.getByIndex(paramIndices[i]);
        if (pFeature) pFeature->read(0, true);
    }
    resumeAcquisition();
    return asynSuccess;
}


asynStatus ADGenICam::readStatus()
{
//    static const char *functionName = "readStatus";
    mGCFeatureSet.readAll();
    callParamCallbacks();
    return asynSuccess;
}

/** Print out a report; calls ADDriver::report to get base class report as well.
  * \param[in] fp File pointer to write output to
  * \param[in] details Level of detail desired.  If >1 prints information about 
               supported video formats and modes, etc.
 */

void ADGenICam::report(FILE *fp, int details)
{
  
    //static const char *functionName = "report";

    if (details > 0) mGCFeatureSet.report(fp, details);
    ADDriver::report(fp, details);
    return;
}

asynStatus ADGenICam::drvUserCreate(asynUser *pasynUser, const char *drvInfo,
                                     const char **pptypeName, size_t *psize) {
    const char *functionName = "drvUserCreate";
    /*printf("drvUserCreate(pasynUser=%p, drvInfo=%s, pptypeName=%p, psize=%p)\n",
            pasynUser, drvInfo, pptypeName, psize);*/
    int index;

    // The first time this is called we first add the standard ADDriver parameters that map to
    // GenICam features
    if (mFirstDrvUserCreateCall) {
        mFirstDrvUserCreateCall = false;
        addADDriverFeatures();
    }

    if (findParam(drvInfo, &index) && strlen(drvInfo) > 3 && !strncmp(drvInfo, "GC_", 3))
    {
        /* Parameters are of the format
         *  GC_X_name
         *
         * Where:
         *   X is one of 'B': GCFeatureTypeBoolean, asynInt32
         *               'D': GCfeatureTypeDouble,  asynFloat64
         *               'E': GCfeatureTypeEnum,    asynInt32
         *               'I': GCFeatureTypeInt,     asynInt32
         *               'S': GCFeatureTypeString,  asynOctet
         */

        asynParamType asynType;
        GCFeatureType_t featureType;
        
        switch(drvInfo[3])
        {
        case 'B':
            featureType = GCFeatureTypeBoolean;
            asynType = asynParamInt32;
            break;
        case 'C':
            featureType = GCFeatureTypeCmd;
            asynType = asynParamInt32;
            break;
        case 'D':
            featureType = GCFeatureTypeDouble;
            asynType = asynParamFloat64;
            break;
        case 'E':
            featureType = GCFeatureTypeEnum;
            asynType = asynParamInt32;
            break;
        case 'I':
            featureType = GCFeatureTypeInteger;
            asynType = asynParamInt64;
            break;
        case 'S':
            featureType = GCFeatureTypeString;
            asynType = asynParamOctet;
            break;
        default:
            asynPrint(pasynUserSelf, ASYN_TRACE_ERROR, 
                "%s::%s [%s] couldn't match %c to an asyn type", 
                driverName, functionName, drvInfo,  drvInfo[3]);
            return asynError;
        }

        std::string featureName(drvInfo+5);

        GenICamFeature *pFeature = createFeature(&mGCFeatureSet, drvInfo, asynType, -1, featureName, featureType);
        if (!pFeature)
            return asynError;

        mGCFeatureSet.insert(pFeature, featureName);
        // Do an initial read of the feature so EPICS output records initialize to this value
        pFeature->read(NULL, true);
        
    }
    return ADDriver::drvUserCreate(pasynUser, drvInfo, pptypeName, psize);
}

asynStatus ADGenICam::createMultiFeature(std::string const & asynName, asynParamType asynType, int asynIndex,
                                         std::string const & featureName1, std::string const & featureName2, 
                                         GCFeatureType_t featureType)
{
    std::string featureName = featureName1;
    GenICamFeature *pFeature = createFeature(&mGCFeatureSet, asynName, asynType, asynIndex, featureName, featureType);
    if (!pFeature->isImplemented()) {
        featureName = featureName2;
        pFeature = createFeature(&mGCFeatureSet, asynName, asynType, asynIndex, featureName, featureType);
    }
    if (pFeature->isImplemented()) {
        mGCFeatureSet.insert(pFeature, featureName);
        // Do an initial read of the feature so EPICS output records initialize to this value
        pFeature->read(NULL, true);
        return asynSuccess;
    }
    return asynError;
}

asynStatus ADGenICam::addADDriverFeatures()
{
    typedef struct {
        int index; 
        std::string featureName;
        GCFeatureType_t featureType;
    } stdParam;
    stdParam params[] = {
        {ADImageMode,         "AcquisitionMode",       GCFeatureTypeEnum},
        {ADFirmwareVersion,   "DeviceFirmwareVersion", GCFeatureTypeString},
        {ADManufacturer,      "DeviceVendorName",      GCFeatureTypeString},
        {ADModel,             "DeviceModelName",       GCFeatureTypeString},
        {ADMaxSizeX,          "WidthMax",              GCFeatureTypeInteger},
        {ADMaxSizeY,          "HeightMax",             GCFeatureTypeInteger},
        {ADSizeX,             "Width",                 GCFeatureTypeInteger},
        {ADSizeY,             "Height",                GCFeatureTypeInteger},
        {ADMinX,              "OffsetX",               GCFeatureTypeInteger},
        {ADMinY,              "OffsetY",               GCFeatureTypeInteger},
        {ADBinX,              "BinningHorizontal",     GCFeatureTypeInteger},
        {ADBinY,              "BinningVertical",       GCFeatureTypeInteger},
        {ADNumImages,         "AcquisitionFrameCount", GCFeatureTypeInteger},
        {ADGain,              "Gain",                  GCFeatureTypeDouble},
        {ADTriggerMode,       "TriggerMode",           GCFeatureTypeEnum},
        {ADTemperatureActual, "DeviceTemperature",     GCFeatureTypeDouble},
        {GCTriggerSource,     "TriggerSource",         GCFeatureTypeEnum},
        {GCTriggerOverlap,    "TriggerOverlap",        GCFeatureTypeEnum},
        {GCTriggerSoftware,   "TriggerSoftware",       GCFeatureTypeCmd},
        {GCExposureMode,      "ExposureMode",          GCFeatureTypeEnum},
        {GCExposureAuto,      "ExposureAuto",          GCFeatureTypeEnum},
        {GCGainAuto,          "GainAuto",              GCFeatureTypeEnum},
        {GCPixelFormat,       "PixelFormat",           GCFeatureTypeEnum},
    };
    int numParams = sizeof(params)/sizeof(params[0]);
    GenICamFeature *pFeature;

    for (int i=0; i<numParams; i++) {
        asynParamType paramType;
        const char *paramName;
        getParamType(params[i].index, &paramType);
        getParamName(params[i].index, &paramName);
        pFeature = createFeature(&mGCFeatureSet, paramName, paramType, params[i].index, 
                                 params[i].featureName, params[i].featureType);
        if (pFeature->isImplemented()) {
            mGCFeatureSet.insert(pFeature, params[i].featureName);
            // We map the areaDetector ImageMode to the GenICam AcquisitionMode.
            // GenICam seems to use consistent enum strings, but not enum values
            if (pFeature->getAsynIndex() == ADImageMode) {
                std::vector<std::string> enumStrings;
                std::vector<int> enumValues;
                pFeature->readEnumChoices(enumStrings, enumValues);
                for (size_t i=0; i<enumStrings.size(); i++) {
                    if (enumStrings[i] == "Continuous")  mGCFeatureSet.mAcquisitionModeContinuous  = enumValues[i];
                    if (enumStrings[i] == "MultiFrame")  mGCFeatureSet.mAcquisitionModeMultiFrame  = enumValues[i];
                    if (enumStrings[i] == "SingleFrame") mGCFeatureSet.mAcquisitionModeSingleFrame = enumValues[i];
                }
            }
            // Do an initial read of the feature so EPICS output records initialize to this value
            pFeature->read(NULL, true);
        }
    }
    
    // Make a single parameter that maps to either AcquisitionFrameRateEnable or AcquisitionFrameRateEnabled
    createMultiFeature(GCFrameRateEnableString, asynParamInt32, GCFrameRateEnable, 
                       "AcquisitionFrameRateEnable", "AcquisitionFrameRateEnabled", GCFeatureTypeBoolean);

    // Make a single ADAcquire parameter that maps to either ExposureTime or ExposureTimeAbs
    createMultiFeature(ADAcquireTimeString, asynParamFloat64, ADAcquireTime,
                       "ExposureTime", "ExposureTimeAbs", GCFeatureTypeDouble);

    // Make a single parameter that maps to either AcquisitionFrameRate or AcquisitionFrameRateAbs
    createMultiFeature(GCFrameRateString, asynParamFloat64, GCFrameRate, 
                       "AcquisitionFrameRate", "AcquisitionFrameRateAbs", GCFeatureTypeDouble);
    createMultiFeature(ADAcquirePeriodString, asynParamFloat64, ADAcquirePeriod, 
                       "AcquisitionFrameRate", "AcquisitionFrameRateAbs", GCFeatureTypeDouble);

    // Make a single parameter that maps to either DeviceSerialNumber or DeviceID (used by AVT)
    createMultiFeature(ADSerialNumberString, asynParamOctet, ADSerialNumber,
                       "DeviceSerialNumber", "DeviceID", GCFeatureTypeString);

    return asynSuccess;
}

/* These functions convert Mono12p and Mono12Packed formats to UInt16.
  The following description of these formats is from this document:
  http://softwareservices.flir.com/BFS-U3-51S5P/latest/Model/public/ImageFormatControl.html
   
  12-bit pixel formats have two different packing formats as defined by USB3 Vision and GigE Vision. 
  Note: the packing format is not related to the interface of the camera. Both may be available on USB3 or GigE devices.

  The USB3 Vision method is designated with a p. 
  It is a 12-bit format with its bit-stream following the bit packing method illustrated in Figure 3. 
  The first byte of the packed stream contains the eight least significant bits (lsb) of the first pixel. 
  The third byte contains the eight most significant bits (msb) of the second pixel. 
  The four lsb of the second byte contains four msb of the first pixel, 
  and the rest of the second byte is packed with the four lsb of the second pixel.

  This packing format is applied to: Mono12p, BayerGR12p, BayerRG12p, BayerGB12p and BayerBG12p.

  The GigE Vision method is designated with Packed. 
  It is a 12-bit format with its bit-stream following the bit packing method illustrated in Figure 4. 
  The first byte of the packed stream contains the eight msb of the first pixel. 
  The third byte contains the eight msb of the second pixel. 
  The four lsb of the second byte contains four lsb of the first pixel, 
  and the rest of the second byte is packed with the four lsb of the second pixel.

  This packing format is applied to: Mono12Packed, BayerGR12Packed, BayerRG12Packed, BayerGB12Packed and BayerBG12Packed.
*/
void ADGenICam::decompressMono12p(int numPixels, epicsUInt8 *input, epicsUInt16 *output)
{
    /* Unpack a buffer that is packed with the USB3 Vision Mono12p compression
     * This routine assumes that the output array has already been allocated */
    int i;

    for (i=0; i<numPixels/2; i++) {
        *output++ = (*input << 4) | ((*(input+1) & 0x0f) << 12);
        *output++ = (*(input+1) & 0xf0) | (*(input+2) << 8);
        input += 3;
    }
}

void ADGenICam::decompressMono12Packed(int numPixels, epicsUInt8 *input, epicsUInt16 *output)
{
    /* Unpack a buffer that is packed with the GigE Vision Mono12Packed compression
     * This routine assumes that the output array has already been allocated */
    int i;

    for (i=0; i<numPixels/2; i++) {
        *output++ = (*input << 8) | ((*(input+1) & 0x0f) << 4);
        *output++ = (*(input+1) & 0xf0) | (*(input+2) << 8);
        input += 3;
    }
}

