#ifndef PLUGINAPI_DATATYPE
#define PLUGINAPI_DATATYPE

typedef void (*EventFunction)(void*);
struct WsHandle__{int unused;}; 
typedef struct WsHandle__ *WsHandle;
typedef int WsBool;
typedef unsigned int ProtectionMode;

typedef const WsHandle LogLevel;
typedef WsHandle WsExtension;

#endif
