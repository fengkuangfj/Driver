
//*****************************************************
//
//			Author:		blueapplez
//			Email:		blueapple1987@163.com
//			Home:		http://hi.baidu.com/blueapple_c/home
//			Time:		2011-05
//
//*********************************************************

#include <ntddk.h>
#include "Jgg.h"


#define REAL_SHIFT(i)	(i%8+1)
#define KEY_BYTE		0xE9
#define BYTEBIT			0x8

void Init_KeyBox(int m[256], unsigned char *key,  int length)
{
    int i=0, j=0, k=0;

    for( i = 0; i < 256; i++ )
    {
        m[i] = i;
    }
	
    for( i = 0; i < 256; i++ )
    {
        j = (unsigned char) ( j + m[i] + key[k] + j%7 + i%11);
		if( i != j)
		{
			m[i] ^= m[j]; 
			m[j] ^= m[i]; 
			m[i] ^= m[j];
		}
        if( ++k >= length ) k = 0;
    }
}

void Jgg_Encrypt(unsigned char IN *key, int iKeyLen, const unsigned char IN *inn, unsigned char OUT *out, int IN iSize)
{
	int i=0;
	int x=0;
	int y=0;
	int m[256] = {0};
	Init_KeyBox(m, key, iKeyLen);
	for (i=0; i<iSize; i++)
	{
		unsigned char a = inn[i] ^ KEY_BYTE ^ (unsigned char)i;
		unsigned char b = REAL_SHIFT(i);
// 		__asm
// 		{
// s1:
// 			rol a, 1
// 			dec b
// 			cmp b,0
// 			jg	s1
// 		}

		do
		{
			a = (a << 1) | a >> ((sizeof(unsigned char) * BYTEBIT) - 1);
			b--;
		} while (b > 0);

		x = (unsigned char)(x + 1);
		y = (unsigned char)(y + m[x]);
		if (x != y)
		{
			m[x] ^= m[y];
			m[y] ^= m[x];
			m[x] ^= m[y];
		}
		out[i] = a^(unsigned char)m[(unsigned char)(m[x]+m[y])];
	}
}

void Jgg_Decrypt(unsigned char IN *key, int iKeyLen, const unsigned char IN *inn, unsigned char OUT *out, int IN iSize)
{
	int i=0;
	int x=0;
	int y=0;
	int m[256] = {0};

	Init_KeyBox(m, key, iKeyLen);
	for (i=0; i<iSize; i++)
	{
		unsigned char a = 0;
		unsigned char b = 0;
		x = (unsigned char)(x + 1);
		y = (unsigned char)(y + m[x]);
		if (x != y)
		{
			m[x] ^= m[y];
			m[y] ^= m[x];
			m[x] ^= m[y];
		}

		a = inn[i]^(unsigned char)m[(unsigned char)(m[x]+m[y])];
		b = REAL_SHIFT(i);
// 		__asm
// 		{
// s1:
// 			ror a, 1
// 			dec b
// 			cmp b,0
// 			jg  s1
// 		}

		do
		{
			a = (a >> 1) | a << ((sizeof(unsigned char) * BYTEBIT) - 1);
			b--;
		} while (b > 0);

		out[i] = a ^ KEY_BYTE ^ (unsigned char)i;
	}
}

void Jgg_EncryptStream(
	__in PUCHAR key, 
	__in INT KeyLen, 
	__in PUCHAR Input, 
	__out PUCHAR Output, 
	__in INT Size )
{
	INT Offset = 0;
	CHAR Step = 4;
	INT Tail = Size % Step;
	INT Body = Size - Tail;


	while (Offset < Body)
	{
		Jgg_Encrypt(key,KeyLen,Input,Output,Step);

		Offset += Step;
		Input += Step;
		Output += Step;
	}

	if (Tail > 0)
	{
		Jgg_Encrypt(key,KeyLen,Input,Output,Tail);
	}
}

void Jgg_DecryptStream(
	__in PUCHAR key, 
	__in INT KeyLen, 
	__in PUCHAR Input, 
	__out PUCHAR Output, 
	__in INT Size )
{
	INT Offset = 0;
	CHAR Step = 4;
	INT Tail = Size % Step;
	INT Body = Size - Tail;


	while (Offset < Body)
	{
		Jgg_Decrypt(key,KeyLen,Input,Output,Step);

		Offset += Step;
		Input += Step;
		Output += Step;
	}

	if (Tail > 0)
	{
		Jgg_Decrypt(key,KeyLen,Input,Output,Tail);
	}
}
