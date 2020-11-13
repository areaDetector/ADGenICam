#include <stdlib.h>
#include <stdexcept>

#include <epicsString.h>
#include <ADDriver.h>

#include <epicsExport.h>
#include "GenICamFeature.h"

// Error message formatters
#define ERR(msg) asynPrint(mSet->getUser(), ASYN_TRACE_ERROR,\
    "Param[%s] %s: %s\n", \
    mAsynName.c_str(), functionName, msg)

#define ERR_ARGS(fmt,...) asynPrint(mSet->getUser(), ASYN_TRACE_ERROR, \
    "Param[%s] %s: " fmt "\n", mAsynName.c_str(), functionName, __VA_ARGS__);

#define WARN_ARGS(fmt,...) asynPrint(mSet->getUser(), ASYN_TRACE_WARNING, \
    "Param[%s] %s: " fmt "\n", mAsynName.c_str(), functionName, __VA_ARGS__);

// Flow message formatters
#define FLOW(msg) asynPrint(mSet->getUser(), ASYN_TRACE_FLOW, \
    "Param[%s] %s: %s\n", \
    mAsynName.c_str(), functionName, msg)

#define FLOW_ARGS(fmt,...) asynPrint(mSet->getUser(), ASYN_TRACE_FLOW, \
    "Param[%s] %s: " fmt "\n", mAsynName.c_str(), functionName, __VA_ARGS__);

#define TRACEIO_DRIVER_ARGS(fmt,...) asynPrint(mSet->getUser(), ASYN_TRACEIO_DRIVER, \
    "Param[%s] %s: " fmt "\n", mAsynName.c_str(), functionName, __VA_ARGS__);

using std::string;
using std::vector;
using std::map;
using std::pair;

int GenICamFeature::getParam (epicsInt64 & value)
{
    if (mAsynType == asynParamInt32) {
        epicsInt32 temp;
        int status = mSet->getPortDriver()->getIntegerParam(mAsynIndex, &temp);
        value = temp;
        return status;
    }
    return (int) mSet->getPortDriver()->getInteger64Param(mAsynIndex, &value);

}

int GenICamFeature::getParam (epicsInt32& value)
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

int GenICamFeature::setParam (epicsInt64 value)
{
    if (mAsynType == asynParamInt32) {
        return (int) mSet->getPortDriver()->setIntegerParam(mAsynIndex, (epicsInt32)value);
    } 
    return (int) mSet->getPortDriver()->setInteger64Param(mAsynIndex, value);
}

int GenICamFeature::setParam (epicsInt32 value)
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

int GenICamFeature::setParam (bool value)
{
    return (int) mSet->getPortDriver()->setIntegerParam(mAsynIndex, (int) value);
}

GenICamFeature::GenICamFeature (GenICamFeatureSet *set, 
        string const & asynName, asynParamType asynType, int asynIndex,
        string const &featureName, GCFeatureType_t featureType)
: mAsynName(asynName), mAsynType(asynType), mAsynIndex(asynIndex),
  mFeatureName(featureName), mFeatureType(featureType), mImageMode(0), mSet(set)
{
    const char *functionName = "GenICamFeature";

    asynStatus status;

    // Check if asyn parameter already exists, create if it doesn't
    if(mAsynIndex == -1) {
        status = mSet->getPortDriver()->createParam(mAsynName.c_str(), mAsynType, &mAsynIndex);
        if (status) {
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
    static const char *functionName = "GenICamFeature::write";

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
                epicsInt64 value;
                if (pValue)
                    value = *(epicsInt64*)pValue;
                else 
                    getParam(value);
                // Check against the min and max
                epicsInt64 max = readIntegerMax();
                epicsInt64 min = readIntegerMin();
                epicsInt64 inc = readIncrement();
                if (inc != 1) {
                    value = (value/inc) * inc;
                }
                if (value < min) {
                   WARN_ARGS("node %s value %lld is less than minimum %lld, setting to minimum\n",
                             mFeatureName.c_str(), value, min);
                    value = min;
                }
                if (value > max) {
                   WARN_ARGS("node %s value %lld is greater than maximum %lld, setting to maximum\n",
                             mFeatureName.c_str(), value, max);
                    value = max;
                }
                writeInteger(value);
                TRACEIO_DRIVER_ARGS("set property %s to %lld\n", mFeatureName.c_str(), value);
                if (isReadable()) {
                    epicsInt64 readback = readInteger();
                    if (pReadbackValue) *(epicsInt64*)pReadbackValue = readback;
                    TRACEIO_DRIVER_ARGS("readback property %s is %lld\n", mFeatureName.c_str(), readback);
                    if (bSetParam) setParam(readback);
                }
                break;
            }
            case GCFeatureTypeBoolean: {
                epicsInt32 value;
                if (pValue) 
                    value = *(epicsInt32*)pValue;
                else
                    getParam(value);
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
                    getParam(value);
                value = convertDoubleUnits(value, GCConvertFromEPICS);
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
                    readback = convertDoubleUnits(readback, GCConvertToEPICS);
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
                    getParam(value);
                value = convertEnum(value, GCConvertFromEPICS);
                writeEnumIndex(value);
                TRACEIO_DRIVER_ARGS("set property %s to %d\n", mFeatureName.c_str(), value);
                if (isReadable()) {
                    epicsInt32 readback = readEnumIndex();
                    readback = convertEnum(readback, GCConvertToEPICS);
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
    static const char *functionName = "GenICamFeature::read";

    if (!isImplemented()) return EXIT_FAILURE;
    FLOW_ARGS("reading %s", mFeatureName.c_str());
    try {
        if ((mFeatureType == GCFeatureTypeEnum) && (mAsynName != "IMAGE_MODE") &&
            (!isImplemented() || !isAvailable() || !isWritable())) {
            if (mEnumStrings.empty() || (mEnumStrings[0] != "N.A.")) {
                mEnumStrings.clear();
                mEnumStrings.push_back("N.A.");
                mEnumValues.clear();
                mEnumValues.push_back(0);
                char *enumStrings[1];
                int enumValues[1];
                int enumSeverities[1];
                enumStrings[0] = epicsStrDup("N.A.");
                enumValues[0] = 0;
                enumSeverities[0] = 0;
                mSet->getPortDriver()->doCallbacksEnum(enumStrings, enumValues, enumSeverities, 
                                                       1, mAsynIndex, 0);
            }
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
                epicsInt64 value = readInteger();
                if (pValue) *(epicsInt64*)pValue = value;
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
                value = convertDoubleUnits(value, GCConvertToEPICS);
                if (pValue) *(epicsFloat64*)pValue = value;
                if (bSetParam) setParam(value);
                break;
            }
            case GCFeatureTypeDoubleMin: {
                epicsFloat64 value = readDoubleMin();
                value = convertDoubleUnits(value, GCConvertToEPICS);
                setParam(value);
                break;
            }
            case GCFeatureTypeDoubleMax: {
                epicsFloat64 value = readDoubleMax();
                value = convertDoubleUnits(value, GCConvertToEPICS);
                setParam(value);
                break;
            }
            case GCFeatureTypeEnum: {
                epicsInt32 value = readEnumIndex();
                value = convertEnum(value, GCConvertToEPICS);
                if (pValue) *(epicsInt32*)pValue = value;
                if (bSetParam) setParam(value);
                // We don't want to replace enum values for EPICS IMAGE_MODE parameter
                if (mAsynName == "IMAGE_MODE") return EXIT_SUCCESS;
                std::vector<std::string> tempStrings;
                std::vector<int> tempValues;
                readEnumChoices(tempStrings, tempValues);
                if ((tempStrings != mEnumStrings) || (tempValues != mEnumValues)) {
                    mEnumStrings = tempStrings;
                    mEnumValues = tempValues;
                    int numEnums = (int)mEnumStrings.size();
                    char **enumStrings = new char*[numEnums];
                    int *enumValues = new int[numEnums];
                    int *enumSeverities = new int[numEnums];
                    for (int i=0; i<numEnums; i++) {
                        enumStrings[i] = epicsStrDup(mEnumStrings[i].c_str());
                        enumValues[i] = mEnumValues[i];
                        enumSeverities[i] = 0;
                    }
                    mSet->getPortDriver()->doCallbacksEnum(enumStrings, enumValues, enumSeverities, 
                                                           numEnums, mAsynIndex, 0);
                    delete [] enumStrings; delete [] enumValues; delete [] enumSeverities;
                }
                break;
            }
            case GCFeatureTypeString: {
                std::string value = readString();
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
//    static const char *functionName = "GenICamFeature::getValueAsString";
    std::string valueString = "Not available";
    char buff[100];
    
    if (isImplemented() && isReadable()) {
        switch (mFeatureType) {
            case GCFeatureTypeString: {
                valueString = readString();
                break;
                }
            case  GCFeatureTypeInteger: {
                epicsInt64 temp = readInteger();
                sprintf(buff, "%lld", temp);
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


double GenICamFeature::convertDoubleUnits(double inputValue, GCConvertDirection_t direction)
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

int GenICamFeature::convertEnum(epicsInt32 inputValue, GCConvertDirection_t direction)
{
    epicsInt32 outputValue = inputValue;
    if (mAsynName == "IMAGE_MODE") {
        // We want to use the EPICS enums
        // Cannot use switch because the things we are testing are not constants
        if (direction == GCConvertToEPICS) {
            if (inputValue == mSet->mAcquisitionModeSingleFrame) {
                outputValue = ADImageSingle;
            } 
            else if (inputValue == mSet->mAcquisitionModeMultiFrame) {
                outputValue = ADImageMultiple;
            }
            else if (inputValue == mSet->mAcquisitionModeContinuous) {
                outputValue = ADImageContinuous;
            }
            // If MultiFrame is not supported then we can't use readback.
            // Use the value that was last stored when converting from EPICS
            if (mSet->mAcquisitionModeMultiFrame == -1) {
                outputValue = mImageMode;
            }
        } else {
            switch (inputValue) {
                case ADImageSingle:
                    outputValue = mSet->mAcquisitionModeSingleFrame;
                    break;
                case ADImageMultiple:
                    // Some cameras, e.g. JAI don't support MultiFrame so we convert to Continuous
                    if (mSet->mAcquisitionModeMultiFrame != -1) {
                        outputValue = mSet->mAcquisitionModeMultiFrame;
                    } else {
                        outputValue = mSet->mAcquisitionModeContinuous;
                    }
                    break;
                case ADImageContinuous:
                    outputValue = mSet->mAcquisitionModeContinuous;
                    break;
            }
            // Need to store the mode that was set because readback won't work if not all modes are supported
            mImageMode = inputValue;
        }
    }
    return outputValue;
}

GenICamFeatureSet::GenICamFeatureSet (asynPortDriver *portDriver, asynUser *user)
: mPortDriver(portDriver), mUser(user), mFeatureMap(), mAsynMap(),
  mAcquisitionModeSingleFrame(-1), mAcquisitionModeMultiFrame(-1), mAcquisitionModeContinuous(-1)
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

void GenICamFeature::report (FILE *fp, int details)
{
    fprintf(fp, "      Node name: %s\n",   mFeatureName.c_str());
    fprintf(fp, "          value: %s\n",   getValueAsString().c_str());
    if (details > 1) {
        fprintf(fp, "      asynIndex: %d\n",   mAsynIndex);
        fprintf(fp, "       asynName: %s\n",   mAsynName.c_str());
        fprintf(fp, "       asynType: %d\n",   mAsynType);
        fprintf(fp, "  isImplemented: %s\n",   isImplemented() ? "true" : "false");
        fprintf(fp, "    isAvailable: %s\n",   isAvailable()   ? "true" : "false");
        fprintf(fp, "     isReadable: %s\n",   isReadable()    ? "true" : "false");
        fprintf(fp, "     isWritable: %s\n",   isWritable()    ? "true" : "false");
        if ((mFeatureType == GCFeatureTypeInteger) && isReadable()) {
            fprintf(fp, "        minimum: %lld\n",   readIntegerMin());
            fprintf(fp, "        maximum: %lld\n",   readIntegerMax());
            fprintf(fp, "      increment: %lld\n",   readIncrement());
        }
        if ((mFeatureType == GCFeatureTypeDouble) && isReadable()) {
            fprintf(fp, "        minimum: %f\n",   readDoubleMin());
            fprintf(fp, "        maximum: %f\n",   readDoubleMax());
        }
        if ((mFeatureType == GCFeatureTypeEnum) && (mEnumStrings.size() > 0)) {
            fprintf(fp, "          enums: %d: %s\n", mEnumValues[0], mEnumStrings[0].c_str());
            for (size_t i=1; i<mEnumStrings.size(); i++) {
                fprintf(fp, "                 %d: %s\n", mEnumValues[i], mEnumStrings[i].c_str());
            }
        }
    }
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
        p->report(fp, details);
    }
}
