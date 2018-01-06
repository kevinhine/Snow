/*
 * Filename: snow.cpp
 * Author: Kevin Hine
 * Description: Main Logic
 * Date: Sep 06 2017
 */

#include "snow.h"
#include "math.cpp"
#include "render.cpp"

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
  p->radius = 2.5f;
  p->x = Random() % buffer->width;
  p->y = -2 * p->radius;
  p->velX = 0;
  p->velY = 160;
  p->color.a = 0.25 + 0.75*RandomPercent();
  p->color.r = 0.55f;
  p->color.g = 0.9f; 
  p->color.b = 1.0f;
  p->lifetime = 600;
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
  DoubleColor background = {1, 0.01, 0.02, 0.05};
  FillRect(buffer, 0, 0, buffer->width, buffer->height, GetColor(background));

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
