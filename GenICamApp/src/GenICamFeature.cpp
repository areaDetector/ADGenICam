#include <stdlib.h>
#include <stdexcept>

#include <epicsString.h>
#include <ADDriver.h>

#include "GenICamFeature.h"

// Error message formatters
#define ERR(msg) asynPrint(mSet->getUser(), ASYN_TRACE_ERROR,\
    "Param[%s]::%s: %s\n", \
    mAsynName.c_str(), functionName, msg)

#define ERR_ARGS(fmt,...) asynPrint(mSet->getUser(), ASYN_TRACE_ERROR, \
    "Param[%s]::%s: " fmt "\n", mAsynName.c_str(), functionName, __VA_ARGS__);

#define WARN_ARGS(fmt,...) asynPrint(mSet->getUser(), ASYN_TRACE_WARNING, \
    "Param[%s]::%s: " fmt "\n", mAsynName.c_str(), functionName, __VA_ARGS__);

// Flow message formatters
#define FLOW(msg) asynPrint(mSet->getUser(), ASYN_TRACE_FLOW, \
    "Param[%s]::%s: %s\n", \
    mAsynName.c_str(), functionName, msg)

#define FLOW_ARGS(fmt,...) asynPrint(mSet->getUser(), ASYN_TRACE_FLOW, \
    "Param[%s]::%s: " fmt "\n", mAsynName.c_str(), functionName, __VA_ARGS__);

#define TRACEIO_DRIVER_ARGS(fmt,...) asynPrint(mSet->getUser(), ASYN_TRACEIO_DRIVER, \
    "Param[%s]::%s: " fmt "\n", mAsynName.c_str(), functionName, __VA_ARGS__);

using std::string;
using std::vector;
using std::map;
using std::pair;

int GenICamFeature::getParam (int & value)
{
    return (int) mSet->getPortDriver()->getIntegerParam(mAsynIndex, &value);
}

int GenICamFeature::getParam (double & value)
{
    return (int) mSet->getPortDriver()->getDoubleParam(mAsynIndex, &value);
}

int GenICamFeature::getParam (std::string & value)
{
    return (int) mSet->getPortDriver()->getStringParam(mAsynIndex, value);
}

int GenICamFeature::setParam (int value)
{
    return (int) mSet->getPortDriver()->setIntegerParam(mAsynIndex, value);
}

int GenICamFeature::setParam (double value)
{
    return (int) mSet->getPortDriver()->setDoubleParam(mAsynIndex, value);
}

int GenICamFeature::setParam (std::string const & value)
{
    return (int) mSet->getPortDriver()->setStringParam(mAsynIndex, value);
}
GenICamFeature::GenICamFeature (GenICamFeatureSet *set, 
        string const & asynName, asynParamType asynType, int asynIndex,
        string const &featureName, GCFeatureType_t featureType)
: mAsynName(asynName), mAsynType(asynType), mAsynIndex(asynIndex),
  mFeatureName(featureName), mFeatureType(featureType), mSet(set)
{
    const char *functionName = "GenICamFeature";

    asynStatus status;

    // Check if asyn parameter already exists, create if it doesn't
    if(mAsynIndex == -1) {
        status = mSet->getPortDriver()->createParam(mAsynName.c_str(), mAsynType, &mAsynIndex);
        if(status) {
            ERR_ARGS("[param=%s] failed to create param", mAsynName.c_str());
            throw std::runtime_error(mAsynName);
        }
    }
}

int GenICamFeature::getAsynIndex (void)
{
    return mAsynIndex;
}

asynParamType GenICamFeature::getAsynType (void)
{
    return mAsynType;
}

std::string GenICamFeature::getAsynName (void)
{
    return mAsynName;
}

std::string GenICamFeature::getFeatureName (void)
{
    return mFeatureName;
}

GCFeatureType_t GenICamFeature::getFeatureType (void)
{
    return mFeatureType;
}

int GenICamFeature::write(void *pValue, void *pReadbackValue, bool bSetParam)
{
    static const char *functionName = "write";

    try {
        if (!isImplemented()) {
             WARN_ARGS("node %s is not implemented\n", mFeatureName.c_str());
             return EXIT_FAILURE;
        }
        if (!isAvailable()) {
             WARN_ARGS("node %s is not available\n", mFeatureName.c_str());
             return EXIT_FAILURE;
        }
        if (!isWritable()) {
             ERR_ARGS("node %s is not writable\n", mFeatureName.c_str());
             return EXIT_FAILURE;
        }
        switch (mFeatureType) {
            case GCFeatureTypeInteger: {
                epicsInt32 value;
                if (pValue)
                    value = *(epicsInt32*)pValue;
                else 
                    mSet->getPortDriver()->getIntegerParam(mAsynIndex, &value);
                value = convertUnits(value, GCConvertFromEPICS);
                // Check against the min and max
                int max = readIntegerMax();
                int min = readIntegerMin();
                int inc = readIncrement();
                if (inc != 1) {
                    value = (value/inc) * inc;
                }
                if (value < min) {
                   WARN_ARGS("node %s value %d is less than minimum %d, setting to minimum\n",
                             mFeatureName.c_str(), value, min);
                    value = min;
                }
                if (value > max) {
                   WARN_ARGS("node %s value %d is greater than maximum %d, setting to maximum\n",
                             mFeatureName.c_str(), value, max);
                    value = max;
                }
                writeInteger(value);
                TRACEIO_DRIVER_ARGS("set property %s to %d\n", mFeatureName.c_str(), value);
                if (isReadable()) {
                    epicsInt32 readback = readInteger();
                    readback = convertUnits(readback, GCConvertToEPICS);
                    if (pReadbackValue) *(epicsInt32*)pReadbackValue = readback;
                    TRACEIO_DRIVER_ARGS("readback property %s is %d\n", mFeatureName.c_str(), readback);
                    if (bSetParam) setParam(readback);
                }
                break;
            }
            case GCFeatureTypeBoolean: {
                epicsInt32 value;
                if (pValue) 
                    value = *(epicsInt32*)pValue;
                else
                    mSet->getPortDriver()->getIntegerParam(mAsynIndex, &value);
                bool bValue = value ? true : false;
                writeBoolean(bValue);
                TRACEIO_DRIVER_ARGS("set property %s to %s\n", mFeatureName.c_str(), bValue ? "true" : "false");
                if (isReadable()) {
                    bool readback = readBoolean();
                    if (pReadbackValue) *(epicsInt32*)pReadbackValue = readback ? 1 : 0;
                    TRACEIO_DRIVER_ARGS("readback property %s is %s\n", mFeatureName.c_str(), readback ? "true" : "false");
                    if (bSetParam) setParam(readback);
                }
                break;
            }
            case GCFeatureTypeDouble: {
                epicsFloat64 value;
                if (pValue) 
                    value = *(epicsFloat64*)pValue;
                else
                    mSet->getPortDriver()->getDoubleParam(mAsynIndex, &value);
                value = convertUnits(value, GCConvertFromEPICS);
                // Check against the min and max
                double max = readDoubleMax();
                double min = readDoubleMin();
                if (value < min) {
                    WARN_ARGS("node %s value %f is less than minimum %f, setting to minimum\n",
                              mFeatureName.c_str(), value, min);
                    value = min;
                }
                if (value > max) {
                    WARN_ARGS("node %s value %f is greater than maximum %f, setting to maximum\n",
                              mFeatureName.c_str(), value, max);
                    value = max;
                }
                writeDouble(value);
                TRACEIO_DRIVER_ARGS("set property %s to %f\n", mFeatureName.c_str(), value);
                if (isReadable()) {
                    double readback = readDouble();
                    readback = convertUnits(readback, GCConvertToEPICS);
                    if (pReadbackValue) *(epicsFloat64*)pReadbackValue = readback;
                    TRACEIO_DRIVER_ARGS("readback property %s is %f\n", mFeatureName.c_str(), readback);
                    if (bSetParam) setParam(readback);
                }
                break;
            }
            case GCFeatureTypeEnum: {
                epicsInt32 value;
                if (pValue) 
                    value = *(epicsInt32*)pValue;
                else
                    mSet->getPortDriver()->getIntegerParam(mAsynIndex, &value);
                value = convertUnits(value, GCConvertFromEPICS);
                writeEnumIndex(value);
                TRACEIO_DRIVER_ARGS("set property %s to %d\n", mFeatureName.c_str(), value);
                if (isReadable()) {
                    epicsInt32 readback = readEnumIndex();
                    readback = convertUnits(readback, GCConvertToEPICS);
                    if (pReadbackValue) *(epicsInt32*)pReadbackValue = readback;
                    TRACEIO_DRIVER_ARGS("readback property %s is %d\n", mFeatureName.c_str(), readback);
                    if (bSetParam) setParam(readback);
                }
                break;
            }
            case GCFeatureTypeString: {
                const char *value;
                if (pValue) 
                    value = (const char*)pValue;
                else {
                    std::string temp;
                    getParam(temp);
                    value = temp.c_str();
                }
                writeString(value);
                TRACEIO_DRIVER_ARGS("set property %s to %s\n", mFeatureName.c_str(), value);
                if (isReadable()) {
                    std::string readback = readString();
                    if (pReadbackValue) *(std::string*)pReadbackValue = readback;
                    TRACEIO_DRIVER_ARGS("readback property %s is %s\n", mFeatureName.c_str(), readback.c_str());
                    if (bSetParam) setParam(readback);
                }
                break;
            }
            case GCFeatureTypeCmd: {
                writeCommand();
                TRACEIO_DRIVER_ARGS("executed command %s\n", mFeatureName.c_str());
                break;
            }
            default:
                break;
        }
    }
    catch (std::exception &e) {
        ERR_ARGS("feature %s exception %s\n", mFeatureName.c_str(), e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int GenICamFeature::read(void *pValue, bool bSetParam)
{
    static const char *functionName = "read";

    if (!isImplemented()) return EXIT_FAILURE;
    try {
        if ((mFeatureType == GCFeatureTypeEnum) &&
            (!isImplemented() || !isAvailable() || !isWritable())) {
            char *enumStrings[1];
            int enumValues[1];
            int enumSeverities[1];
            enumStrings[0] = epicsStrDup("N.A.");
            enumValues[0] = 0;
            enumSeverities[0] = 0;
            mSet->getPortDriver()->doCallbacksEnum(enumStrings, enumValues, enumSeverities, 
                                                   1, mAsynIndex, 0);
        }
        if (!isImplemented()) {
             WARN_ARGS("node %s is not implemented\n", mFeatureName.c_str());
             return EXIT_FAILURE;
        }
        if (!isAvailable()) {
             WARN_ARGS("node %s is not available\n", mFeatureName.c_str());
             return EXIT_FAILURE;
        }
        if (!isReadable()) {
             WARN_ARGS("node %s is not readable\n", mFeatureName.c_str());
             return EXIT_FAILURE;
        }
        switch (mFeatureType) {
            case GCFeatureTypeInteger: {
                epicsInt32 value = readInteger();
                value = convertUnits(value, GCConvertToEPICS);
                if (pValue) *(epicsInt32*)pValue = value;
                if (bSetParam) setParam(value);
                break;
            }
            case GCFeatureTypeBoolean: {
                epicsInt32 value = (epicsInt32)readBoolean() ? 1 : 0;
                if (pValue) *(epicsInt32*)pValue = value;
                if (bSetParam) setParam(value);
                break;
            }
            case GCFeatureTypeDouble: {
                epicsFloat64 value = readDouble();
                value = convertUnits(value, GCConvertToEPICS);
                if (pValue) *(epicsFloat64*)pValue = value;
                if (bSetParam) setParam(value);
                break;
            }
            case GCFeatureTypeDoubleMin: {
                epicsFloat64 value = readDoubleMin();
                value = convertUnits(value, GCConvertToEPICS);
                setParam(value);
                break;
            }
            case GCFeatureTypeDoubleMax: {
                epicsFloat64 value = readDoubleMax();
                value = convertUnits(value, GCConvertToEPICS);
                setParam(value);
                break;
            }
            case GCFeatureTypeEnum: {
                epicsInt32 value = readEnumIndex();
                value = convertUnits(value, GCConvertToEPICS);
                if (pValue) *(epicsInt32*)pValue = value;
                if (bSetParam) setParam(value);
                // We don't want to replace enum values for EPICS IMAGE_MODE parameter
                if (mAsynName == "IMAGE_MODE") return EXIT_SUCCESS;
                std::vector<std::string> tempStrings;
                std::vector<int> tempValues;
                readEnumChoices(tempStrings, tempValues);
                int numEnums = (int)tempStrings.size();
                char **enumStrings = new char*[numEnums];
                int *enumValues = new int[numEnums];
                int *enumSeverities = new int[numEnums];
                for (int i=0; i<numEnums; i++) {
                    enumStrings[i] = epicsStrDup(tempStrings[i].c_str());
                    enumValues[i] = tempValues[i];
                    enumSeverities[i] = 0;
                }
                mSet->getPortDriver()->doCallbacksEnum(enumStrings, enumValues, enumSeverities, 
                                                     numEnums, mAsynIndex, 0);
                delete [] enumStrings; delete [] enumValues; delete [] enumSeverities;
                break;
            }
            case GCFeatureTypeString: {
                std::string value = readString().c_str();
                if (pValue) *(std::string *)pValue = value;
                if (bSetParam) setParam(value);
                break;
            }
            case GCFeatureTypeCmd: {
                break;
            }
            default:
                break;
        }
    }
    catch (std::exception &e) {
        ERR_ARGS("feature %s exception %s\n", mFeatureName.c_str(), e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}


std::string GenICamFeature::getValueAsString()
{
//    static const char *functionName = "getValueAsString";
    std::string valueString = "Not available";
    char buff[100];
    
    if (isImplemented() && isReadable()) {
        switch (mFeatureType) {
            case GCFeatureTypeString: {
                valueString = readString();
                break;
                }
            case  GCFeatureTypeInteger: {
                int temp = readInteger();
                sprintf(buff, "%d", temp);
                valueString = buff;
                break; 
                }
    
            case GCFeatureTypeDouble: {
                double temp = readDouble();
                sprintf(buff, "%f", temp);
                valueString = buff;
                break;
                }
            case GCFeatureTypeBoolean: {
                bool temp = readBoolean();
                sprintf(buff, "%s", temp ? "true" : "false");
                valueString = buff;
                break;
                }
            case GCFeatureTypeCmd: {
                valueString = "";
                break;
                }
            case GCFeatureTypeEnum: {
                valueString = readEnumString();
                break;
               }
            default:
               break; 
        }
    }
    return valueString;
}


double GenICamFeature::convertUnits(double inputValue, GCConvertDirection_t direction)
{
    double outputValue = inputValue;
    if ((mFeatureName == "ExposureTime") ||
        (mFeatureName == "ExposureTimeAbs") ||
        (mFeatureName == "TriggerDelay")) {
        // EPICS uses seconds, GenICam uses microseconds
        if (direction == GCConvertToEPICS)
            outputValue = inputValue / 1.e6;
        else
            outputValue = inputValue * 1.e6;
    } 
    else if (mAsynName == "ACQ_PERIOD") {
        // EPICS uses period in seconds, GenICam uses rate in Hz
        outputValue = 1. / inputValue;
    }
    return outputValue;
}

int GenICamFeature::convertUnits(int inputValue, GCConvertDirection_t direction)
{
    int outputValue = inputValue;
    if (mAsynName == "IMAGE_MODE") {
        // We want to use the EPICS enums
        if (direction == GCConvertToEPICS) {
            switch (inputValue) {
                case GCAcquisitionMode_SingleFrame: 
                    outputValue = ADImageSingle;
                    break;
                case GCAcquisitionMode_MultipleFrame:
                    outputValue = ADImageMultiple;
                    break;
                case GCAcquisitionMode_Continuous:
                    outputValue = ADImageContinuous;
                    break;
            }
        } else {
            switch (inputValue) {
                case ADImageSingle:
                    outputValue = GCAcquisitionMode_SingleFrame;
                    break;
                case ADImageMultiple:
                    outputValue = GCAcquisitionMode_MultipleFrame;
                    break;
                case ADImageContinuous:
                    outputValue = GCAcquisitionMode_Continuous;
                    break;
            }
        }
    }
    return outputValue;
}

GenICamFeatureSet::GenICamFeatureSet (asynPortDriver *portDriver, asynUser *user)
: mPortDriver(portDriver), mUser(user), mFeatureMap(), mAsynMap()
{}

void GenICamFeatureSet::insert(GenICamFeature *pFeature, string const & name)
{
    if(!name.empty())
        mFeatureMap.insert(std::make_pair(name, pFeature));

    mAsynMap.insert(std::make_pair(pFeature->getAsynIndex(), pFeature));
}

asynPortDriver *GenICamFeatureSet::getPortDriver (void)
{
    return mPortDriver;
}

GenICamFeature *GenICamFeatureSet::getByName (string const & name)
{
    GCFeatureMap_t::iterator item(mFeatureMap.find(name));

    if(item != mFeatureMap.end())
        return item->second;
    return NULL;
}

GenICamFeature *GenICamFeatureSet::getByIndex (int index)
{
    GCAsynMap_t::iterator item(mAsynMap.find(index));

    if(item != mAsynMap.end())
        return item->second;
    return NULL;
}

asynUser *GenICamFeatureSet::getUser (void)
{
    return mUser;
}

int GenICamFeatureSet::readAll (void)
{
    int status = EXIT_SUCCESS;

    GCAsynMap_t::iterator it;
    for(it = mAsynMap.begin(); it != mAsynMap.end(); ++it)
        status |= it->second->read(0, true);

    return status;
}

int GenICamFeatureSet::readFeatures (vector<string> const & params)
{
    int status = EXIT_SUCCESS;
    vector<string>::const_iterator param;

    for(param = params.begin(); param != params.end(); ++param)
    {
        GenICamFeature *p = getByName(*param);
        if(p)
            status |= p->read(0, true);
    }

    return status;
}

void GenICamFeatureSet::report (FILE *fp, int details)
{
    // Print out feature set
    GenICamFeature *p;
    GCFeatureMap_t::iterator it;
    fprintf(fp, "Feature list\n");
    for (it=mFeatureMap.begin(); it != mFeatureMap.end(); it++) {
        p = it->second;
        fprintf(fp, "\n");
        fprintf(fp, "      Node name: %s\n",   p->getFeatureName().c_str());
        fprintf(fp, "          value: %s\n",   p->getValueAsString().c_str());
        if (details > 1) {
            fprintf(fp, "      asynIndex: %d\n",   p->getAsynIndex());
            fprintf(fp, "       asynName: %s\n",   p->getAsynName().c_str());
            fprintf(fp, "       asynType: %d\n",   p->getAsynType());
            fprintf(fp, "  isImplemented: %s\n",   p->isImplemented() ? "true" : "false");
            fprintf(fp, "    isAvailable: %s\n",   p->isAvailable()   ? "true" : "false");
            fprintf(fp, "     isReadable: %s\n",   p->isReadable()    ? "true" : "false");
            fprintf(fp, "     isWritable: %s\n",   p->isWritable()    ? "true" : "false");
       }
    }
}
