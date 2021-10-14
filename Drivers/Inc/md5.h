#ifndef _MD5_H_
#define _MD5_H_

/*
 * Simple MD5 implementation
 *
 * Compile with: gcc -o md5 -O3 -lm md5.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
 
// leftrotate function definition
#define LEFTROTATE(x, c) (((x) << (c)) | ((x) >> (32 - (c))))
#define MD5_LEN 16 //16B

uint8_t* md5hash(uint8_t *initial_msg, int initial_len);

#endif