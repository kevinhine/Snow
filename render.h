/*
 * Filename: render.h
 * Author: Kevin Hine
 * Description: 
 * Date: Sep 14 2017
 */

#ifndef RENDER_H
#define RENDER_H 

struct DoubleColor {
  double a;
  double r;
  double g;
  double b;
};

struct Color {
  union {
    uint32_t argb;
    // Little Endian
    struct {
      uint8_t b;
      uint8_t g;
      uint8_t r;
      uint8_t a;
    };
  };
};

struct Mask {
  int width;
  int height;
  double *pixel;
};

// When lifetime is 0, the particle is not in use, so other data fields can be 
// used as a linked list of free particles
struct Particle {
  uint32_t lifetime; //Particle lifetime in frames
  union {
    Particle *next;
    struct {
      double x;
      double y;
      double z;
      double velX;
      double velY;
      double startVelX;
      double startVelY;
      double targetVelX;
      double targetVelY;
      double lerp;
      double lerpSpeed;
      double radius;
      DoubleColor color;
    };
  };
};

#endif /* RENDER_H */
