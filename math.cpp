/*
 * Filename: math.cpp
 * Author: Kevin Hine
 * Description: Math Utility Library
 * Date: Sep 14 2017
 */

#include <stdint.h>

// Standard Math Functions
#define Min(a, b) (((a) < (b)) ? (a) : (b))
#define Max(a, b) (((a) > (b)) ? (a) : (b))

internal inline uint8_t
Lerp(uint8_t a, uint8_t b, double percent) {
  uint8_t result = b + percent * (a - b);
  return result;
}

// Rounds positive
internal inline int32_t
RoundDoubleToInt32(double d) {
  int32_t result = (int32_t)(d + 0.5f);
  return result;
}

internal inline uint32_t
RoundDoubleToUInt32(double d) {
  uint32_t result = (uint32_t)(d + 0.5f);
  return result;
}

internal inline double
Abs(double d) {
  return (d < 0) ? -d : d;
}

// Bit-conversion to a double (used for random numbers)
internal inline double
ToDouble(uint64_t x) {
  double result = (x >> 11) * (1. / ((uint64_t)1 << 53));
  return result;
}

// Random Number Generator
// xoroshiro128+ by David Blackman and Sebastiano Vigna
// http://xoroshiro.di.unimi.it/xoroshiro128plus.c
global_variable uint64_t randomSeed[2];

/*
 * Function Name: rotl
 * Description: Left rotate instruction, usually compiler optimized
 * Parameters: x - value
 *             k - shift amount
 * Side Effects: N/A
 * Error Conditions: N/A
 * Return Value: Result
 */
internal inline uint64_t
rotl(const uint64_t x, int k) {
  return (x << k) | (x >> (64 - k));
}

/*
 * Function Name: Random
 * Description: Generate random 64 bit number
 * Parameters: N/A
 * Side Effects: Updates state bits
 * Error Conditions: N/A
 * Return Value: Result
 */
internal uint64_t
Random() {
  const uint64_t s0 = randomSeed[0];
  uint64_t s1 = randomSeed[1];
  const uint64_t result = s0 + s1;

  s1 ^= s0;
  randomSeed[0] = rotl(s0, 55) ^ s1 ^ (s1 << 14);
  randomSeed[1] = rotl(s1, 36);

  return result;
}

/*
 * Function Name: RandomPercent
 * Description: Generate random double
 * Parameters: N/A
 * Side Effects: Updates state bits
 * Error Conditions: N/A
 * Return Value: Result
 */
internal inline double
RandomPercent() {
  double result = ToDouble(Random());
  return result;
}

/*
 * Function Name: jump
 * Description: Equivalent to 2^64 calls to Random(), able to generate 2^64
 *              non-overlapping subsequences for parallel computations
 * Parameters: N/A
 * Side Effects: Updates state bits
 * Error Conditions: N/A
 * Return Value: N/A
 */
internal void
jump() {
  static const uint64_t JUMP[] = { 0xbeac0467eba5facb, 0xd86b048b86aa9922 };

  uint64_t s0 = 0;
  uint64_t s1 = 0;
  for(size_t i = 0; i < sizeof JUMP / sizeof *JUMP; i++)
    for(int b = 0; b < 64; b++) {
      if (JUMP[i] & (uint64_t)1 << b) {
        s0 ^= randomSeed[0];
        s1 ^= randomSeed[1];
      }
      Random();
    }

  randomSeed[0] = s0;
  randomSeed[1] = s1;
}
