#ifndef AD_GENICAM_H
#define AD_GENICAM_H

#include <ADDriver.h>

#include "GenICamFeature.h"

class ADGenICam : public ADDriver
{
public:
    ADGenICam(const char *portName, size_t maxMemory, int priority, int stackSize);

    // virtual methods to override from ADDriver
    virtual asynStatus writeInt32( asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeFloat64( asynUser *pasynUser, epicsFloat64 value);
    //virtual asynStatus writeOctet(asynUser *pasynUser, const char *value,
    //                              size_t nChars, size_t *nActual);
    virtual asynStatus readEnum(asynUser *pasynUser, char *strings[], int values[], int severities[], 
                                size_t nElements, size_t *nIn);
    void report(FILE *fp, int details);
    virtual asynStatus drvUserCreate(asynUser *pasynUser, const char *drvInfo,
                                     const char **pptypeName, size_t *psize);
    virtual asynStatus readStatus();
    virtual asynStatus setImageParams();

    // Pure virtual functions that all drivers must implement
    virtual GenICamFeature *createFeature(GenICamFeatureSet *set, 
                                          std::string const & asynName, asynParamType asynType, int asynIndex,
                                          std::string const & featureName, GCFeatureType_t featureType) = 0;
    virtual asynStatus addADDriverFeatures();
    virtual asynStatus startCapture() = 0;
    virtual asynStatus stopCapture() = 0;

protected:
    GenICamFeatureSet mGCFeatureSet;
    
private:
    int mFirstParam;
    bool mFirstDrvUserCreateCall;

};

#endif

