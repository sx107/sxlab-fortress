/*
 * rnd.c
 *
 *  Created on: Feb 24, 2024
 *      Author: Sx107
 */

#include <rnd.h>

uint32_t xorshift32(uint32_t x) {
	x ^= x << 13;
	x ^= x >> 17;
	x ^= x << 5;
	return x;
}
