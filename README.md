ADGenICam
===========
An 
[EPICS](http://www.aps.anl.gov/epics)
[areaDetector](https://github.com/areaDetector/areaDetector/blob/master/README.md)
driver for
[GenICam](https://www.emva.org/standards-technology/genicam/) cameras.
This is a base class that actual drivers inherit from.  These actual drivers
implement the GenICam standard in vendor-specific SDKs.  

The following drivers currently derive from this base class:
- [ADAravis](https://github.com/areaDetector/ADAravis) which supports any GigE, 10GigE, or USB-3 GenICam camera on Linux.
  It is based on [aravis](https://github.com/AravisProject/aravis).
- [ADSpinnaker](https://github.com/areaDetector/ADSpinnaker) which supports GigE, 10GigE, and USB-3 GenICam cameras
  from FLIR/Point Grey on Linux and Windows.  It uses the [FLIR/Point Grey Spinnaker SDK](https://www.ptgrey.com/spinnaker-sdk)
- [ADVimba](https://github.com/areaDetector/ADVimba) which supports GigE, 10GigE, and USB-3 GenICam cameras
  from AVT/Prosilica on Linux and Windows.  It uses the [Allied Vision's Vimba SDK](https://www.alliedvision.com/en/products/software.html).

Additional information:
* [Documentation](https://areadetector.github.io/master/ADGenICam/ADGenICam.html)
* [Release notes](RELEASE.md)
