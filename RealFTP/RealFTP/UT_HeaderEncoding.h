// =================================================================
//  class: CUT_HeaderEncoding
//  File:  UT_HeaderEncoding.h
//  
//  Purpose:
//
//  RFC 2047: MIME Part III. Message Header Extensions for Non-ASCII Text
//
// ===================================================================
// Ultimate TCP/IP v4.2
// This software along with its related components, documentation and files ("The Libraries")
// is © 1994-2007 The Code Project (1612916 Ontario Limited) and use of The Libraries is
// governed by a software license agreement ("Agreement").  Copies of the Agreement are
// available at The Code Project (www.codeproject.com), as part of the package you downloaded
// to obtain this file, or directly from our office.  For a copy of the license governing
// this software, you may contact us at legalaffairs@codeproject.com, or by calling 416-849-8900.
// ===================================================================

#if !defined(AFX_UT_HEADERENCODING_H__5E5288CB_0EEC_11D4_A59B_0080C858F182__INCLUDED_)
#define AFX_UT_HEADERENCODING_H__5E5288CB_0EEC_11D4_A59B_0080C858F182__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define MAX_CHARSET_SIZE 40		// v4.2 added - was hard coded in Decode fn
								// from RFC 1700 40 ASCII chars, case insensitive

typedef enum enumEncodingType 
{
	ENCODING_BASE64,
	ENCODING_QUOTED_PRINTABLE,
	ENCODING_NONE
};

class CUT_HeaderEncoding  
{
private:
	char	m_szBase64Table[128];   // Buffer to hold the base64 character table
	LPSTR	m_lpszEncodedBuffer;	// Encoded header text buffer
	LPSTR	m_lpszDecodedBuffer;	// Decoded header text buffer
	LPSTR	m_lpszCharSetBuffer;	// Decoded char set buffer
	int		m_nLastError;			// Last error code

	// Encoding helper functions
    void	EncodeBase64Bytes(unsigned char *in,unsigned char *out, int count);
	void	EncodeBase64(LPCSTR lpszHeader, LPCSTR lpszCharSet);
	void	EncodeQuotedPrintable(LPCSTR lpszHeader, LPCSTR lpszCharSet);
	UCHAR	FindBase64Val(char character);
	int		DecodeBase64Bytes(unsigned char *in, unsigned char *out);
	int		DecodeBase64(LPCSTR lpszHeader, LPSTR lpszBuffer, int nBufferSize);
	long	HexToDec(char *hexVal, int numDigits);
	int		DecodeQuotedPrintable(LPCSTR lpszText, LPSTR lpszBuffer, int nBufferSize);


public:
	CUT_HeaderEncoding();
	virtual ~CUT_HeaderEncoding();

	// Encode the message header text
	virtual LPCSTR	Encode(LPCSTR lpszText, LPCSTR lpszCharSet, enumEncodingType EncodingType = ENCODING_QUOTED_PRINTABLE);

	// Decode the message header text
	virtual LPCSTR	Decode(LPCSTR lpszText, LPSTR lpszCharSet);

	// Return the last error code
	virtual int		GetLastError() { return m_nLastError; }

	virtual BOOL IsEncoded(LPCSTR lpszText);

};

#endif // !defined(AFX_UT_HEADERENCODING_H__5E5288CB_0EEC_11D4_A59B_0080C858F182__INCLUDED_)
