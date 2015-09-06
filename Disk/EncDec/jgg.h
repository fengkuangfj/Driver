
//*****************************************************
//
//			Author:		blueapplez
//			Email:		blueapple1987@163.com
//			Home:		http://hi.baidu.com/blueapple_c/home
//			Time:		2011-05
//
//*********************************************************

/*/******	【注意】:本算法不属于流加密算法, 流模式加密必须带一个跟随因子, 
					就像rc4算法中的那个结构体的 x y
					本算法加解密函数参数的缓冲大小要一致，并且要一次完成加解密 
					而流加密算法rc4则没有这个问题
					请原谅我  发了一个这么Jgg的算法.(- -!)
*/

#ifndef Jgg_INCLUDED
#define Jgg_INCLUDED

#ifndef IN
#define IN
#endif

#ifndef OUT
#define OUT
#endif

#ifdef __cplusplus
extern "C" 
{
#endif

	void Jgg_Encrypt(unsigned char IN *key, int iKeyLen, const unsigned char IN *inn, unsigned char OUT *out, int IN iSize);
	void Jgg_Decrypt(unsigned char IN *key, int iKeyLen, const unsigned char IN *inn, unsigned char OUT *out, int IN iSize);

	void Jgg_EncryptStream(
		__in PUCHAR key, 
		__in INT KeyLen, 
		__in PUCHAR Input, 
		__out PUCHAR Output, 
		__in INT Size );

	void Jgg_DecryptStream(
		__in PUCHAR key, 
		__in INT KeyLen, 
		__in PUCHAR Input, 
		__out PUCHAR Output, 
		__in INT Size );

#ifdef __cplusplus
}  /* end extern "C" */
#endif

#endif /* Jgg_INCLUDED */
