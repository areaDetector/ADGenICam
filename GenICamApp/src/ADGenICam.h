#ifndef AD_GENICAM_H
#define AD_GENICAM_H

#include <ADDriver.h>

#include "ADGenICamAPI.h"
#include "GenICamFeature.h"

#define GCFrameRateString           "GC_FRAMERATE"              // asynParamFloat64, R/W
#define GCFrameRateEnableString     "GC_FRAMERATE_ENABLE"       // asynParamInt32, R/W
#define GCTriggerSourceString       "GC_TRIGGER_SOURCE"         // asynParamInt32, R/W
#define GCTriggerOverlapString      "GC_TRIGGER_OVERLAP"        // asynParamInt32, R/W
#define GCTriggerSoftwareString     "GC_TRIGGER_SOFTWARE"       // asynParamInt32, R/W
#define GCExposureModeString        "GC_EXPOSURE_MODE"          // asynParamInt32, R/W
#define GCExposureAutoString        "GC_EXPOSURE_AUTO"          // asynParamInt32, R/W
#define GCGainAutoString            "GC_GAIN_AUTO"              // asynParamInt32, R/W
#define GCPixelFormatString         "GC_PIXEL_FORMAT"           // asynParamInt32, R/W

class ADGENICAM_API ADGenICam : public ADDriver
{
public:
    ADGenICam(const char *portName, size_t maxMemory, int priority, int stackSize);

    // virtual methods to override from ADDriver
    virtual asynStatus writeInt32( asynUser *pasynUser, epicsInt32 value);
    virtual asynStatus writeInt64( asynUser *pasynUser, epicsInt64 value);
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
    virtual asynStatus pauseAcquisition();
    virtual asynStatus resumeAcquisition();
    virtual void decompressMono12p(int numPixels, epicsUInt8 *input, epicsUInt16 *output);
    virtual void decompressMono12Packed(int numPixels, epicsUInt8 *input, epicsUInt16 *output);

    // Pure virtual functions that all drivers must implement
    virtual GenICamFeature *createFeature(GenICamFeatureSet *set, 
                                          std::string const & asynName, asynParamType asynType, int asynIndex,
                                          std::string const & featureName, GCFeatureType_t featureType) = 0;
    virtual asynStatus addADDriverFeatures();
    virtual asynStatus startCapture() = 0;
    virtual asynStatus stopCapture() = 0;

protected:
    int GCFrameRate;
    int GCFrameRateEnable;
    int GCTriggerSource;
    int GCTriggerOverlap;
    int GCTriggerSoftware;
    int GCExposureMode;
    int GCExposureAuto;
    int GCGainAuto;
    int GCPixelFormat;

    GenICamFeatureSet mGCFeatureSet;
    
private:
    asynStatus createMultiFeature(std::string const & asynName, asynParamType asynType, int asynIndex,
                                  std::string const & featureName1, std::string const & featureName2, 
                                  GCFeatureType_t featureType);
    int mFirstParam;
    bool mFirstDrvUserCreateCall;
    bool mWasAcquiring;
};

#endif
