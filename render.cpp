/*
 * Filename: render.cpp
 * Author: Kevin Hine
 * Userid: khine
 * Description: Rendering Library Code
 * Date: Jan 05 2018
 */

#include "render.h"

/*
 * Function Name: GetPixel
 * Description: Pixel access from framebuffer
 * Parameters: buffer - framebuffer
 *             x - horizontal pos
 *             y - vertical pos
 * Side Effects: N/A
 * Error Conditions: N/A
 * Return Value: Result
 */
inline uint8_t *
GetPixel(FrameBuffer *buffer, int x, int y) {
  uint8_t *result = (uint8_t *)buffer->bitmap + x * buffer->pixelBytes + y * buffer->pitch;
  return result;
}

/*
 * Function Name: GetColor
 * Description: Convert double color to int color
 * Parameters: c - color to convert
 * Side Effects: N/A
 * Error Conditions: N/A
 * Return Value: Result
 */
inline Color
GetColor(DoubleColor c) {
  Color result;
  result.a = RoundDoubleToUInt32(c.a * 255.0f);
  result.r = RoundDoubleToUInt32(c.r * 255.0f);
  result.g = RoundDoubleToUInt32(c.g * 255.0f);
  result.b = RoundDoubleToUInt32(c.b * 255.0f);
  return result;
}

/*
 * Function Name: GetDoubleColor
 * Description: Convert 32-bit argb color to 0-1.0 double color
 * Parameters: c - color to convert
 * Side Effects: N/A
 * Error Conditions: N/A
 * Return Value: Result
 */
inline DoubleColor
GetDoubleColor(uint32_t c) {
  DoubleColor result;
  result.a = (double) ((c >> 24) & 0xff) / 255.0f;
  result.r = (double) ((c >> 16) & 0xff) / 255.0f;
  result.g = (double) ((c >> 8)  & 0xff) / 255.0f;
  result.b = (double) ((c >> 0)  & 0xff) / 255.0f;
  return result;
}

/*
 * Function Name: Composite
 * Description: Overlay src on desitantion by alpha
 * Parameters: src - additive color
 *             dest - initial color
 *             percent - color lerp amount
 * Side Effects: N/A
 * Error Conditions: N/A
 * Return Value: Result
 */
inline internal Color
Composite(Color src, Color dest, double percent) {
  if(percent == 1) {
    return src;
  }
  Color result;
  result.a = src.a;
  result.r = Lerp(src.r, dest.r, percent);
  result.g = Lerp(src.g, dest.g, percent);
  result.b = Lerp(src.b, dest.b, percent);
  return result;
}

/*
 * Function Name: FillRect
 * Description: Draw filled rectangle to framebuffer
 * Parameters: buffer - framebuffer
 *             startX - min x pos
 *             startY - min y pos
 *             endX - max x pos
 *             endY - max y pos
 *             srcColor - rect color
 * Side Effects: Fills framebuffer with tiled gradient
 * Error Conditions: N/A
 * Return Value: N/A
 */
internal void
FillRect(FrameBuffer *buffer, double startX, double startY, double endX, double endY, Color srcColor) {
  int32_t minX = RoundDoubleToInt32(startX);
  int32_t minY = RoundDoubleToInt32(startY);
  int32_t maxX = RoundDoubleToInt32(endX);
  int32_t maxY = RoundDoubleToInt32(endY);

  double minXFill = Abs(startX - minX);
  double minYFill = Abs(startY - minY);
  double maxXFill = Abs(endX - maxX);
  double maxYFill = Abs(endY - maxY);

  // Clamp
  if(minX <= 0) {
    minX = 0;
    minXFill = 1;
  }
  if(minY <= 0) {
    minY = 0;
    minYFill = 1;
  }
  if(maxX >= buffer->width) {
    maxX = buffer->width;
    maxXFill = 1;
  }
  if(maxY >= buffer->height) {
    maxY = buffer->height;
    maxYFill = 1;
  }

  uint8_t *row = GetPixel(buffer, minX, minY);
  for(int y = minY; y < maxY; y++) {

    uint32_t *pixel = (uint32_t *)row;
    for(int x = minX; x < maxX; x++) {

      // Compositing
      Color destColor = {*pixel};
      double alpha = (srcColor.a / 255.0f);

      // Partial pixel alpha multiplier
      double fillRatio = 1;
      if(y == minY) fillRatio *= minYFill;
      if(x == minX) fillRatio *= minXFill;
      if(y == maxY - 1) fillRatio *= maxYFill;
      if(x == maxX - 1) fillRatio *= maxXFill;

      double percent = alpha * fillRatio;
      Color color = Composite(srcColor, destColor, percent);
      *pixel++ = color.argb;
    }
    row += buffer->pitch;
  }
}

/*
 * Function Name: RenderGradient
 * Description: Debugging function for frame timing, color endian-ness
 * Parameters: buffer - framebuffer
 *             var - gradient offset
 * Side Effects: Fills framebuffer with tiled gradient
 * Error Conditions: N/A
 * Return Value: N/A
 */
internal void
RenderGradient(FrameBuffer *buffer, int var) {
  uint8_t *row = (uint8_t *)buffer->bitmap;
  for(int y = 0; y < buffer->height; y++) {
    uint32_t *pixel = (uint32_t *)row;
    for(int x = 0; x < buffer->width; x++) {
      int red = (uint8_t)var;
      int green = (uint8_t)(y + var);
      int blue = (uint8_t)(x + var);
      *pixel++ = (red << 16) | (green << 8) | (blue);
    }
    row += buffer->pitch;
  }
}

internal void
RenderGradient2(FrameBuffer *buffer, int var) {
  uint8_t *row = (uint8_t *)buffer->bitmap;
  for(int y = 0; y < buffer->height; y++) {
    uint32_t *pixel = (uint32_t *)row;
    for(int x = 0; x < buffer->width; x++) {
      int alpha = (uint8_t)var;
      int red = (uint8_t)var;
      int green = (uint8_t)var;
      int blue = (uint8_t)var;
      *pixel++ = (alpha << 24) | (red << 16) | (green << 8) | (blue);
    }
    row += buffer->pitch;
  }
}
