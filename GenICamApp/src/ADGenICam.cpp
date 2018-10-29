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
            asynEnumMask, asynEnumMask,
            ASYN_CANBLOCK, 1, priority, stackSize),
    mGCFeatureSet(this, pasynUserSelf)
{
    //static const char *functionName = "ADGenICam";
    
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
  * function codes make calls to the Spinnaker library from this function. */

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
             (function == ADBinY)        ||
             (function == ADImageMode)   ||
             (function == ADNumImages)   ||
             (function == NDDataType)) {    
        status = setImageParams();
    } 
    else if (function == ADReadStatus) {
        status = readStatus();
    } 
    GenICamFeature *pFeature = mGCFeatureSet.getByIndex(function);
    if (pFeature) {
        pFeature->write(&value, NULL, true);
        mGCFeatureSet.readAll();
    }

    asynPrint(pasynUserSelf, ASYN_TRACEIO_DRIVER, 
        "%s::%s function=%d, value=%d, status=%d\n",
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
    if (!pFeature->isAvailable() || !pFeature->isWritable()) {
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


asynStatus ADGenICam::setImageParams()
{
    std::vector<int> paramIndices;
    GenICamFeature *pFeature;
    //static const char *functionName = "setImageParams";
    //bool resumeAcquire;
    unsigned i;
    //if (!pCamera_) return asynError;
    
    paramIndices.push_back(ADImageMode);
    paramIndices.push_back(ADSizeX);
    paramIndices.push_back(ADSizeY);
    paramIndices.push_back(ADMinX);
    paramIndices.push_back(ADMinY);
    paramIndices.push_back(ADBinX);
    paramIndices.push_back(ADBinY);

    for(i=0; i<paramIndices.size(); i++) {
        pFeature = mGCFeatureSet.getByIndex(paramIndices[i]);
        if (pFeature) pFeature->write(0, 0, false);
    }

    for(i=0; i<paramIndices.size(); i++) {
        pFeature = mGCFeatureSet.getByIndex(paramIndices[i]);
        if (pFeature) pFeature->read(0, true);
    }
 
    return asynSuccess;
}


asynStatus ADGenICam::readStatus()
{
//    static const char *functionName = "readStatus";

//        const TransportLayerStream& camInfo = pCamera_->TLStream;
//  		  cout << "Stream ID: " << camInfo.StreamID.ToString() << endl;
//	  	  cout << "Stream Type: " << camInfo.StreamType.ToString() << endl;
//		    cout << "Stream Buffer Count: " << camInfo.StreamDefaultBufferCount.ToString() << endl;
//		    cout << "Stream Buffer Handling Mode: " << camInfo.StreamBufferHandlingMode.ToString() << endl;
//        cout << "Stream Packets Received: " << camInfo.GevTotalPacketCount.ToString() << endl;
//        getSPProperty(ADTemperatureActual);
//printf("StreamBufferUnderrunCount = %d\n", (int)camInfo.StreamBufferUnderrunCount.GetValue());
//        setIntegerParam(SPBufferUnderrunCount, (int)camInfo.StreamBufferUnderrunCount.GetValue());
//        setIntegerParam(SPFailedBufferCount,   (int)camInfo.StreamFailedBufferCount.GetValue());
//        if (camInfo.StreamType.GetIntValue() == StreamType_GEV) {
//printf("GeVFailedPacketCount = %d\n", (int)camInfo.GevFailedPacketCount.GetValue());
//            setIntegerParam(SPFailedPacketCount,   (int)camInfo.GevFailedPacketCount.GetValue());
//printf("GeVTotalPacketCount = %d\n", (int)camInfo.GevTotalPacketCount.GetValue());
//        }
//    }
//    callParamCallbacks();
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

    mGCFeatureSet.report(fp, details);
    ADDriver::report(fp, details);
    return;
}

asynStatus ADGenICam::drvUserCreate(asynUser *pasynUser, const char *drvInfo,
                                     const char **pptypeName, size_t *psize) {
    const char *functionName = "drvUserCreate";
    /*printf("drvUserCreate(pasynUser=%p, drvInfo=%s, pptypeName=%p, psize=%p)\n",
            pasynUser, drvInfo, pptypeName, psize);*/
    int index;

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
            asynType = asynParamInt32;
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

        GenICamFeature *p = createFeature(&mGCFeatureSet, drvInfo, asynType, featureName, featureType);
        if (!p)
            return asynError;

        // Map some asyn indices into the base class parameters
        p->mapAsynIndex(ADImageMode,         "AcquisitionMode");
        p->mapAsynIndex(ADSerialNumber,      "DeviceSerialNumber");
        p->mapAsynIndex(ADFirmwareVersion,   "DeviceFirmwareVersion");
        p->mapAsynIndex(ADManufacturer,      "DeviceVendorName");
        p->mapAsynIndex(ADModel,             "DeviceModelName");
        p->mapAsynIndex(ADMaxSizeX,          "WidthMax");
        p->mapAsynIndex(ADMaxSizeY,          "HeightMax");
        p->mapAsynIndex(ADSizeX,             "Width");
        p->mapAsynIndex(ADSizeY,             "Height");
        p->mapAsynIndex(ADMinX,              "OffsetX");
        p->mapAsynIndex(ADMinY,              "OffsetY");
        p->mapAsynIndex(ADBinX,              "BinningHorizontal");
        p->mapAsynIndex(ADBinY,              "BinningVertical");
        p->mapAsynIndex(ADNumImages,         "AcquisitionFrameCount");
        p->mapAsynIndex(ADAcquireTime,       "ExposureTime");
        p->mapAsynIndex(ADAcquirePeriod,     "AcquisitionFrameRate");
        p->mapAsynIndex(ADGain,              "Gain");
        p->mapAsynIndex(ADTriggerMode,       "TriggerMode");
        p->mapAsynIndex(ADTemperatureActual, "DeviceTemperature");

        mGCFeatureSet.insert(p, featureName);
    }
    return ADDriver::drvUserCreate(pasynUser, drvInfo, pptypeName, psize);
}
