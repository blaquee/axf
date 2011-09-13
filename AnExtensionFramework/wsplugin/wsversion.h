#ifndef wsversion_h__
#define wsversion_h__

/*
    How the packet editor version works

    The overall version (PACKET_EDITOR_VERSION) is formed by joining 3 segments up (each part is described below).

    Each segment ranges from 0 to 255.
    
    if one of the segment increases, all the segments below it must be reset to 0
    for example an increase to minor version done like this: 1.0.50 becomes 1.1.0

    major = Will only be increased if there are major changes to the system that will 
            render all existing plugins completely unusable (a plugin rewrite is unavoidable).
            At the moment, increasing the major version is reserved for a complete codebase rewrite (which I hope will never happen).

    minor = Small breaking changes to the system or API that may or may not require the plugins to be updated.

    service = Bug fixes or enhancements to the system, no API changes and will not require plugin update.

*/

#define AXF_MAJOR_VERSION 1
#define AXF_MINOR_VERSION 0
#define AXF_SERVICE_VERSION 0

#define AXF_VERSION ( ((AXF_MAJOR_VERSION) << (16)) | \
    ((AXF_MINOR_VERSION) << (8))  |  \
    (AXF_SERVICE_VERSION) )

#endif // wsversion_h__

