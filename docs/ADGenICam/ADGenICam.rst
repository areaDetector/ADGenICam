======================================
areaDetector GenICam base class driver
======================================

:author: Mark Rivers, University of Chicago

.. contents:: Contents

Overview
--------

This is an :doc:`../index` base class driver for GenICam cameras.

GenICam is a Generic Interface for Cameras from the European Machine Vision Association (EMVA). 
The stated goal is:

  The goal of GenICamTM (Generic Interface for Cameras) is to provide a generic programming interface for 
  all kinds of devices (mainly cameras), no matter what interface technology (GigE Vision, USB3 Vision, CoaXPress, 
  Camera Link HS, Camera Link etc.) they are using or what features they are implementing. 
  The result is that the application programming interface (API) will be identical regardless 
  of interface technology.

GenICam supports the following interface standards:
  - GigE Vision 
  - USB3 Vision
  - Camera Link
  - Camera Link HS
  - IIDC2 (Firewire)

GenICam cameras are controlled via "features" such as ExposureTime, Gain, TriggerMode, etc.  Each feature has a
name, data type, flags indicating whether it can be read or written, etc.
Many features are defined in the GenICam specification, and vendors must use these names
for those features.  Vendors are also free to add new features that are specific to their cameras.

All GenICam cameras contain an XML file that provides a complete description of all of the features available
in that camera. ADGenICam provides Python scripts that read and parse the XML file and automatically produce
the EPICS database (.template) file and medm display (.adl) files.  These medm files can then be autoconverted
to edm, CSS-Boy, and caQtDM files.

This is a small snippet from an XML file for the feature called PixelFormat::

  <Enumeration Name="PixelFormat" NameSpace="Standard">
    <ToolTip>Format of the pixel data.</ToolTip>
    <Description>Format of the pixel data.</Description>
    <DisplayName>Pixel Format</DisplayName>
    <Visibility> Beginner</Visibility>
    <pIsLocked>TLParamsLocked</pIsLocked>
    <ImposedAccessMode>RW</ImposedAccessMode>
    <EnumEntry Name="Mono8" NameSpace="Standard">
      <ToolTip>Pixel format set to Mono 8.</ToolTip>
      <Description>Pixel format set to Mono 8.</Description>
      <DisplayName>Mono 8</DisplayName>
      <pIsImplemented>Mono8Inq_Reg</pIsImplemented>
      <Value>0x01080001</Value>
    </EnumEntry>


ADGenICam Classes
-----------------
ADGenICam provides three classes that are used by derived classes for real cameras.

GenICamFeature class
====================
This class defines a GenICam feature.  Real drivers must define a class derived from ADGenICam that implements
a number of pure virtual functions, for example to write/read features to/from the camera.  
This is the complete list of all pure virtual functions that the derived class must implement::

    // These are the pure virtual functions that derived classes must implement
    virtual bool isImplemented(void) = 0;
    virtual bool isAvailable(void) = 0;
    virtual bool isReadable(void) = 0;
    virtual bool isWritable(void) = 0;
    virtual int readInteger(void) = 0;
    virtual int readIntegerMin(void) = 0;
    virtual int readIntegerMax(void) = 0;
    virtual int readIncrement(void) = 0;
    virtual void writeInteger(int value) = 0;
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

This class includes the following properties::

    std::string mAsynName;        // asyn parameter name
    asynParamType mAsynType;      // asyn parameter type
    int mAsynIndex;               // asyn parameter index
    std::string mFeatureName;     // GenICam feature name
    GCFeatureType_t mFeatureType; // GenICam feature type


ADGenICamFeatureSet
===================

ADGenICam class
===============
ADGenICam inherits from :doc:`../ADCore/ADDriver`.

MEDM screens
------------

The following is the MEDM screen ADGenICam.adl.

.. figure:: prosilica.png
    :align: center

