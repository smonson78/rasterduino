#include <string.h>
#include <stdint.h>

#include "longnum.h"

uint8_t longzero(uint8_t num[], uint8_t len)
{
	uint8_t i;
	for (i = 0; i < len; i++)
	{
		if (num[i] > 0)
			return 0;
	}

	return 1;
}

void longshl(uint8_t num[], uint8_t len, uint8_t bits)
{
	uint8_t i;
	uint8_t temp;

	while (bits > 0)
	{
		if (bits < 8)
		{
			num[0] <<= bits;

			/* Shift 1 to 7 bits */
			for (i = 1; i < len; i++)
			{
				/* Save shifted-out bits */
				temp = num[i] >> (8 - bits);

				/* Move bits up */
				num[i] <<= bits;

				/* Add saved bits to leftwards byte */
				num[i - 1] |= temp;
			}
			
			bits = 0;
		}
		else
		{
			/* 8 or more - shift by one byte */
			for (i = 0; i < len - 1; i++)
				num[i] = num[i + 1];	

			num[i] = 0;
			bits -= 8;
		}
	}
}

void longshr(uint8_t num[], uint8_t len, uint8_t bits)
{
	int8_t i;
	uint8_t temp;

	while (bits > 0)
	{
		if (bits < 8)
		{
			num[len - 1] >>= bits;

			/* Shift 1 to 7 bits */
			for (i = len - 2; i >= 0; i--)
			{
				/* Save shifted-out bits */
				temp = num[i] << (8 - bits);

				/* Move bits right */
				num[i] >>= bits;

				/* Add saved bits to rightwards byte */
				num[i + 1] |= temp;
			}
			
			bits = 0;
		}
		else
		{
			/* 8 or more - shift by one byte */
			for (i = len - 1; i > 0; i--)
				num[i] = num[i - 1];	

			num[0] = 0;
			bits -= 8;
			break;
		}
	}
}

/* Compare two numbers with the same length (result positive if
   first number is bigger) */
int8_t longcmp(uint8_t d1[], uint8_t d2[], uint8_t len)
{
	/* Size of operands in bits */
	uint8_t i;

	/* Compare byte at a time */
	for (i = 0; i < len; i++)
	{
		if (d1[i] > d2[i])
			return 1;
		else if (d1[i] < d2[i])
			return -1;
	}

	/* Same */
	return 0;
}

/* Add two big numbers - only compares lowest (len1)bytes of both terms */
void longadd(uint8_t d1[], uint8_t len1, uint8_t d2[], uint8_t len2)
{
	int8_t i;
	uint16_t temp;
	uint8_t carry = 0;

	/* Start at the right */
	for (i = 0; i < len1; i++)
	{
		temp = d1[len1 - i - 1];
		temp += carry;

		if (i < len2)
		{
			temp += d2[len2 - i - 1];
		}

		d1[len1 - i - 1] = temp & 0xff;
		carry = temp >> 8;
	}
}


void longsub(uint8_t d1[], uint8_t d2[], uint8_t len)
{
	int8_t i;
	uint8_t borrow = 0;

	/* Start at the right */
	for (i = len - 1; i >= 0; i--)
	{
		/* Previous borrow */
		if (d1[i] >= borrow)
		{
			d1[i] -= borrow;
			borrow = 0;
		}
		else
		{
			d1[i] -= borrow;
			borrow = 1;
		}

		/* Borrow needed? */
		borrow += d1[i] >= d2[i] ? 0 : 1;

		/* Subtract */
		d1[i] -= d2[i];
	}
}

static void setbit(uint8_t num[], uint8_t len, uint8_t bit)
{
	uint8_t byte = len - 1 - (bit / 8);
	bit = bit % 8;

	if (byte < len)
		num[byte] |= (1 << bit);
}

void longdiv(uint8_t d1[], uint8_t len1, uint8_t d2[], uint8_t len2,
	uint8_t result[], uint8_t remainder[], uint8_t work[])
{
	/* Divide d1[] by d2[].
	   Place result in result[] and remainder[].
	   Obliterates work[] which is same size as d1[]. */

	int8_t magnitude;

	/* Copy divisor to working area */
	memcpy(work + (len1 - len2), d2, len2);
	memset(work, 0, len1 - len2);

	/* Test for divide by zero */
	if (longzero(d2, len2))
	{
		/* As close to infinity as possible */
		memset(result, 0xff, len1);
		memset(remainder, 0, len1);
		return;
	}
	
	/* Move divisor up to top bit of work area */
	for (magnitude = 0; (work[0] & 0x80) == 0; magnitude++)
		longshl(work, len1, 1);

	/* Copy dividend into remainder */
	memcpy(remainder, d1, len1);

	/* Clear result */
	memset(result, 0, len1);
	
	while (magnitude >= 0)
	{
		/* Compare remainder with divisor work area */

		/* If possible to do so... */
		if (longcmp(remainder, work, len1) >= 0)
		{
			/* Subtract divisor from remainder */
			longsub(remainder, work, len1);

			/* Set bit in result */
			setbit(result, len1, magnitude);
		}

		longshr(work, len1, 1);
		magnitude--;
	}
	
}

/* Multiplication algorithm as used by ancient egyptian scribes */
void longmult(uint8_t d1[], uint8_t len1, uint8_t d2[], uint8_t len2,
	uint8_t result[], uint8_t work[])
{
	/* Multiply d1[] by d2[].
	   Place result in result[].
	   Obliterates work[] which must be size of d1 plus d2. */
	uint8_t i;
	int j;
	uint8_t worklen = len1 + len2;

	/* For efficiency (fewer adds) d2 should be shorter */

	/* Copy d1 into work buffer */
	memcpy(work + len2, d1, len1);
	memset(work, 0, len2);

	/* Clear result (same size as worklen) */
	memset(result, 0, worklen);

	/* Once for each bit of d2 */
	for (i = 0; i < (len2 * 8); i++)
	{
		j = len2 - (i / 8) - 1;
		if (d2[j] & (1 << (i % 8)))
			longadd(result, worklen, work, worklen);
		longshl(work, worklen, 1);
	}
}

void mathbuf_set(uint8_t buf[], uint8_t size, uint32_t value)
{
	uint8_t i;
	for (i = 0; i < size - 4; i++)
		buf[i] = 0;
	buf[size - 1] = value & 0xff;
	buf[size - 2] = (value >> 8) & 0xff;
	buf[size - 3] = (value >> 16) & 0xff;
	buf[size - 4] = (value >> 24) & 0xff;
}

uint32_t mathbuf_get(uint8_t buf[], uint8_t size)
{
	uint32_t temp;
	temp = buf[size - 4] & 0xff;
	temp <<= 8;
	temp |= buf[size - 3] & 0xff;
	temp <<= 8;
	temp |= buf[size - 2] & 0xff;
	temp <<= 8;
	temp |= buf[size - 1] & 0xff;

	return temp;
}

