ADGenICam
===========
An 
[EPICS](http://www.aps.anl.gov/epics)
[areaDetector](http://cars.uchicago.edu/software/epics/areaDetector.html)
driver for  
[GenICam](https://www.emva.org/standards-technology/genicam/) cameras.
This is a base class that actual drivers inherit from.  These actual drivers
implement the GenICam standard in vendor-specific SDKs.  The plan is to initially support 
[aravis](https://github.com/AravisProject/aravis), 
[FLIR/Point Grey Spinnaker](https://www.ptgrey.com/spinnaker-sdk), and 
[Allied Vision's Vimba](https://www.alliedvision.com/en/products/software.html).

Additional information:
* [Documentation](http://cars.uchicago.edu/software/epics/ADGenICamDoc.html).
* [Release notes and links to source and binary releases](RELEASE.md).
