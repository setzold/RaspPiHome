/*
 *
 *  crccheck.c:
 *
 *  check characters against given crc polynoms
 *  
 */
 
#include "crccheck.h"

//CRC-8 Polynom 100111001

const char * makeCRC8Poly(char *bitString, char *result)
{
	char CRC[8];
	int  i;
	char DoInvert;

	for (i=0; i<8; ++i)
		CRC[i] = 0; 		// init all with 0

	for (i=0; i<strlen(bitString); ++i)
	{
		DoInvert = ('1'== bitString[i]) ^ CRC[7];         // XOR required?

		CRC[7] = CRC[6];
		CRC[6] = CRC[5];
		CRC[5] = CRC[4] ^ DoInvert;
		CRC[4] = CRC[3] ^ DoInvert;
		CRC[3] = CRC[2];
		CRC[2] = CRC[1];
		CRC[1] = CRC[0];
		CRC[0] = DoInvert;
	}

	for (i=0; i<8; ++i)
		result[7-i] = CRC[i] ? '1' : '0'; // Convert binary to ASCII

	result[8] = 0; // Set string terminator

	return result;
}