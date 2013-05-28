// Base64.h: interface for the CBase64 class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BASE64_H__758269C1_A9DB_11D4_9A19_0050BAD44BCD__INCLUDED_)
#define AFX_BASE64_H__758269C1_A9DB_11D4_9A19_0050BAD44BCD__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



#ifndef _LPCBYTE_DEFINED
#define _LPCBYTE_DEFINED
typedef const BYTE *LPCBYTE;
#endif


class CBase64  
{

private:

	int DecodeChars(LPCSTR in, char *out);
	void EncodeChars(LPCSTR in, char *out, int count);

	int DecodeBytes(LPCSTR in, LPBYTE out);
	void EncodeBytes(LPCBYTE in, LPSTR out, int count);

	UCHAR FindBase64Val(const char character);

public:
	CBase64();
	virtual ~CBase64();

	BOOL DecodeString(LPCSTR szIn, LPSTR szOut, int nOutLen);
	BOOL EncodeString(LPCSTR szIn, LPSTR szOut, int nOutLen);

	BOOL DecodeData(LPCSTR szIn, LPBYTE szOut, int* nOutLen);
	BOOL EncodeData(LPCBYTE szIn, int nInLen, LPSTR szOut, int nOutLen);

};

#endif // !defined(AFX_BASE64_H__758269C1_A9DB_11D4_9A19_0050BAD44BCD__INCLUDED_)
