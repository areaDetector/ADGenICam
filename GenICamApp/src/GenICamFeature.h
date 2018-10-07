#ifndef GENICAM_FEATURE_H
#define GENICAM_FEATURE_H

#include <string>
#include <vector>
#include <map>
#include <asynPortDriver.h>

typedef enum 
{
    GCFeatureTypeInt,
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
    GCAccess_RO,
    GCAccess_RW,
    GCAccess_WO
} GCAccessMode_t;

typedef struct 
{
    bool exists;
    union
    {
        int valInt;
        double valDouble;
    };
} GCMinMax_t;

class GenICamFeatureSet;

class GenICamFeature
{

private:
    GenICamFeatureSet *mSet;
    std::string mAsynName;
    asynParamType mAsynType;
    std::string mName;
    bool mRemote;

    int mAsynIndex;
    GCFeatureType_t mType;
    GCAccessMode_t mAccessMode;
    GCMinMax_t mMin, mMax;
    std::vector <std::string> mEnumValues;
    double mEpsilon;
    bool mCustomEnum;

    int getEnumIndex (std::string const & value, size_t & index);

    int getParam (int & value);
    int getParam (double & value);
    int getParam (std::string & value);

    int setParam (int value);
    int setParam (double value);
    int setParam (std::string const & value);

    int baseFetch (std::string & rawValue);
    int basePut (std::string const & rawValue);

public:
    GenICamFeature (GenICamFeatureSet *set, std::string const & asynName,
            asynParamType asynType,
            std::string const & name = "");

    int getIndex (void);
    void setEnumValues (std::vector<std::string> const & values);

    // Get the underlying asyn parameter value
    int get (bool & value);
    int get (int & value);
    int get (double & value);
    int get (std::string & value);

    // Fetch the current value from the detector, update underlying asyn parameter
    // and return the value
    int fetch (void);
    int fetch (bool & value);
    int fetch (int & value);
    int fetch (double & value);
    int fetch (std::string & value);

    // Put the value both to the detector (if it is connected to a detector
    // parameter) and to the underlying asyn parameter if successful. Update
    // other modified parameters automatically.
    int put (bool value);
    int put (int value);
    int put (double value);
    int put (std::string const & value);
    int put (const char *value);
};

typedef std::map<std::string, GenICamFeature*> GCFeatureMap_t;
typedef std::map<int, GenICamFeature*> GCAsynMap_t;

class GenICamFeatureSet
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
    int fetchAll (void);

    int fetchParams (std::vector<std::string> const & params);
};



#endif
