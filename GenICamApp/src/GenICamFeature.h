#ifndef GENICAM_FEATURE_H
#define GENICAM_FEATURE_H

#include <string>
#include <vector>
#include <map>
#include <asynPortDriver.h>

typedef enum 
{
    GCFeatureTypeInteger,
    GCFeatureTypeBoolean,
    GCFeatureTypeEnum,
    GCFeatureTypeDouble,
    GCFeatureTypeDoubleMin,
    GCFeatureTypeDoubleMax,
    GCFeatureTypeString,
    GCFeatureTypeCmd,
    GCFeatureTypeUnknown
} GCFeatureType_t;

typedef enum 
{
    GCConvertToEPICS,
    GCConvertFromEPICS,
} GCConvertDirection_t;

typedef enum {
    GCAcquisitionMode_Continuous,
    GCAcquisitionMode_SingleFrame,
    GCAcquisitionMode_MultipleFrame,
} GCAcquisitionMode_t;


class GenICamFeatureSet;

class epicsShareClass GenICamFeature
{

private:
    int getParam (epicsInt64 & value);
    int getParam (epicsInt32 & value);
    int getParam (double & value);
    int getParam (std::string & value);

    int setParam (epicsInt64 value);
    int setParam (epicsInt32 value);
    int setParam (double value);
    int setParam (std::string const & value);
    int setParam (bool value);

protected:
    std::string mAsynName;
    asynParamType mAsynType;
    int mAsynIndex;
    std::string mFeatureName;
    GCFeatureType_t mFeatureType;
    std::vector<std::string> mEnumStrings;
    std::vector<int> mEnumValues;
    int mImageMode;

    GenICamFeatureSet *mSet;

public:
    GenICamFeature (GenICamFeatureSet *set, 
                    std::string const & asynName, asynParamType asynType, int asynIndex,
                    std::string const & featureName, GCFeatureType_t featureType);

    // These are the pure virtual functions that derived classes must implement
    virtual bool isImplemented(void) = 0;
    virtual bool isAvailable(void) = 0;
    virtual bool isReadable(void) = 0;
    virtual bool isWritable(void) = 0;
    virtual epicsInt64 readInteger(void) = 0;
    virtual epicsInt64 readIntegerMin(void) = 0;
    virtual epicsInt64 readIntegerMax(void) = 0;
    virtual epicsInt64 readIncrement(void) = 0;
    virtual void writeInteger(epicsInt64 value) = 0;
    virtual bool readBoolean(void) = 0;
    virtual void writeBoolean (bool value) = 0;
    virtual double readDouble(void) = 0;
    virtual double readDoubleMin(void) = 0;
    virtual double readDoubleMax(void) = 0;
    virtual void writeDouble(double value) = 0;
    virtual int readEnumIndex(void) = 0;
    virtual void writeEnumIndex(int value) = 0;
    virtual std::string readEnumString(void) = 0;
    virtual void writeEnumString(std::string const & value) = 0;
    virtual void readEnumChoices(std::vector<std::string>& enumStrings, std::vector<int>& enumValues) = 0;
    virtual std::string readString(void) = 0;
    virtual void writeString(std::string const & value) = 0;
    virtual void writeCommand(void) = 0;
   
    // Put the value both to the detector (if it is connected to a detector
    // parameter) and to the underlying asyn parameter if successful. Update
    // other modified parameters automatically.
    int write(void *pValue, void *pReadbackValue, bool setParam);

    // Fetch the current value from the detector, update underlying asyn parameter
    // and return the value
    int read(void *pValue, bool bSetParam);

    void report (FILE *fp, int details);

    int getAsynIndex(void);
    std::string getAsynName(void);
    asynParamType getAsynType(void);
    std::string getFeatureName(void);
    std::string getValueAsString(void);
    GCFeatureType_t getFeatureType(void); 

    virtual epicsInt32 convertEnum(epicsInt32 inputValue, GCConvertDirection_t direction);
    virtual double convertDoubleUnits(double inputValue, GCConvertDirection_t direction);
};

typedef std::multimap<std::string, GenICamFeature*> GCFeatureMap_t;
typedef std::map<int, GenICamFeature*> GCAsynMap_t;

class epicsShareClass GenICamFeatureSet
{
private:
    asynPortDriver *mPortDriver;
    asynUser *mUser;

    GCFeatureMap_t mFeatureMap;
    GCAsynMap_t mAsynMap;

public:
    GenICamFeatureSet (asynPortDriver *portDriver, asynUser *user);

    void insert(GenICamFeature *pFeature, std::string const & name = "");

    asynPortDriver *getPortDriver (void);
    GenICamFeature *getByName (std::string const & name);
    GenICamFeature *getByIndex (int index);
    asynUser *getUser (void);
    int readAll (void);
    int readFeatures (std::vector<std::string> const & params);
    void report (FILE *fp, int details);
    
    // These are used for mapping between areaDetector ImageMode and GenICam AcquisitionMode
    int mAcquisitionModeSingleFrame;
    int mAcquisitionModeMultiFrame;
    int mAcquisitionModeContinuous;

};

#endif
