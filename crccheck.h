/*
 *
 *  crccheck.h:
 *
 *  check characters against given crc polynoms
 *  
 */

#ifndef CRCCHECK_H
#define CRCCHECK_H

#include <stdio.h>
#include <string.h>

//CRC-8 Polynom 100111001
const char * makeCRC8Poly(char *, char *);

#endif /* CRCCHECK_H */
