/*
 * Filename: snow.cpp
 * Author: Kevin Hine
 * Description: Main Logic
 * Date: Sep 06 2017
 */

#include "render.h"
#include "snow.h"
#include "math.cpp"

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
  uint8_t *result = (uint8_t *)buffer->bitmap + x*buffer->pixelBytes + y*buffer->pitch;
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
      double alpha = (destColor.a / 255.0f);

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
 * Function Name: DrawParticle
 * Description: Render particle
 * Parameters: buffer - framebuffer
 *             p - particle
 * Side Effects: Render particle to framebuffer
 * Error Conditions: N/A
 * Return Value: N/A
 */
internal void
DrawParticle(FrameBuffer *buffer, Particle *p) {
  Color c = GetColor(p->color);
  FillRect(buffer, p->x - p->radius, p->y - p->radius, p->x + p->radius, p->y + p->radius, c);
}

/*
 * Function Name: InitParticle
 * Description: Initialize particle
 * Parameters: buffer - framebuffer
 *             p - particle
 * Side Effects: N/A
 * Error Conditions: N/A
 * Return Value: N/A
 */
internal void
InitParticle(FrameBuffer *buffer, Particle *p) {
  p->radius = 20;//2.5f;
  p->x = Random() % buffer->width;
  p->y = -2 * p->radius;
  p->velX = 50;
  p->velY = 160;
  p->color.a = 0.25 + 0.75*RandomPercent();
  p->color.r = 0.55f;
  p->color.g = 0.9f; 
  p->color.b = 1.0f;
  p->lifetime = 200;
}

/*
 * Function Name: AnimateParticle
 * Description: Update particle state
 * Parameters: p - particle
 *             secondsElapsed - animation time step
 * Side Effects: Render particle to framebuffer
 * Error Conditions: N/A
 * Return Value: N/A
 */
internal void
AnimateParticle(Particle *p, double secondsElapsed) {
  p->x += p->velX * secondsElapsed;
  p->y += p->velY * secondsElapsed;
  p->lifetime--;
}

/*
 * Function Name: UpdateAndRender
 * Description: Manage particle state and display
 * Parameters: memory - system allocated storage
 *             buffer - framebuffer
 *             secondsElapsed - animation time step
 * Side Effects: Updates and Renders particles
 * Error Conditions: N/A
 * Return Value: N/A
 */
internal void
UpdateAndRender(Memory *memory, FrameBuffer *buffer, double secondsElapsed) {
  Assert(sizeof(State) <= memory->size);
  State *state = (State *)memory->storage;
  if(!memory->isInitialized) {
    randomSeed[0] = 0x0bdb1dd352d7ddd4;
    randomSeed[1] = 0x009b18cd16d1df52;
  
    // Link particle free list
    for(int i = ArrayLength(state->particles) - 2; i >= 0; i--) {
      Particle *p = state->particles + i;
      p->next = p + 1;
    }

    state->availableParticle = state->particles;

    memory->isInitialized = true;
  }

  // Specifies color for the background
  DoubleColor background = {1}; //{1, 0.01, 0.02, 0.05};
  FillRect(buffer, 0, 0, buffer->width, buffer->height, GetColor(background));
  RenderGradient(buffer, state->ticks);

  // Particle spawning
  // TODO constant particle density?
  if(state->ticks % 2 == 0) {
    Particle *p = state->availableParticle;

    if(p) {
      state->availableParticle = p->next;
      InitParticle(buffer, p);
    }
    // TODO Currently, particles fail to spawn if none are available. Possibly
    // look into reducing lifetimes of existing particles or cull at higher
    // threshold
  }

  // Simulate and draw particles
  for(size_t i = 0; i < ArrayLength(state->particles); i++) {
    Particle *p = state->particles + i;

    // Add to free list
    if(p->lifetime != 0 && p->lifetime <= 1) {
      p->next = state->availableParticle;
      state->availableParticle = p;
      p->lifetime = 0;
    }
    else {
      AnimateParticle(p, secondsElapsed);
      DrawParticle(buffer, p);
    }
  }

  state->ticks++;
}
