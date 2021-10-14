#ifndef _AES_128_H_
#define _AES_128_H_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define AES128_KEY_LEN 16 //16B
#define AES128_BLOCK_LEN 16 //16B

//generate new key for new connect
void AES_Generate_Rand_Key();

uint8_t* AES_Get_Key();

//overwrite, make sure data memory is enough
int AES_Encrypt_Packet(uint8_t *data, int len);
int AES_Decrypt_Packet(uint8_t *data, int len);

int AES_Decrypt_Packet_Key(uint8_t *data, int len, uint8_t *key);

#endif // _AES_128_H_