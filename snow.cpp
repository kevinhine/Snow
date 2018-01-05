/*
 * Filename: snow.cpp
 * Author: Kevin Hine
 * Description: Main Logic
 * Date: Sep 06 2017
 */

#include "render.h"
#include "snow.h"
#include "math.cpp"

inline uint8_t *
GetPixel(FrameBuffer *buffer, int x, int y) {
  uint8_t *result = (uint8_t *)buffer->bitmap + x*buffer->pixelBytes + y*buffer->pitch;
  return result;
}

inline Color
GetColor(DoubleColor c) {
  Color result;
  result.a = RoundDoubleToUInt32(c.a * 255.0f);
  result.r = RoundDoubleToUInt32(c.r * 255.0f);
  result.g = RoundDoubleToUInt32(c.g * 255.0f);
  result.b = RoundDoubleToUInt32(c.b * 255.0f);
  return result;
}

inline DoubleColor
GetDoubleColor(uint32_t c) {
  DoubleColor result;
  result.a = (double) ((c >> 24) & 0xff) / 255.0f;
  result.r = (double) ((c >> 16) & 0xff) / 255.0f;
  result.g = (double) ((c >> 8)  & 0xff) / 255.0f;
  result.b = (double) ((c >> 0)  & 0xff) / 255.0f;
  return result;
}

inline internal Color
Composite(Color src, Color dest) {
  double percent = (double) src.a / 255.0f;
  Color result;
  result.a = src.a;
  result.r = Lerp(src.r, dest.r, percent);
  result.g = Lerp(src.g, dest.g, percent);
  result.b = Lerp(src.b, dest.b, percent);
  return result;
}

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
FillRect(FrameBuffer *buffer, double realMinX, double realMinY, double realMaxX, double realMaxY, Color srcColor) {
  // TODO give partial pixels the color but with alpha?
  int32_t minX = RoundDoubleToInt32(realMinX);
  int32_t minY = RoundDoubleToInt32(realMinY);
  int32_t maxX = RoundDoubleToInt32(realMaxX);
  int32_t maxY = RoundDoubleToInt32(realMaxY);

  if(minX < 0) minX = 0;
  if(minY < 0) minY = 0;
  maxX = Min(buffer->width, maxX);
  maxY = Min(buffer->height, maxY);

  uint8_t *row = GetPixel(buffer, minX, minY);
  for(int y = minY; y < maxY; y++) {
    uint32_t *pixel = (uint32_t *)row;
    for(int x = minX; x < maxX; x++) {
      // Overwrite
      if(srcColor.a == 1) {
        *pixel++ = srcColor.argb;
      }
      // Compositing
      else {
        Color destColor = {*pixel};
        Color color = Composite(srcColor, destColor);
        *pixel++ = color.argb;
      }
    }
    row += buffer->pitch;
  }
}

internal void
DrawParticle(FrameBuffer *buffer, Particle *p) {
  Color c = GetColor(p->color);
  FillRect(buffer, p->x - p->radius, p->y - p->radius, p->x + p->radius, p->y + p->radius, c);
}

internal void
InitParticle(FrameBuffer *buffer, Particle *p) {
  p->radius = 2.5f;
  p->x = Random() % buffer->width;
  p->y = -2 * p->radius;
  p->color.a = 0.25 + 0.75*RandomPercent();
  p->color.r = 0.55f;
  p->color.g = 0.9f; 
  p->color.b = 1.0f;
  p->lifetime = 600;
}

internal void
AnimateParticle(Particle *p, double secondsElapsed) {
  p->x += 0;
  p->y += (160 * secondsElapsed);
  p->lifetime--;
}

internal void
UpdateAndRender(Memory *memory, FrameBuffer *buffer, double secondsElapsed) {
  Assert(sizeof(State) < memory->size);
  State *state = (State *)memory->storage;
  if(!memory->isInitialized) {
    randomSeed[0] = 0x0bdb1dd352d7ddd4;
    randomSeed[1] = 0x009b18cd16d1df52;
  
    // Link particle free list (skip the last, it has no next to initialize)
    for(int i = ArrayLength(state->particles) - 2; i >= 0; i--) {
      Particle *p = &state->particles[i];
      p->next = &state->particles[i + 1];
    }
    state->availableParticle = state->particles;

    memory->isInitialized = true;
  }

  // Specifies just the alpha color for the background
  DoubleColor background = {0.05};
  FillRect(buffer, 0, 0, buffer->width, buffer->height, GetColor(background));
  //RenderGradient(buffer, state->ticks);

  // Particle Spawning
  // TODO constant particle density?
  if(state->ticks % 2 == 0) {
    Particle *p = state->availableParticle;
    // TODO: replace oldest?
    Assert(p != 0);
    if(p) {
      state->availableParticle = p->next;
      InitParticle(buffer, p);
    }
  }

  //Simulate and Draw Particles
  for(size_t i = 0; i < ArrayLength(state->particles); i++) {
    Particle *p = &state->particles[i];
    if(p->lifetime == 0) {
      p->next = state->availableParticle;
      state->availableParticle = p;
    }
    else {
      AnimateParticle(p, secondsElapsed);
      DrawParticle(buffer, p);
    }
  }

  state->ticks++;
}
