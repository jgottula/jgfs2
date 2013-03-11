/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#include "rand.h"
#include <stdlib.h>


uint32_t rand32(void) {
	return (uint32_t)mrand48();
}

/* random in range [0, max]: stackoverflow.com/questions/2999075 */
uint32_t rand32_range(uint32_t max) {
	if (max == UINT32_MAX) {
		return rand32();
	}
	
	uint32_t divisor = UINT32_MAX / (max + 1);
	uint32_t result;
	
	do {
		result = rand32() / divisor;
	} while (result > max);
	
	return result;
}

uint64_t rand64(void) {
	uint64_t top    = (uint32_t)mrand48();
	uint64_t bottom = (uint32_t)mrand48();
	
	return (top << 32) | bottom;
}

/* random in range [0, max]: stackoverflow.com/questions/2999075 */
uint64_t rand64_range(uint64_t max) {
	if (max == UINT64_MAX) {
		return rand64();
	}
	
	uint64_t divisor = UINT64_MAX / (max + 1);
	uint64_t result;
	
	do {
		result = rand64() / divisor;
	} while (result > max);
	
	return result;
}

/* fill array with random values in range [0, max] */
void rand32_fill_range(uint32_t *arr, size_t cnt, uint32_t max) {
	while (cnt-- != 0) {
		*(arr++) = rand32_range(max);
	}
}

/* standard Fisher-Yates shuffle */
void rand32_permute(uint32_t *arr, size_t cnt) {
	for (size_t i = cnt - 1; i >= 1; --i) {
		size_t j = rand32_range(i);
		
		/* swap arr[i] with arr[j] */
		uint32_t temp = arr[i];
		arr[i] = arr[j];
		arr[j] = temp;
	}
}

/* inside-out Fisher-Yates shuffle, initialized to values from [0, cnt) */
void rand32_permute_init(uint32_t *arr, size_t cnt) {
	arr[0] = 0;
	
	for (size_t i = 1; i < cnt; ++i) {
		size_t j = rand32_range(i);
		
		/* swap and initialize */
		arr[i] = arr[j];
		arr[j] = i;
	}
}
