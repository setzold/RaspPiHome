/*
 * input from: http://stackoverflow.com/questions/8205298/convert-hex-to-binary-in-c
 *
 *  hexconverter.c:
 *
 *  converts hex string in binary string
 *  e.g.:0x12 -> 10010
 */

#include "hexconverter.h"

const char* quads[] = {
			"0000", "0001", "0010", "0011", "0100", "0101",
			"0110", "0111", "1000", "1001", "1010", "1011",
			"1100", "1101", "1110", "1111"
			};

const char * hex_to_bin_quad(unsigned char c)
{
	if (c >= '0' && c <= '9') return quads[     c - '0'];
	if (c >= 'A' && c <= 'F') return quads[10 + c - 'A'];
	if (c >= 'a' && c <= 'f') return quads[10 + c - 'a'];

	return quads[0];
}
