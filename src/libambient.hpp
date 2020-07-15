#ifndef LIB_AMBIENT_H
#define LIB_AMBIENT_H

#ifndef AMBIENT_API
    #define AMBIENT_API __declspec(dllexport)
#endif

typedef unsigned int COLOR;
typedef float HUE;

// Debug stuff
// (use '#define DEBUG' before including this header to enable debug messages)
#ifdef DEBUG
    #define dbgInfo(msg) printf("[INFO] %s\n", msg);
    #define dbgErr(msg) printf("[ERROR] %s\n", msg); 
#else
    #define dbgInfo(msg)
    #define dbgErr(msg)
#endif

extern "C"
{
    AMBIENT_API void    initialize(int screenWidth, int screenHeight, int bitmapWidth, int bitmapHeight);
    AMBIENT_API void    uninitialize();
    AMBIENT_API HUE     getAmbientScreenHue();
}

#endif