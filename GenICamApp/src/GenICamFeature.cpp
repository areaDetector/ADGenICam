#include <stdlib.h>
#include <vector>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <limits>

#include <ADDriver.h>
#include <math.h>
#include "GenICamFeature.h"

// Error message formatters
#define ERR(msg) asynPrint(mSet->getUser(), ASYN_TRACE_ERROR,\
    "Param[%s]::%s: %s\n", \
    mAsynName.c_str(), functionName, msg)

#define ERR_ARGS(fmt,...) asynPrint(mSet->getUser(), ASYN_TRACE_ERROR, \
    "Param[%s]::%s: "fmt"\n", mAsynName.c_str(), functionName, __VA_ARGS__);

// Flow message formatters
#define FLOW(msg) asynPrint(mSet->getUser(), ASYN_TRACE_FLOW, \
    "Param[%s]::%s: %s\n", \
    mAsynName.c_str(), functionName, msg)

#define FLOW_ARGS(fmt,...) asynPrint(mSet->getUser(), ASYN_TRACE_FLOW, \
    "Param[%s]::%s: "fmt"\n", mAsynName.c_str(), functionName, __VA_ARGS__);


#define MAX_BUFFER_SIZE 128
#define MAX_MESSAGE_SIZE 512
#define MAX_JSON_TOKENS 100

using std::string;
using std::vector;
using std::map;
using std::pair;

int GenICamFeature::getEnumIndex (std::string const & value, size_t & index)
{
    const char *functionName = "getEnumIndex";
    for(index = 0; index < mEnumValues.size(); ++index)
        if(mEnumValues[index] == value)
            return EXIT_SUCCESS;

    ERR_ARGS("[param=%s] can't find index of value %s", mAsynName.c_str(),
            value.c_str());
    return EXIT_FAILURE;
}

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


GenICamFeature::GenICamFeature (GenICamFeatureSet *set, string const & asynName,
        asynParamType asynType, string const & name)
: mSet(set), mAsynName(asynName), mAsynType(asynType),
  mName(name), mRemote(!mName.empty()), mAsynIndex(-1),
  mType(GCFeatureTypeUnknown), mAccessMode(), mMin(), mMax(), mEnumValues(),
  mCustomEnum(false)
{
    const char *functionName = "GenICamFeature";

    asynStatus status;

    // Check if asyn parameter already exists, create if it doesn't
    if(mSet->getPortDriver()->findParam(mAsynName.c_str(), &mAsynIndex))
    {
        status = mSet->getPortDriver()->createParam(mAsynName.c_str(), mAsynType, &mAsynIndex);
        if(status)
        {
            ERR_ARGS("[param=%s] failed to create param", mAsynName.c_str());
            throw std::runtime_error(mAsynName);
        }
    }

    if(mName.empty())
    {
        switch(asynType)
        {
        case asynParamInt32:   mType = GCFeatureTypeInt;    break;
        case asynParamFloat64: mType = GCFeatureTypeDouble; break;
        case asynParamOctet:   mType = GCFeatureTypeString; break;
        default:
            ERR_ARGS("[param=%s] invalid asyn type %d", mAsynName.c_str(),
                    (int)asynType);
            throw std::runtime_error(mAsynName);
        }
    }
}

int GenICamFeature::getIndex (void)
{
    return mAsynIndex;
}

void GenICamFeature::setEnumValues (vector<string> const & values)
{
    mEnumValues = values;
    mCustomEnum = true;
}

int GenICamFeature::get (bool & value)
{
    if(mAsynType == asynParamInt32)
    {
        int tempValue;
        getParam(tempValue);
        value = (bool) tempValue;
    }
    else if(mAsynType == asynParamOctet && mEnumValues.size() == 2)
    {
        int tempValue;
        if(get(tempValue))
            return EXIT_FAILURE;
        value = (bool) tempValue;
    }
    else
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

int GenICamFeature::get (int & value)
{
    if(mAsynType == asynParamInt32)
        getParam(value);
    else if(mAsynType == asynParamOctet && mEnumValues.size())
    {
        string tempValue;
        getParam(tempValue);

        size_t index;
        if(getEnumIndex(tempValue, index))
            return EXIT_FAILURE;

        value = (int) index;
    }
    else
        return EXIT_FAILURE;
    return EXIT_SUCCESS;
}

int GenICamFeature::get (double & value)
{
    return (int) getParam(value);
}

int GenICamFeature::get (std::string & value)
{
    if(mType == GCFeatureTypeEnum && mAsynType == asynParamInt32)
    {
        int index;
        int status = getParam(index);
        value = mEnumValues[index];
        return status;
    }

    return (int) getParam(value);
}

int GenICamFeature::put (bool value)
{
    const char *functionName = "put<bool>";
    FLOW_ARGS("%d", value);
    if(!mRemote)
    {
        setParam((int)value);
        return EXIT_SUCCESS;
    }

    if(mType == GCFeatureTypeUnknown && fetch())
        return EXIT_FAILURE;

    if(mType != GCFeatureTypeBoolean && mType != GCFeatureTypeEnum)
        return EXIT_FAILURE;

    int status = EXIT_SUCCESS;
    if(mType == GCFeatureTypeEnum)
    {
        if(mEnumValues.size() != 2)
            return EXIT_FAILURE;
    }

    if(status)
        return EXIT_FAILURE;

    if(setParam(value))
    {
        ERR_ARGS("[param=%s] failed to set asyn parameter",
                mAsynName.c_str());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int GenICamFeature::put (int value)
{
    const char *functionName = "put<int>";
    int status = EXIT_SUCCESS;
    FLOW_ARGS("%d", value);
    if(mRemote)
    {
        if(mType == GCFeatureTypeUnknown && fetch())
            return EXIT_FAILURE;

        if(mType != GCFeatureTypeBoolean && mType != GCFeatureTypeInt &&
           mType != GCFeatureTypeInt && mType != GCFeatureTypeEnum &&
           mType != GCFeatureTypeCmd)
        {
            ERR_ARGS("[param=%s] expected bool, int, uint or enum",
                    mAsynName.c_str());
            return EXIT_FAILURE;
        }

        // Clamp value
        if(mMin.exists && value < mMin.valInt)
            value = mMin.valInt;

        if(mMax.exists && value > mMax.valInt)
            value = mMax.valInt;

        // Protect against trying to write negative values to an unsigned int
        if(mType == GCFeatureTypeInt && value < 0)
            value = 0;


        if(status)
        {
            ERR_ARGS("[param=%s] underlying basePut failed",
                    mAsynName.c_str());
            return EXIT_FAILURE;
        }
    }

    if(mAsynType == asynParamInt32)
        status = setParam(value);
    else
        status = setParam(mEnumValues[value]);

    if(status)
    {
        ERR_ARGS("[param=%s] failed to set asyn parameter",
                mAsynName.c_str());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int GenICamFeature::put (double value)
{
    const char *functionName = "put<double>";
    FLOW_ARGS("%lf", value);

    if(mRemote)
    {
        if(mType == GCFeatureTypeUnknown && fetch())
            return EXIT_FAILURE;

        if(mType != GCFeatureTypeDouble)
            return EXIT_FAILURE;

        // Clamp value
        if(mMin.exists && (value < mMin.valDouble))
        {
            value = mMin.valDouble;
            ERR_ARGS("clamped to min %lf", value);
        }

        if(mMax.exists && (value > mMax.valDouble))
        {
            value = mMax.valDouble;
            ERR_ARGS("clamped to max %lf", value);
        }

    }

    if(setParam(value))
    {
        ERR_ARGS("[param=%s] failed to set asyn parameter",
                mAsynName.c_str());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int GenICamFeature::put (string const & value)
{
    const char *functionName = "put<string>";
    FLOW_ARGS("%s", value.c_str());
    if(!mRemote)
    {
        setParam(value);
        return EXIT_SUCCESS;
    }

    if(mType == GCFeatureTypeUnknown && fetch())
        return EXIT_FAILURE;

    if(mType != GCFeatureTypeString && mType != GCFeatureTypeEnum)
        return EXIT_FAILURE;

    size_t index;
    if(mType == GCFeatureTypeEnum && getEnumIndex(value, index))
        return EXIT_FAILURE;

    int status;
    if(mAsynType == asynParamInt32)
        status = setParam((int)index);
    else
        status = setParam(value);

    if(status)
    {
        ERR_ARGS("[param=%s] failed to set asyn parameter",
                mAsynName.c_str());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int GenICamFeature::put (const char * value)
{
    return put(string(value));
}

GenICamFeatureSet::GenICamFeatureSet (asynPortDriver *portDriver,
        asynUser *user)
: mPortDriver(portDriver), mUser(user), mFeatureMap(), mAsynMap()
{}

void GenICamFeatureSet::insert(GenICamFeature *pFeature, string const & name)
{
    if(!name.empty())
        mFeatureMap.insert(std::make_pair(name, pFeature));

    mAsynMap.insert(std::make_pair(pFeature->getIndex(), pFeature));
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

int GenICamFeatureSet::fetchAll (void)
{
    int status = EXIT_SUCCESS;

    GCAsynMap_t::iterator it;
    for(it = mAsynMap.begin(); it != mAsynMap.end(); ++it)
        status |= it->second->fetch();

    return status;
}

int GenICamFeatureSet::fetchParams (vector<string> const & params)
{
    int status = EXIT_SUCCESS;
    vector<string>::const_iterator param;

    for(param = params.begin(); param != params.end(); ++param)
    {
        GenICamFeature *p = getByName(*param);
        if(p)
            status |= p->fetch();
    }

    return status;
}
