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
  // Z depth must go first
  p->z = RandomPercent();
  p->radius = 2.5f + 2 * p->z;
  p->x = Random() % buffer->width;
  p->y = -2 * p->radius;

  p->velX = 0;
  p->velY = 100;
  p->targetVelX = p->velX;
  p->targetVelY = p->velY;
  p->lerp = 1;
  p->lerpSpeed = 0.01;

  double hue = RandomPercent();
  p->color.a = 0.25 + 0.75 * p->z;
  p->color.r = Lerp(0.3f, 0.5f, hue);
  p->color.g = Lerp(0.9f, 0.5f, hue);
  p->color.b = Lerp(1.0f, 1.0f, hue);
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
  // Perterbations
  if(RandomPercent() > 0.95 && p->lerp > 0.7) {

    // Compound with gravity
    p->startVelX = p->velX;
    p->startVelY = p->velY;

    p->targetVelX = 20 * (0.5 - RandomPercent());
    p->targetVelY = p->velY + 10 * (0.5 - RandomPercent());

    p->lerp = 0;
  }

  // Acceleration
  p->velX = Lerp(p->targetVelX, p->startVelX, p->lerp);
  p->velY = Lerp(p->targetVelY, p->startVelY, p->lerp);
  p->lerp = (p->lerp > 1) ? 1 : p->lerp + p->lerpSpeed;

  // Velocity
  p->x += p->velX * secondsElapsed * (0.5 + 0.5 * p->z);
  p->y += p->velY * secondsElapsed * (0.5 + 0.5 * p->z);
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
    
    // TODO Die early if they leave the screen

    // Particles fade out as they near end
    if(p->lifetime != 0 && p->lifetime < 20) {
      p->color.a *= 0.8;
    }

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
