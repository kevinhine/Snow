/*
 * Filename: win32_snow.cpp
 * Author: Kevin Hine
 * Description: Windows System Layer
 *              Based upon work by Casey Muratori in Handmade Hero
 *              https://handmadehero.org/
 * Date: Aug 28 2017
 */

#include <windows.h>
#include <stdint.h>
#include "snow.h"
#include "snow.cpp"

struct win32FrameBuffer {
  BITMAPINFO info;
  void *bitmap;
  int width;
  int height;
  int pitch;
  int pixelBytes;
};

struct win32Dimension {
  int width;
  int height;
};

global_variable bool globalRunning;
global_variable win32FrameBuffer globalBuffer;

global_variable int64_t globalPerfCountFrequency;

/*
 * Function Name: Win32GetWindowDimension
 * Description: Bundle window dimension
 * Parameters: window - window handle
 * Side Effects: N/A
 * Error Conditions: N/A
 * Return Value: window dimension
 */
internal win32Dimension
Win32GetWindowDimension(HWND window) {
  win32Dimension result;

  RECT clientRect;
  GetClientRect(window, &clientRect);

  result.width = clientRect.right - clientRect.left;
  result.height = clientRect.bottom - clientRect.top;

  return result;
}

/*
 * Function Name: Win32ResizeDIBSection
 * Description: Scale framebuffer to window dimension
 * Parameters: buffer - framebuffer
 *             width - bitmap width
 *             height - bitmap height
 * Side Effects: Reallocate the bitmap
 * Error Conditions: N/A
 * Return Value: N/A
 */
internal void
Win32ResizeDIBSection(win32FrameBuffer *buffer, int width, int height) {
  if(buffer->bitmap) {
    VirtualFree(buffer, 0, MEM_RELEASE);
  }

  buffer->width = width;
  buffer->height = height;
  buffer->pitch = buffer->width*buffer->pixelBytes;
  buffer->pixelBytes = 4;

  // Negative biHeight specifies that this is a top-down image
  BITMAPINFOHEADER *bmiHeader = &buffer->info.bmiHeader;
  bmiHeader->biSize = sizeof(buffer->info.bmiHeader);
  bmiHeader->biWidth = buffer->width;
  bmiHeader->biHeight = -buffer->height;
  bmiHeader->biPlanes = 1;
  bmiHeader->biBitCount = 32;
  bmiHeader->biCompression = BI_RGB;

  // VirtualAlloc clears to zero, so bitmap is automatically cleared to black
  int bitmapSize = (buffer->width * buffer->height) * buffer->pixelBytes;
  buffer->bitmap = VirtualAlloc(0, bitmapSize, MEM_COMMIT, PAGE_READWRITE);
}

/*
 * Function Name: Win32DisplayBuffer
 * Description: Blit buffer to screen
 * Parameters: deviceContext - screen handle
 *             windowWidth - width
 *             windowHeight - height
 *             buffer - frameBuffer
 * Side Effects: Renders the frame buffer
 * Error Conditions: N/A
 * Return Value: N/A
 */
internal void
Win32DisplayBuffer(HDC deviceContext, int windowWidth, int windowHeight, win32FrameBuffer *buffer) {
  StretchDIBits(deviceContext,
                0, 0, windowWidth, windowHeight, // Dest
                0, 0, buffer->width, buffer->height, // Src
                buffer->bitmap,
                &buffer->info,
                DIB_RGB_COLORS, SRCCOPY);
}

/*
 * Function Name: Win32WindowCallback
 * Description: Process window messages
 * Parameters: window - window handle
 *             message - system message
 *             wParam - additional message data
 *             lParam - additional message data
 * Side Effects: Interrupt program
 * Error Conditions: N/A
 * Return Value: Message result
 */
LRESULT CALLBACK
Win32WindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
  LRESULT result = 0;

  switch(message) {
    // Create new drawing buffer
    case WM_SIZE: {
      win32Dimension dimension = Win32GetWindowDimension(window);
      Win32ResizeDIBSection(&globalBuffer, dimension.width, dimension.height);
    } break;

    case WM_DESTROY: {
      globalRunning = false;
    } break;

    case WM_CLOSE: {
    globalRunning = false;
    } break; 

    case WM_ACTIVATEAPP: {
    } break;

    // Paints the bitmap to the screen
    case WM_PAINT: {
      PAINTSTRUCT paint;
      HDC deviceContext = BeginPaint(window, &paint);
      win32Dimension dimension = Win32GetWindowDimension(window);
      Win32DisplayBuffer(deviceContext, dimension.width, dimension.height, &globalBuffer);
      EndPaint(window, &paint);
    } break;

    default: {
      result = DefWindowProc(window, message, wParam, lParam);
    } break;
  }
  return result;
}

/*
 * Function Name: Win32GetWallClock
 * Description: Clock system time 
 * Parameters: N/A
 * Side Effects: N/A
 * Error Conditions: N/A 
 * Return Value: High resolution time stamp 
 */
inline LARGE_INTEGER
Win32GetWallClock() {
  LARGE_INTEGER result;
  QueryPerformanceCounter(&result);
  return result;
}

/*
 * Function Name: Win32GetSecondsElapsed
 * Description: Convert sample count to wall clock duration
 * Parameters: start - sample start
 *             end - sample end
 * Side Effects: N/A
 * Error Conditions: N/A 
 * Return Value: Seconds elapsed
 */
inline double
Win32GetSecondsElapsed(LARGE_INTEGER start, LARGE_INTEGER end) {
  double result = (double)(end.QuadPart - start.QuadPart) / (double)globalPerfCountFrequency;
  return result;  
}

/*
 * Function Name: WinMain
 * Description: Program Entry, initializes window and main loop
 * Parameters: instance - program handle
 *             prevInstance - always 0
 *             commandLine - command line args
 *             showCode - launch properties
 * Side Effects: Program execution
 * Error Conditions: N/A
 * Return Value: Exit code
 */
int CALLBACK
WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR commandLine, int showCode) {
  // Timing init
  LARGE_INTEGER performanceFrequency;
  QueryPerformanceFrequency(&performanceFrequency);
  globalPerfCountFrequency = performanceFrequency.QuadPart;
  
  // Set windows scheduler grandularity to 1ms
  UINT schedulerMS = 1;
  bool sleepIsGranular = (timeBeginPeriod(schedulerMS) == TIMERR_NOERROR);

  // Close window on crash
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);

  // Window init
  WNDCLASS windowClass = {};
  windowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
  windowClass.lpfnWndProc = Win32WindowCallback;
  windowClass.hInstance = instance;
  //windowClas.hIcon;
  windowClass.lpszClassName = "SnowWC";

  ATOM classAtom = RegisterClass(&windowClass);
  if(!classAtom) {
    return 0;
  }

  HWND window = CreateWindowEx(
    0,
    windowClass.lpszClassName,
    "Snow",
    WS_OVERLAPPEDWINDOW|WS_VISIBLE,
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
    0, 0, instance, 0);
  if(!window) {
    return 0;
  }

  int monitorHZ = 60;
  double targetFrameSeconds = 1.0f / (double)monitorHZ;

  win32Dimension dimension = Win32GetWindowDimension(window);
  Win32ResizeDIBSection(&globalBuffer, dimension.width, dimension.height);

  Memory memory = {};
  memory.size = sizeof(State); //Megabytes(1);
  memory.storage = VirtualAlloc(0, memory.size, MEM_COMMIT, PAGE_READWRITE);
  Assert(memory.size > 0);
 
  // Main loop
  LARGE_INTEGER lastCounter = Win32GetWallClock();
  // Use predicted value for first loop, and prior value for every other
  double frameSecondsElapsed = targetFrameSeconds;
  globalRunning = true;
  while(globalRunning) {

    // Message loop
    MSG message;
    while(PeekMessage(&message, 0, 0, 0, PM_REMOVE)) {
      if(message.message == WM_QUIT) {
        globalRunning = false;
      }
      TranslateMessage(&message);
      DispatchMessage(&message);
    }

    FrameBuffer buffer = {};
    buffer.bitmap = globalBuffer.bitmap;
    buffer.width = globalBuffer.width;
    buffer.height = globalBuffer.height;
    buffer.pitch = globalBuffer.pitch;
    buffer.pixelBytes = globalBuffer.pixelBytes;

    // Uses the total frame time for the previous frame,
    // which is only accurate with a consistent frame-rate
    UpdateAndRender(&memory, &buffer, frameSecondsElapsed);
    
    // Enforced framerate
    LARGE_INTEGER workCounter = Win32GetWallClock();
    double workSecondsElapsed = Win32GetSecondsElapsed(lastCounter, workCounter);

    frameSecondsElapsed = workSecondsElapsed;
    while(frameSecondsElapsed < targetFrameSeconds) {
      if(sleepIsGranular) {
        DWORD sleepMS = (DWORD)(1000.0f * (targetFrameSeconds - frameSecondsElapsed));
        Sleep(sleepMS);
      }
      frameSecondsElapsed = Win32GetSecondsElapsed(lastCounter, Win32GetWallClock());
    }

    LARGE_INTEGER endCounter = Win32GetWallClock();
    lastCounter = endCounter;

    HDC deviceContext = GetDC(window);
    win32Dimension dimension = Win32GetWindowDimension(window);
    Win32DisplayBuffer(deviceContext, dimension.width, dimension.height, &globalBuffer);
    ReleaseDC(window, deviceContext);
  }
  return 0;
}
