#include <AES_128.h>

uint8_t AES128key[AES128_KEY_LEN];

//generate new key for new connect
void AES_Generate_Rand_Key() {
	int i;
	srand(time(NULL));
	for(i = 0; i < AES128_KEY_LEN; i++) {
		AES128key[i] = (uint8_t)rand();
	}
}

uint8_t* AES_Get_Key() {return AES128key;}

// rconBox
uint8_t rcon[10] =
{
	0x01, 0x02, 0x04, 0x08,
	0x10, 0x20, 0x40, 0x80,
	0x1b, 0x36
};

//ShiftRows Box
uint8_t SR_box[16] =
{
	0, 5, 10,  15,
	4, 9, 14,  3,
	8, 13, 2, 7,
	12, 1, 6, 11
};
//InvShiftRows Box
uint8_t InvSR_box[16] =
{
	0, 13, 10,  7,
	4, 1, 14,  11,
	8, 5, 2, 15,
	12,9, 6, 3
};

//MixBox
uint8_t Mix[16] =
{
	2, 3, 1, 1,
	1, 2, 3, 1,
	1, 1, 2, 3,
	3, 1, 1, 2

};
//InvMixBox
uint8_t InvMix[16] =
{
	0x0E, 0x0B, 0x0D, 0x09,
	0x09, 0x0E, 0x0B, 0x0D,
	0x0D, 0x09, 0x0E, 0x0B,
	0x0B, 0x0D, 0x09, 0x0E

};

//SBox
uint8_t s_box[256] =
{
 0x63 ,0x7c ,0x77 ,0x7b ,0xf2 ,0x6b ,0x6f ,0xc5 ,0x30 ,0x01 ,0x67 ,0x2b ,0xfe ,0xd7 ,0xab ,0x76
,0xca ,0x82 ,0xc9 ,0x7d ,0xfa ,0x59 ,0x47 ,0xf0 ,0xad ,0xd4 ,0xa2 ,0xaf ,0x9c ,0xa4 ,0x72 ,0xc0
,0xb7 ,0xfd ,0x93 ,0x26 ,0x36 ,0x3f ,0xf7 ,0xcc ,0x34 ,0xa5 ,0xe5 ,0xf1 ,0x71 ,0xd8 ,0x31 ,0x15
,0x04 ,0xc7 ,0x23 ,0xc3 ,0x18 ,0x96 ,0x05 ,0x9a ,0x07 ,0x12 ,0x80 ,0xe2 ,0xeb ,0x27 ,0xb2 ,0x75
,0x09 ,0x83 ,0x2c ,0x1a ,0x1b ,0x6e ,0x5a ,0xa0 ,0x52 ,0x3b ,0xd6 ,0xb3 ,0x29 ,0xe3 ,0x2f ,0x84
,0x53 ,0xd1 ,0x00 ,0xed ,0x20 ,0xfc ,0xb1 ,0x5b ,0x6a ,0xcb ,0xbe ,0x39 ,0x4a ,0x4c ,0x58 ,0xcf
,0xd0 ,0xef ,0xaa ,0xfb ,0x43 ,0x4d ,0x33 ,0x85 ,0x45 ,0xf9 ,0x02 ,0x7f ,0x50 ,0x3c ,0x9f ,0xa8
,0x51 ,0xa3 ,0x40 ,0x8f ,0x92 ,0x9d ,0x38 ,0xf5 ,0xbc ,0xb6 ,0xda ,0x21 ,0x10 ,0xff ,0xf3 ,0xd2
,0xcd ,0x0c ,0x13 ,0xec ,0x5f ,0x97 ,0x44 ,0x17 ,0xc4 ,0xa7 ,0x7e ,0x3d ,0x64 ,0x5d ,0x19 ,0x73
,0x60 ,0x81 ,0x4f ,0xdc ,0x22 ,0x2a ,0x90 ,0x88 ,0x46 ,0xee ,0xb8 ,0x14 ,0xde ,0x5e ,0x0b ,0xdb
,0xe0 ,0x32 ,0x3a ,0x0a ,0x49 ,0x06 ,0x24 ,0x5c ,0xc2 ,0xd3 ,0xac ,0x62 ,0x91 ,0x95 ,0xe4 ,0x79
,0xe7 ,0xc8 ,0x37 ,0x6d ,0x8d ,0xd5 ,0x4e ,0xa9 ,0x6c ,0x56 ,0xf4 ,0xea ,0x65 ,0x7a ,0xae ,0x08
,0xba ,0x78 ,0x25 ,0x2e ,0x1c ,0xa6 ,0xb4 ,0xc6 ,0xe8 ,0xdd ,0x74 ,0x1f ,0x4b ,0xbd ,0x8b ,0x8a
,0x70 ,0x3e ,0xb5 ,0x66 ,0x48 ,0x03 ,0xf6 ,0x0e ,0x61 ,0x35 ,0x57 ,0xb9 ,0x86 ,0xc1 ,0x1d ,0x9e
,0xe1 ,0xf8 ,0x98 ,0x11 ,0x69 ,0xd9 ,0x8e ,0x94 ,0x9b ,0x1e ,0x87 ,0xe9 ,0xce ,0x55 ,0x28 ,0xdf
,0x8c ,0xa1 ,0x89 ,0x0d ,0xbf ,0xe6 ,0x42 ,0x68 ,0x41 ,0x99 ,0x2d ,0x0f ,0xb0 ,0x54 ,0xbb ,0x16
};
//InvSBox
unsigned char Invs_box[256] =
{
   0x52, 0x09, 0x6A, 0xD5, 0x30, 0x36, 0xA5, 0x38, 0xBF, 0x40, 0xA3, 0x9E, 0x81, 0xF3, 0xD7, 0xFB,
   0x7C, 0xE3, 0x39, 0x82, 0x9B, 0x2F, 0xFF, 0x87, 0x34, 0x8E, 0x43, 0x44, 0xC4, 0xDE, 0xE9, 0xCB,
   0x54, 0x7B, 0x94, 0x32, 0xA6, 0xC2, 0x23, 0x3D, 0xEE, 0x4C, 0x95, 0x0B, 0x42, 0xFA, 0xC3, 0x4E,
   0x08, 0x2E, 0xA1, 0x66, 0x28, 0xD9, 0x24, 0xB2, 0x76, 0x5B, 0xA2, 0x49, 0x6D, 0x8B, 0xD1, 0x25,
   0x72, 0xF8, 0xF6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xD4, 0xA4, 0x5C, 0xCC, 0x5D, 0x65, 0xB6, 0x92,
   0x6C, 0x70, 0x48, 0x50, 0xFD, 0xED, 0xB9, 0xDA, 0x5E, 0x15, 0x46, 0x57, 0xA7, 0x8D, 0x9D, 0x84,
   0x90, 0xD8, 0xAB, 0x00, 0x8C, 0xBC, 0xD3, 0x0A, 0xF7, 0xE4, 0x58, 0x05, 0xB8, 0xB3, 0x45, 0x06,
   0xD0, 0x2C, 0x1E, 0x8F, 0xCA, 0x3F, 0x0F, 0x02, 0xC1, 0xAF, 0xBD, 0x03, 0x01, 0x13, 0x8A, 0x6B,
   0x3A, 0x91, 0x11, 0x41, 0x4F, 0x67, 0xDC, 0xEA, 0x97, 0xF2, 0xCF, 0xCE, 0xF0, 0xB4, 0xE6, 0x73,
   0x96, 0xAC, 0x74, 0x22, 0xE7, 0xAD, 0x35, 0x85, 0xE2, 0xF9, 0x37, 0xE8, 0x1C, 0x75, 0xDF, 0x6E,
   0x47, 0xF1, 0x1A, 0x71, 0x1D, 0x29, 0xC5, 0x89, 0x6F, 0xB7, 0x62, 0x0E, 0xAA, 0x18, 0xBE, 0x1B,
   0xFC, 0x56, 0x3E, 0x4B, 0xC6, 0xD2, 0x79, 0x20, 0x9A, 0xDB, 0xC0, 0xFE, 0x78, 0xCD, 0x5A, 0xF4,
   0x1F, 0xDD, 0xA8, 0x33, 0x88, 0x07, 0xC7, 0x31, 0xB1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xEC, 0x5F,
   0x60, 0x51, 0x7F, 0xA9, 0x19, 0xB5, 0x4A, 0x0D, 0x2D, 0xE5, 0x7A, 0x9F, 0x93, 0xC9, 0x9C, 0xEF,
   0xA0, 0xE0, 0x3B, 0x4D, 0xAE, 0x2A, 0xF5, 0xB0, 0xC8, 0xEB, 0xBB, 0x3C, 0x83, 0x53, 0x99, 0x61,
   0x17, 0x2B, 0x04, 0x7E, 0xBA, 0x77, 0xD6, 0x26, 0xE1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0C, 0x7D
};

void KeyExpansionCore(uint8_t* in, uint8_t i)
{
	// Rotate left
	uint32_t* q = (uint32_t*)in; //uint8_t* q =in;
	*q = (*q >> 8) | ((*q & 0xff) << 24);
	// s-box four bytes
	in[0] = s_box[in[0]];	 in[1] = s_box[in[1]];
	in[2] = s_box[in[2]];	 in[3] = s_box[in[3]];
	// Rcon
	in[0] ^= rcon[i];
}

void KeyExpansion(uint8_t* inputKey, uint8_t* expandedKeys)
{
	// The first 16 bytes are the original key:
	for (int i = 0; i < 16; i++)
		expandedKeys[i] = inputKey[i];
	//Variables:
	int bytesGenerated = 16; // We've generated 16 bytes so far
	int rconIteration = 0;  // Rcon Iteration begins at 1
	uint8_t temp[4]; // Temporary storage for core

	while (bytesGenerated < 176)
	{
		// Read 4 bytes for the core
		for (int i = 0; i < 4; i++)
			temp[i] = expandedKeys[i + bytesGenerated - 4];
		//Perform the core once for each 16 byte key:
		if (bytesGenerated % 16 == 0)
		{
			KeyExpansionCore(temp, rconIteration);
			rconIteration++;
		}
		//XOR temp with [bytesGenerates-16], and store in expandedKeys:
		for (uint8_t a = 0; a < 4; a++)
		{
			expandedKeys[bytesGenerated] =
				expandedKeys[bytesGenerated - 16] ^ temp[a];
			bytesGenerated++;
		}
	}
}

void SubBytes(uint8_t* state)
{
	for (int i = 0; i < 16; i++)
		state[i] = s_box[state[i]];
}
void InvSubBytes(uint8_t* state)
{
	for (int i = 0; i < 16; i++)
		state[i] = Invs_box[state[i]];
}

void ShiftRows(uint8_t* state)
{
	uint8_t temp[16];
	for (int i = 0; i < 16; i++)
		temp[i] = state[SR_box[i]];
	for (int i = 0; i < 16; i++)
		state[i] = temp[i];
}
void InvShiftRows(uint8_t* state)
{
	uint8_t temp[16];
	for (int i = 0; i < 16; i++)
		temp[i] = state[InvSR_box[i]];
	for (int i = 0; i < 16; i++)
		state[i] = temp[i];
}

uint8_t mulGF(uint8_t i, uint8_t Mix) // multiple in GF 2^8
{
	uint16_t a = i;
	if (Mix == 1)  return a;						  //mul 1  0001
	if (Mix == 3)  a = (i << 1) ^ i;				  //mul 3  0011
	if (Mix == 2)  a = (i << 1);					  //mul 2  0010
	if (Mix == 9)  a = (i << 3) ^ i;				  //mul 9  1001
	if (Mix == 11) a = (i << 3) ^ (i << 1) ^ i;		  //mul 11 1011
	if (Mix == 13) a = (i << 3) ^ (i << 2) ^ i;		  //mul 13 1101
	if (Mix == 14) a = (i << 3) ^ (i << 2) ^ (i << 1);//mul 14 1110
	int k = 0;
	while (a > 0xff)
	{
		k = a / 0x100; // 1 0000  0000
		if (k >= 4) k = 4;
		if (k >= 2 && k < 4) k = 2;
		a ^= (k * 0x11B); // 1 0001 1011: x^8 + x^4 + x^3 + x + 1
	}
	return a;
}
void MixColumns(uint8_t* state)
{
	uint8_t temp[16];
	uint8_t n = 0, k = 0;
	for (int i = 0; i < 16; i++)
	{
		if (i == 4) { n = 4;  k = 0; }
		if (i == 8) { n = 8;  k = 0; }
		if (i == 12) { n = 12; k = 0; }
		uint8_t tempXor[4];
		for (int j = 0; j < 4; j++)
		{
			tempXor[j] = mulGF(state[j + n], Mix[j + k * 4]);
		}
		temp[i] = tempXor[0] ^ tempXor[1] ^ tempXor[2] ^ tempXor[3];
		k++;
	}
	for (int i = 0; i < 16; i++)
		state[i] = temp[i];
}
void InvMixColumns(uint8_t* state)
{
	uint8_t temp[16];
	uint8_t n = 0, k = 0;
	for (int i = 0; i < 16; i++)
	{
		if (i == 4) { n = 4;  k = 0; }
		if (i == 8) { n = 8;  k = 0; }
		if (i == 12) { n = 12; k = 0; }
		uint8_t tempXor[4];
		for (int j = 0; j < 4; j++)
		{
			tempXor[j] = mulGF(state[j + n], InvMix[j + k * 4]);
		}
		temp[i] = tempXor[0] ^ tempXor[1] ^ tempXor[2] ^ tempXor[3];
		k++;
	}
	for (int i = 0; i < 16; i++)
		state[i] = temp[i];
}
void AddRoundKey(uint8_t* state, uint8_t* roundKey)
{
	for (int i = 0; i < 16; i++)
		state[i] ^= roundKey[i];
}

//a block contain 16B
void AES_Encrypt_Block(uint8_t* message, uint8_t* key, uint8_t* cipher)
{
	uint8_t state[16];
	for (int i = 0; i < 16; i++)
		state[i] = message[i];

	//Expand the key:
	uint8_t expandedKey[176];
	KeyExpansion(key, expandedKey);

	AddRoundKey(state, key);

	for (int i = 0; i < 9; i++)
	{
		SubBytes(state);
		ShiftRows(state);
		MixColumns(state);
		AddRoundKey(state, expandedKey + (16 * (i + 1)));
	}
	//Final Round
	SubBytes(state);
	ShiftRows(state);
	AddRoundKey(state, expandedKey + 160);
	//Copy over the message with the encrypt message!
	for (int i = 0; i < 16; i++)
		cipher[i] = state[i];
}
void AES_Encrypt_Block_Overwrite(uint8_t* message, uint8_t* key)
{
	//Expand the key:
	uint8_t expandedKey[176];
	KeyExpansion(key, expandedKey);

	AddRoundKey(message, key);

	for (int i = 0; i < 9; i++)
	{
		SubBytes(message);
		ShiftRows(message);
		MixColumns(message);
		AddRoundKey(message, expandedKey + (16 * (i + 1)));
	}
	//Final Round
	SubBytes(message);
	ShiftRows(message);
	AddRoundKey(message, expandedKey + 160);

	//trick exchange first and last two bytes
	uint8_t tmp = message[0];
	message[0] = message[1];
	message[1] = tmp;
	tmp = message[14];
	message[14] = message[15];
	message[15] = tmp;
}

void AES_Decrypt_Block(uint8_t* cipher, uint8_t* key, uint8_t* decrypted_cipher)
{
	uint8_t state[16];
	for (int i = 0; i < 16; i++)
		state[i] = cipher[i];

	//Expand the key:
	uint8_t expandedKey[176];
	KeyExpansion(key, expandedKey);

	AddRoundKey(state, expandedKey + 160);

	for (int i = 8; i >= 0; i--)
	{
		InvShiftRows(state);
		InvSubBytes(state);
		AddRoundKey(state, expandedKey + (16 * (i + 1)));
		InvMixColumns(state);
	}
	//Final Round
	InvShiftRows(state);
	InvSubBytes(state);
	AddRoundKey(state, expandedKey);
	//Copy over the message with the encrypt message!
	for (int i = 0; i < 16; i++)
		decrypted_cipher[i] = state[i];
}

void AES_Decrypt_Block_Overwrite(uint8_t* cipher, uint8_t* key)
{
	//trick exchange first and last two bytes
	uint8_t tmp = cipher[0];
	cipher[0] = cipher[1];
	cipher[1] = tmp;
	tmp = cipher[14];
	cipher[14] = cipher[15];
	cipher[15] = tmp;

	//Expand the key
	uint8_t expandedKey[176];
	KeyExpansion(key, expandedKey);

	AddRoundKey(cipher, expandedKey + 160);

	for (int i = 8; i >= 0; i--)
	{
		InvShiftRows(cipher);
		InvSubBytes(cipher);
		AddRoundKey(cipher, expandedKey + (16 * (i + 1)));
		InvMixColumns(cipher);
	}
	//Final Round
	InvShiftRows(cipher);
	InvSubBytes(cipher);
	AddRoundKey(cipher, expandedKey);
}

//overwrite, make sure data memory is enough. return ecrypted packet len
int AES_Encrypt_Packet(uint8_t *data, int len) {
	int remainder = len % 16;
	int newLen, i;
	if(remainder != 0) {
		newLen = len + 16 - remainder;
		//generate random
		srand(time(NULL));
		for(i = len; i < newLen; i++) {
			data[i] = (uint8_t)rand();
		}
	} else {
		newLen = len;
	}

	int blockNum = newLen / 16;
	for(i = 0; i < blockNum; i++) {
		AES_Encrypt_Block_Overwrite(data + i*16, AES128key);
	}
	return newLen;
}

//overwrite, own key, make sure data memory is enough. return ecrypted packet len
int AES_Encrypt_Packet_Key(uint8_t *data, int len, uint8_t *key) {
	int remainder = len % 16;
	int newLen, i;
	if(remainder != 0) {
		newLen = len + 16 - remainder;
		//generate random
		srand(time(NULL));
		for(i = len; i < newLen; i++) {
			data[i] = (uint8_t)rand();
		}
	} else {
		newLen = len;
	}

	int blockNum = newLen / 16;
	for(i = 0; i < blockNum; i++) {
		AES_Encrypt_Block_Overwrite(data + i*16, key);
	}
	return newLen;
}

//overwrite, make sure data memory size is multiple of a block size. return decrypted packet len, fail return -1
int AES_Decrypt_Packet(uint8_t *data, int len) {
	if(len % 16 != 0) return - 1;
	int i;
	int blockNum = len / 16;
	for(i = 0; i < blockNum; i++) {
		AES_Decrypt_Block_Overwrite(data + i*16, AES128key);
	}
	return len;
}

//overwrite, own key, make sure data memory size is multiple of a block size. return decrypted packet len, fail return -1
int AES_Decrypt_Packet_Key(uint8_t *data, int len, uint8_t *key) {
	if(len % 16 != 0) return - 1;
	int i;
	int blockNum = len / 16;
	for(i = 0; i < blockNum; i++) {
		AES_Decrypt_Block_Overwrite(data + i*16, key);
	}
	return len;
}

// int main()
// {
// 	uint8_t message[] = "1234567812345678";//e we will encrypt with AES!";
// 	uint8_t key[] =
// 	{
// 		 0x54, 0x68, 0x61, 0x74,
// 		 0x73, 0x20, 0x6D, 0x79,
// 		 0x20, 0x4B, 0x75, 0x6E,
// 		 0x67, 0x20, 0x46, 0x75
// 	}; 

// 	uint8_t encrypted[16];
// 	uint8_t decrypted[16];
// 	AES_Encrypt(message, key, encrypted);
// 	AES_Decrypt(encrypted, key, decrypted);
// 	for (int i=0; i<16; i++)
// 		// cout << "0x" << hex << (int)encrypted[i] << ", ";
// 		printf("%#x", (int)encrypted[i]);
// 	return 0;
// }