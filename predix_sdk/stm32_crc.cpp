#include <pch.h>
#include <stdbool.h>
#include <stdlib.h>
#include "stm32_crc.h"



#ifdef _DEBUG
#define new DEBUG_NEW
#endif



width crcTable[256];

static bool is_crcTable_inited = false;

void crcInit(void)
{
	width	remainder;
	width	dividend;
	int		bit;

	/* Perform binary long division, a bit at a time. */
	for (dividend = 0; dividend < 256; dividend++)
	{
		/* Initialize the remainder. */
		remainder = dividend << (WIDTH - 8);

		/* Shift and XOR with the polynomial. */
		for (bit = 0; bit < 8; bit++)
		{
			/* Try to divide the current data bit. */
			if (remainder & TOPBIT)
			{
				remainder = (remainder << 1) ^ POLYNOMIAL;
			}
			else
			{
				remainder = remainder << 1;
			}
		}

		/* Save the result in the table. */
		crcTable[dividend] = remainder;
	}

	is_crcTable_inited = true;
}

width crcCompute(unsigned char *message, unsigned int nBytes)
{
	unsigned int	offset;
	unsigned char	byte;
	width			remainder = INITIAL_REMAINDER;

	if (!is_crcTable_inited)
	{
		crcInit();
	}

	/* Divide the message by the polynomial, a byte at time. */
	for (offset = 0; offset < nBytes; offset++)
	{
		byte = (remainder >> (WIDTH - 8)) ^ message[offset];
		remainder = crcTable[byte] ^ (remainder << 8);
	}

	/* The final remainder is the CRC result. */
	return (remainder ^ FINAL_XOR_VALUE);
}


// kiwa72(2022.11.24 17h) - CRC-32/MPEG-2 검증 코드
#define CRC_INIT	0xFFFFFFFF
#define CRC_POLY	0x04C11DB7
width crcCompute32(unsigned int* message, unsigned int nBytes)
{
	size_t i, j;
	unsigned int crc, msb;

	crc = CRC_INIT;
	for (i = 0; i < nBytes; i++)
	{
		// xor next byte to upper bits of crc
		crc ^= ((unsigned int)message[i]);

		unsigned int cc = (unsigned int)message[i];
		cc += 0;

		int a = 1;
		a = 3;

		for (j = 0; j < 32; j++)
		{
			msb = crc >> 31;
			if (msb == 1)
			{
				crc = (crc << 1) ^ 0x04C11DB7;
			}
			else
			{
				crc = crc << 1;
			}
		}
	}

	return crc;
}
