ADGenICam Releases
==================

The latest untagged master branch can be obtained at
https://github.com/areaDetector/ADGenICam.

The versions of EPICS base, asyn, and other synApps modules used for each release can be obtained from 
the EXAMPLE_RELEASE_PATHS.local, EXAMPLE_RELEASE_LIBS.local, and EXAMPLE_RELEASE_PRODS.local
files respectively, in the configure/ directory of the appropriate release of the 
[top-level areaDetector](https://github.com/areaDetector/areaDetector) repository.


Release Notes
=============
R1-3 (XXX-January-2020)
------------------------
* Fix an error when creating the enum choices for GenICam features that are not writable.
  This cause the readback value to be invalid because it was not one of the allowed enum values.
  This generated errors in medm (and probably other OPIs) when opening screens containing these records.
* Added a number of new FLIR/Point Grey cameras.
* Fixed addCamera.sh to specify that Python 3 should be used.

R1-2 (5-January-2020)
------------------------
* Change GenICam integer feature support from 32-bit integers to 64-bit integers, which is what GenICam specifies.  
  This requires asyn R4-38 which adds asynInt64 support for the ai, ao, longin, and longout records.
  With EPICS base 3.16.1 and later (including EPICS 7) int64in and int64 out records can be used for these features.  
  On older versions of base ai and ao records must be used, which limits exact representation of the features to 52 bits.
* Changed makeDb.py to use int64in and int64out records for GenICam integer features if the --devInt64 option flag is used.
  If this option flag is not used then ai and ao records are used for these features.
  The --devInt64 flag is recommended if running on EPICS base 3.16.1 or later, including EPICS 7.
* Changed makeDb.py and makeAdl.py to use Python 3, rather than Python 2.
* Added addCamera.sh which is a simple script that runs both makeDb.py and makeAdl.py.
  It is run from the top-level ADGenICam directory and is passed the name of the camera,
  i.e. the name of the XML file without the path and without the .xml extension.
  It can be edited to enable or disable the --devInt64 flag for makeDb.py.
* Added updateCameras.sh which runs addCamera.sh for each camera in the xml directory. 
  It builds the databases and medm screens for all of the cameras.
* Fixed problems with some EPICS PVs not getting their initial values from the camera correctly.
  This showed up the first time the IOC was run if autosave was being used, or every time the
  IOC was run if autosave was not being used.
* Added a number of new AVT cameras.

R1-1 (20-October-2019)
----------------------
* Changed ADSerialNumber to use either DeviceSerialNumber or DeviceID feature.  
  AVT uses DeviceID for at least some of its cameras.
* Changed makeDb.py to remove the autosaveFields info nodes because they were not useful.
* Rebuilt all the template files using the new version of makeDb.py so they no longer have the autosaveFields tags.
* Added new camera xml, template and OPI files for AVT_GC1380CH and PGR_PGE_23S6C cameras.

R1-0 (12-August-2019)
----
* Initial release

