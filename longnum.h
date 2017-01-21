#ifndef __LONGNUM_H_
#define __LONGNUM_H_

uint8_t longzero(uint8_t num[], uint8_t len);

void longshl(uint8_t num[], uint8_t len, uint8_t bits);
void longshr(uint8_t num[], uint8_t len, uint8_t bits);
int8_t longcmp(uint8_t d1[], uint8_t d2[], uint8_t len);

void longadd(uint8_t d1[], uint8_t len1, uint8_t d2[], uint8_t len2);
void longsub(uint8_t d1[], uint8_t d2[], uint8_t len);

void longdiv(uint8_t d1[], uint8_t len1, uint8_t d2[], uint8_t len2,
	uint8_t result[], uint8_t remainder[], uint8_t work[]);

void longmult(uint8_t d1[], uint8_t len1, uint8_t d2[], uint8_t len2,
	uint8_t result[], uint8_t work[]);

void mathbuf_set(uint8_t buf[], uint8_t size, uint32_t value);
uint32_t mathbuf_get(uint8_t buf[], uint8_t size);


#endif
