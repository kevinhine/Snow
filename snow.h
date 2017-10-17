/*
 * Filename: snow.h
 * Author: Kevin Hine
 * Description: Macros, Structs, and Function Declarations
 * Date: Sep 06 2017
 */

#ifndef SNOW_H
#define SNOW_H

#define STOP_HAMMER_TIME(x) #x
#define TOSTRING(x) STOP_HAMMER_TIME(x)

/*
  EXTERNAL_BUILD:
    0 - Dev Build Only
    1 - Release

  FAST_BUILD:
    0 - Asserts and other checking allowed
    1 - Optimized
 */

#if FAST_BUILD
#define Assert(expr)
#else
#define Assert(expr) if(!(expr)) { \
                       MessageBox(0, "Error in "__FILE__" on line " TOSTRING(__LINE__) "\nFailed: " TOSTRING(expr), 0, MB_OK); \
                       PostQuitMessage(0);\
                     }
#endif

#define Kilobytes(value) ((uint64_t)(value)*1024)
#define Megabytes(value) (Kilobytes(value)*1024)
#define Gigabytes(value) (Megabytes(value)*1024)
#define Terabytes(value) (Gigabytes(value)*1024)

#define internal static
#define local_persist static
#define global_variable static

#define ArrayLength(arr) (sizeof(arr) / sizeof(*arr))

struct Memory {
  bool isInitialized; // TODO maybe able to use something else as flag
  size_t size;
  void *storage;
};

struct FrameBuffer {
  void *bitmap;
  int width;
  int height;
  int pitch;
  int pixelBytes;
};

//
// Services Provided to the Platform
//
internal void UpdateAndRender(Memory *memory, FrameBuffer *buffer, double secondsElapsed);


//
// Application Structures
//
#include "render.h"

struct State {
  uint64_t ticks;
  Particle *availableParticle;
  // Size >= particle lifetime/spawn rate
  Particle particles[300]; 
};

#endif /* SNOW_H */


