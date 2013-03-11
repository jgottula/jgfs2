/* jgfs2
 * (c) 2013 Justin Gottula
 * The source code of this project is distributed under the terms of the
 * simplified BSD license. See the LICENSE file for details.
 */


#ifndef JGFS2_SRC_TEST_RAND_H
#define JGFS2_SRC_TEST_RAND_H


uint32_t rand32(void);
uint32_t rand32_range(uint32_t max);

uint64_t rand64(void);
uint64_t rand64_range(uint64_t max);

void rand32_fill_range(uint32_t *arr, size_t cnt, uint32_t max);

void rand32_permute(uint32_t *arr, size_t cnt);
void rand32_permute_init(uint32_t *arr, size_t cnt);


#endif
