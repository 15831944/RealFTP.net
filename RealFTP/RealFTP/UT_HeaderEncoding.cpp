// =================================================================
//  class: CUT_HeaderEncoding
//  File:  UT_HeaderEncoding.cpp
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

#include "stdafx.h"
#include "UT_HeaderEncoding.h"
#include "UTErr.h"

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/***************************************************
CUT_HeaderEncoding
	Constructor
****************************************************/
CUT_HeaderEncoding::CUT_HeaderEncoding() : 
	m_lpszEncodedBuffer(NULL), 
	m_lpszDecodedBuffer(NULL),
	m_lpszCharSetBuffer(NULL),
	m_nLastError(UTE_SUCCESS)
{
	strcpy(m_szBase64Table,"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
}

/***************************************************
~CUT_HeaderEncoding
	Destructor
****************************************************/
CUT_HeaderEncoding::~CUT_HeaderEncoding()
{
	// Delete allocated memory
	delete [] m_lpszEncodedBuffer;
	delete [] m_lpszDecodedBuffer;
	delete [] m_lpszCharSetBuffer;
}

/***************************************************
Decode
	Decode the e-mail message header string using
	the BASE64 or quoted printable algorithm.
PARAM
	lpszText	- text to decode
	lpszCharSet	- (out) character set of the decoded text
RETURN
	Pointer to the decoded header or NULL in case of 
	error. Character set is returned through the 
	lpszCharSet parameter. You can get the last error 
	code using the GetLastError method.
****************************************************/
LPCSTR	CUT_HeaderEncoding::Decode(LPCSTR lpszText, LPSTR lpszCharSet)
{
	// Check parameters
	if(lpszText == NULL)
	{
		m_nLastError = UTE_PARAMETER_INVALID_VALUE;
		return NULL;
	}

	// Allocate memory for the decoded text and character set buffers
	delete [] m_lpszDecodedBuffer;
	delete [] m_lpszCharSetBuffer;
	m_lpszDecodedBuffer = new char[strlen(lpszText) + 10];
	m_lpszCharSetBuffer = new char[MAX_CHARSET_SIZE + 1];
	m_lpszDecodedBuffer[0] = NULL;
	m_lpszCharSetBuffer[0] = NULL;
	
	// Decode string
	char	*lpszIn = (char *)lpszText, *lpszOut = m_lpszDecodedBuffer;
	while(*lpszIn != NULL)
	{
		// Beginning of the encoded data was found
		if(*lpszIn == '=' && *(lpszIn + 1) == '?')
		{
			lpszIn += 2;

			//*************************************************
			//** Get the character set
			//*************************************************
			int nCharSetSize = 0;
			while(*lpszIn != '?')
			{
				m_lpszCharSetBuffer[nCharSetSize++] = *lpszIn++;
				if(nCharSetSize > MAX_CHARSET_SIZE)
				{
					m_nLastError = UTE_CHARSET_TOO_BIG;
					return NULL;
				}
			}
			m_lpszCharSetBuffer[nCharSetSize] = NULL;
			strcpy(lpszCharSet, m_lpszCharSetBuffer);
			++lpszIn;

			//*************************************************
			//** Get the encoding type
			//*************************************************
			char	cAlgorithm = *lpszIn;
			if(*(lpszIn + 1) != '?')
			{
				m_nLastError = UTE_INVALID_ENCODING_FORMAT;
				return NULL;
			}
			lpszIn += 2;


			//*************************************************
			//** Get encoded text
			//*************************************************
			char	*lpszEncodedText = lpszIn;
			int		nEncodedTextLength = 0;
			if(strlen(lpszEncodedText) < 2)
			{
				m_nLastError = UTE_INVALID_ENCODING_FORMAT;
				return NULL;
			}
			
			// v4.2 changed to eliminate C4127: conditional expression is constant
			for(;;)
			{
				if(*lpszIn == NULL)
				{
					m_nLastError = UTE_INVALID_ENCODING_FORMAT;
					return NULL;
				}

				// Check for the end of the data
				if(*lpszIn == '?' && *(lpszIn + 1) == '=')
				{
					lpszIn += 2;
					break;
				}

				// Go to the next character
				++lpszIn;
				++nEncodedTextLength;
			}

			//*************************************************
			//** Check encoded data size
			//*************************************************
			char	szEncodedData[200], szDecodedData[200];
			if(nEncodedTextLength > sizeof(szEncodedData) - 1)
			{
				m_nLastError = UTE_INVALID_ENCODING_FORMAT;
				return NULL;
			}
			strncpy(szEncodedData, lpszEncodedText, nEncodedTextLength);
			szEncodedData[nEncodedTextLength] = NULL;

			//*************************************************
			//** Check for CRLF+SPACE which means that the
			//** encoded line will be continued
			//*************************************************
			if(strlen(lpszIn) >= 3)
			{
				if(*lpszIn == '\r' && *(lpszIn + 1) == '\n' && (*(lpszIn + 2) == ' ' || *(lpszIn + 2) == '\t'))
				{
					// Ignore these characters
					lpszIn += 3;
				}
			}
			

			//*************************************************
			//** Get encoding algorithm
			//*************************************************
			int nResult;
			*szDecodedData = NULL;
			if(cAlgorithm == 'Q' || cAlgorithm == 'q')
			{
				nResult = DecodeQuotedPrintable(szEncodedData, szDecodedData, sizeof(szDecodedData));
			}
			else if(cAlgorithm == 'B' || cAlgorithm == 'b')
			{
				nResult = DecodeBase64(szEncodedData, szDecodedData, sizeof(szDecodedData));
			}
			else
			{
				m_nLastError = UTE_UNSUPPORTED_ENCODING_TYPE;
				return NULL;
			}

			//*************************************************
			//** Check for errors
			//*************************************************
			if(nResult != UTE_SUCCESS)
			{
				m_nLastError = nResult;
				return NULL;
			}

			//*************************************************
			//** Add decoded string to the result buffer
			//*************************************************
			strcpy(lpszOut, szDecodedData);
			lpszOut += strlen(szDecodedData);
		}

		// Copy non encoded character to the output buffer
		else
			*lpszOut++ = *lpszIn++;
	}
	// v4.2 fixes problem of trailing garbage when only first part of string is encoded...
	*lpszOut = '\0';	// terminate
	return m_lpszDecodedBuffer;
}

/***************************************************
Encode
	Enocde the e-mail message header string using
	the BASE64 or quoted printable algorithm.
PARAM
	lpszText		- text to encode
	lpszCharSet		- character set of the string
	[EncodingType]	- encoding algorithm (ENCODING_BASE64 or ENCODING_QUOTED_PRINTABLE)
RETURN
	Pointer to the encoded header or NULL in case of 
	error.You can get the last error code using the 
	GetLastError method.
****************************************************/
LPCSTR	CUT_HeaderEncoding::Encode(LPCSTR lpszText, LPCSTR lpszCharSet, enumEncodingType EncodingType)
{
	int		nHeaderSize;

	//*************************************************
	//** Check the parameters
	//*************************************************

	// The header to encode and the character set can not be epmty
	if(lpszText == NULL || lpszCharSet == NULL || (nHeaderSize=(int)strlen(lpszText)) == 0 || strlen(lpszCharSet) == 0)
	{
		m_nLastError = UTE_PARAMETER_INVALID_VALUE;
		return NULL;
	}

	// Check for the invalid characters in the character set string
	if(	strchr(lpszCharSet, '(') != NULL ||
		strchr(lpszCharSet, ')') != NULL ||
		strchr(lpszCharSet, '<') != NULL ||
		strchr(lpszCharSet, '>') != NULL ||
		strchr(lpszCharSet, '@') != NULL ||
		strchr(lpszCharSet, ',') != NULL ||
		strchr(lpszCharSet, ';') != NULL ||
		strchr(lpszCharSet, ':') != NULL ||
		strchr(lpszCharSet, '\"') != NULL ||
		strchr(lpszCharSet, '/') != NULL ||
		strchr(lpszCharSet, '[') != NULL ||
		strchr(lpszCharSet, ']') != NULL ||
		strchr(lpszCharSet, '?') != NULL ||
		strchr(lpszCharSet, '.') != NULL ||
		strchr(lpszCharSet, '=') != NULL )
	{
		m_nLastError = UTE_INVALID_CHARACTER_IN_CHARSET;
		return NULL;
	}

	// Char set name shoul not be more then 30 characters
	if(strlen(lpszCharSet) > MAX_CHARSET_SIZE)
	{
		m_nLastError = UTE_CHARSET_TOO_BIG;
		return NULL;
	}

	//*************************************************
	//** Allocate memory for the encoded header
	//*************************************************

	// Calculate the buffer size as 7 control characters plus size of the character set
	// string plus size of the header time 3 (the string will grow while encoding but no
	// more then 3 times). One extra character for the terminating NULL.
	long lBufferSize = ((nHeaderSize * 3) / (75 - 10 - (int)strlen(lpszCharSet)) + 2) * 75 + 1;

	// Allocate the memory
	delete [] m_lpszEncodedBuffer;
	m_lpszEncodedBuffer = new char[lBufferSize];
	m_lpszEncodedBuffer[0] = NULL;
	

	//*************************************************
	//** Encode the header
	//*************************************************
	if(EncodingType == ENCODING_BASE64)
		EncodeBase64(lpszText, lpszCharSet);
	else
		EncodeQuotedPrintable(lpszText, lpszCharSet);

	// Return the pointer to the string
	return m_lpszEncodedBuffer;
}

/***************************************************
DecodeBase64Bytes()
    Change the passed string to its representation before 
    it was encoded in base64 format.

    Decodes 4 characters (possiblly less) to 3 characters.
    
    base64 encoding blocks are padded with '=' at the
    end of the file.  Base64 is based on the 
    concept of taking 3 8bit bytes and converting them
    into 4 6bit bytes.  It is possible that you 
    may have 1, 2 or 3 used 8bit bytes to 
    encode.  As base64 requires encoded output in
    groups of 4 6bit bytes, padding is necessary.
PARAM
    unsigned char *in - the encoded octet string
    unsigned char *out - The decoded octet string
RETURN
     1 - the result is one 8bit byte since the first two bytes were used
     2 - only the first three bytes were used, resulting in 2 8bit bytes.
     3 - all bytes were used.
****************************************************/
int CUT_HeaderEncoding::DecodeBase64Bytes(unsigned char *in, unsigned char *out)
{
    UCHAR c1, c2, c3, c4;
    
    c1 = FindBase64Val(in[0]);
    c2 = FindBase64Val(in[1]);      
    c3 = FindBase64Val(in[2]);
    c4 = FindBase64Val(in[3]);
    
    out[0] = (UCHAR)(((c1 & 63)<<2) | ((c2 & 48)>>4));
    out[1] = (UCHAR)(((c2 & 15)<<4) | ((c3 & 60)>>2));
    out[2] = (UCHAR)(((c3 & 3)<<6) | (c4 & 63));
    
    // c1 and c2 were used (the mimimum number possible)
    // and resulted in one 8bit byte.
    if (in[2] == '=')
        return 1;
    
    // c1, c2 and c3 were used, resuting in 2 8bit bytes.
    if (in[3] == '=')
        return 2;
    
    // all bytes were used.
    return 3;
}

/***************************************************
FindBase64Val
    Find the respresentation of the passed charecter in the base64 
    Table. The Base64 Alphabet as described by RFC 2045 is 
----------------------------------------------------------------
Value Encoding |  Value Encoding|  Value Encoding | Value Encoding
------------------------------------------------------------------
         0 A            17 R            34 i            51 z
         1 B            18 S            35 j            52 0
         2 C            19 T            36 k            53 1
         3 D            20 U            37 l            54 2
         4 E            21 V            38 m            55 3
         5 F            22 W            39 n            56 4
         6 G            23 X            40 o            57 5
         7 H            24 Y            41 p            58 6
         8 I            25 Z            42 q            59 7
         9 J            26 a            43 r            60 8
        10 K            27 b            44 s            61 9
        11 L            28 c            45 t            62 +
        12 M            29 d            46 u            63 /
        13 N            30 e            47 v
        14 O            31 f            48 w         (pad) =
        15 P            32 g            49 x
        16 Q            33 h            50 y
--------------------------------------------------------------------
    SEE Constructor of this class for m_szBase64Table
PARAM
    char character - the charecter to find the value for
RETURN
    UCHAR   - the corresponding character position from the table
    -1      - Not found
****************************************************/
UCHAR CUT_HeaderEncoding::FindBase64Val(char character)
{
    for (UCHAR pos = 0; pos < 64; pos ++)
        if (m_szBase64Table[pos] == character)
            return pos;
        
	return (UCHAR)-1;
}

/***************************************************
EncodeBase64Bytes()
    Encodes 3 characters and returns 4 characters in accordance
    with Base64 scheme
PARAM
    unsigned char *in - the encoded octet string
    unsigned char *out - The decoded octete string
    int count - number of bytes to use in case it is the end of string
        to pad the character "="
RETURN 
    VOID
****************************************************/
void CUT_HeaderEncoding::EncodeBase64Bytes(unsigned char *in, unsigned char *out, int count)
{
    int index1 = ((in[0] >> 2) & 63);
    int index2 = (((in[0] << 4) & 48) | ((in[1] >> 4) & 15));
    int index3 = (((in[1] << 2) & 60) | ((in[2] >> 6) & 3));
    int index4 = (in[2] & 63);
    
    out[0] = m_szBase64Table[index1];
    out[1] = m_szBase64Table[index2];
    out[2] = m_szBase64Table[index3];
    out[3] = m_szBase64Table[index4];
    
    switch(count)
	{
		case 1:
			out[2]='=';
		case 2:
			out[3]='=';
    }
}

/***************************************************
EncodeBase64
	Enocde the header using the BASE64 algorithm
PARAM
	lpszText	- header string to encode
	lpszCharSet	- character set of the string
RETURN
	Nothing
****************************************************/
void CUT_HeaderEncoding::EncodeBase64(LPCSTR lpszText, LPCSTR lpszCharSet)
{
	// Check parameters
	if(lpszText == NULL || lpszCharSet == NULL)
		return;

	// Initialize buffer
	*m_lpszEncodedBuffer = NULL;

	// Get string sizes and number of result lines.
	// NOTE: Each line cannot be more then 75 bytes.
	int nHeaderSize = (int)strlen(lpszText);
	int nCharSetSize = (int)strlen(lpszCharSet);
	int nNumberOfLines = nHeaderSize / (49 - nCharSetSize) + 1;
	int nCurPointer = 0, nProcessBytes = 0;
	char	out[5];
	 
	// Loop throuh each header line
	for(int i = 0; i < nNumberOfLines; i++)
	{
		// Separate all the lines but first with CRLF SPACE 
		if(i != 0)
			strcat(m_lpszEncodedBuffer, "\r\n ");

		// Put in starting headers & mark
		strcat(m_lpszEncodedBuffer, "=?");
		strcat(m_lpszEncodedBuffer, lpszCharSet);
		strcat(m_lpszEncodedBuffer, "?B?");

        // Encode the line
		nProcessBytes = min((UINT)(49 - nCharSetSize), strlen(&lpszText[nCurPointer]));

        for(int j = 0; j < nProcessBytes; j += 3) {
            EncodeBase64Bytes((unsigned char*)&lpszText[nCurPointer + j], (unsigned char*)out, ((nProcessBytes - j > 3) ? 3 : nProcessBytes - j) );
			strncat(m_lpszEncodedBuffer, out, 4);
	        }
		nCurPointer += nProcessBytes;
		
		// Put in the ending mark
		strcat(m_lpszEncodedBuffer, "?=");
	}

}

/***************************************************
DecodeBase64
	Deocde the header text using the BASE64 algorithm
PARAM
	lpszText	- header string to decode
	lpszBuffer	- buffer for the decoded text
	nBufferSize	- buffer size
RETURN
	CUT_SUCCESS
	UTE_NULL_PARAM
	UTE_BUFFER_TOO_SHORT
****************************************************/
int CUT_HeaderEncoding::DecodeBase64(LPCSTR lpszText, LPSTR lpszBuffer, int nBufferSize)
{
	// Check parameters
	if(lpszText == NULL || lpszBuffer == NULL)
		return UTE_NULL_PARAM;

	// Check the buffer size
	if(nBufferSize < (int) strlen(lpszText))
		return UTE_BUFFER_TOO_SHORT;

	// Check that decoded string is padded with '=' characters at the end
	char	*lpszNewBuffer = new char[strlen(lpszText) + 5], *lpszIn = lpszNewBuffer, *lpszOut = lpszBuffer;
	strcpy(lpszIn, lpszText);
	while(strlen(lpszIn)%4 != 0)
		strcat(lpszIn, "=");

	// Decode string 
	while(*lpszIn != NULL)
	{
		int nBytesDecoded = DecodeBase64Bytes((unsigned char *)lpszIn, (unsigned char *)lpszOut);
		lpszIn += 4;
		lpszOut += nBytesDecoded;
		*lpszOut = NULL;
	}

	// Delete allocated bufefr
	delete [] lpszNewBuffer;

	return CUT_SUCCESS;
}

/***************************************************
DecodeQuotedPrintable
	Deocde the header text using the Quoted 
	Printable algorithm
PARAM
	lpszText	- header string to decode
	lpszBuffer	- buffer for the decoded text
	nBufferSize	- buffer size
RETURN
	CUT_SUCCESS
	UTE_ERROR
	UTE_NULL_PARAM
	UTE_BUFFER_TOO_SHORT
****************************************************/
int CUT_HeaderEncoding::DecodeQuotedPrintable(LPCSTR lpszText, LPSTR lpszBuffer, int nBufferSize)
{
	// Check parameters
	if(lpszText == NULL || lpszBuffer == NULL)
		return UTE_NULL_PARAM;

	// Check the buffer size
	if(nBufferSize < (int) strlen(lpszText))
		return UTE_BUFFER_TOO_SHORT;

	// Decode string
	char	*lpszIn = (char *)lpszText, *lpszOut = lpszBuffer;
	while(*lpszIn != NULL)
	{
		if(*lpszIn == '=')
		{
			// Check formart
			if(strlen(lpszIn) < 3)
				return UTE_ERROR;

			*lpszOut = (char)HexToDec(lpszIn + 1, 2);
			lpszIn += 2;

		}
		else if(*lpszIn == '_')
			*lpszOut = ' ';
		else
			*lpszOut = *lpszIn;

		// Go to the next char
		++lpszOut;
		++lpszIn;
	}

	// Set NULL at the end of the string
	*lpszOut = NULL;

	return CUT_SUCCESS;
}

/***************************************************
HexToDec()
    Change a Hex string presentation to a decimal number
PARAM
    char *hexVal - The hexadecimal string value 
    int numDigits - number of digits in the string
RETURN
    Decimal presentation of the Hexa value
****************************************************/
long CUT_HeaderEncoding::HexToDec(char *hexVal, int numDigits)
{
    long num=0;
    int base = 1;
    int index;
    for (int i = numDigits -1 ;i >=0;i--){
        if(hexVal[i] >= '0' && hexVal[i] <='9')
            index = hexVal[i] -'0';
        else
            index = hexVal[i] - 'A' + 10;
        
        num += index * base;
        base *= 16;
    }   
    
    return num;
}

/***************************************************
EncodeQuotedPrintable
	Enocde the header using the Quoted-Printable
PARAM
	lpszText	- header string to encode
	lpszCharSet	- character set of the string
RETURN
	Nothing
****************************************************/
void CUT_HeaderEncoding::EncodeQuotedPrintable(LPCSTR lpszText, LPCSTR lpszCharSet)
{
	unsigned char hexList[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

	// Check parameters
	if(lpszText == NULL || lpszCharSet == NULL)
		return;

	// Initialize buffer
	*m_lpszEncodedBuffer = NULL;

	// Get string sizes and number of result lines.
	// NOTE: Each line cannot be more then 75 bytes.
	int nCharSetSize = (int)strlen(lpszCharSet);
	int nCurPointer = 0, nPerLineCounter = 0;
	int nCurOutPointer = (int)strlen(m_lpszEncodedBuffer);
	 

	// Loop throuh each header line
	while(lpszText[nCurPointer] != NULL)
	{
		BOOL bProcessed = FALSE;

		if(nCurPointer == 0 || nPerLineCounter >= (75 - nCharSetSize - 4))
		{
			m_lpszEncodedBuffer[nCurOutPointer] = NULL;

			// Put in the ending mark & delimiter
			if(nCurPointer != 0)
				strcat(m_lpszEncodedBuffer, "?=\r\n ");

			// Put in starting headers & mark
			strcat(m_lpszEncodedBuffer, "=?");
			strcat(m_lpszEncodedBuffer, lpszCharSet);
			strcat(m_lpszEncodedBuffer, "?Q?");

			nPerLineCounter = (int)strlen(lpszCharSet) + 5; 
			nCurOutPointer = (int)strlen(m_lpszEncodedBuffer);
		}

        // If the character value is in the defined 'acceptable'
        // range, it is appropriate to simply copy the character 
        // into the output message
        
        // RFC 2045 has also indicated that quoted printable encoding 
        // may not be reliable over EBCDIC gateways common on IBM mainframe
        // systems.  In order to improve reliability it may be necessary to 
        // for the following symbols to also be 'quoted' as hexidecimal values
        // in the output stream, rather than forwarding them through normally
        // the symbols:  !"#@[\]^`{|}~
        
        if ((lpszText[nCurPointer] >= 33 && lpszText[nCurPointer] <= 60) || 
			(lpszText[nCurPointer] >= 62 && lpszText[nCurPointer] <= 126)) 
		{
			// Special cases
			if(	lpszText[nCurPointer] != ' ' &&
				lpszText[nCurPointer] != ',' &&
				lpszText[nCurPointer] != ';' &&
				lpszText[nCurPointer] != '\t' &&
				lpszText[nCurPointer] != '=' &&
				lpszText[nCurPointer] != '?' &&
				lpszText[nCurPointer] != '_' &&
				lpszText[nCurPointer] != '!' &&
				lpszText[nCurPointer] != '\"' &&
				lpszText[nCurPointer] != '@' &&
				lpszText[nCurPointer] != '[' &&
				lpszText[nCurPointer] != ']' &&
				lpszText[nCurPointer] != '\\' &&
				lpszText[nCurPointer] != '^' &&
				lpszText[nCurPointer] != '\'' &&
				lpszText[nCurPointer] != '{' &&
				lpszText[nCurPointer] != '}' &&
				lpszText[nCurPointer] != '|' &&
				lpszText[nCurPointer] != '~')
			{
				m_lpszEncodedBuffer[nCurOutPointer] = lpszText[nCurPointer];
				bProcessed = TRUE;
				nCurOutPointer++;
			}
		}
        
        // For any other character that does not fit into the previous
        // categories, we must convert it into hexidecimal form, prepend it
        // with an '=' and add it to the output stream.
        
        if (!bProcessed) {
            m_lpszEncodedBuffer[nCurOutPointer++] = '=';
            m_lpszEncodedBuffer[nCurOutPointer++] = hexList[(int)((unsigned char)lpszText[nCurPointer]/16)];
            m_lpszEncodedBuffer[nCurOutPointer++] = hexList[(int)((unsigned char)lpszText[nCurPointer]%16)];
			nPerLineCounter += 2;
			}
		
		++nCurPointer;
		++nPerLineCounter;
	}

	// Put in the ending mark
	m_lpszEncodedBuffer[nCurOutPointer] = NULL;
	strcat(m_lpszEncodedBuffer, "?=");

}

/***************************************************
IsEncoded
	Determines if the given text is encoded

PARAM
	lpszText	- text to test
RETURN
	TRUE		- the test is encoded
	FALSE		- the text is not encoded
****************************************************/
BOOL CUT_HeaderEncoding::IsEncoded(LPCSTR lpszText)
{
	// pre v4.2 - If the text starts with "=?" and ends with "?=" it is encoded
	// v4.2 revised to detect embedded encoding 
	BOOL retval = false;

	if (lpszText == NULL) {
		retval = FALSE;
	}
	else {

		int iLength = (int)::strlen(lpszText);

		if (iLength < 4) {
			retval = FALSE;
		}
		else {
			int index = 0;

			// ignore leading spaces AND tabs...
			while (lpszText[index] == ' ' || lpszText[index] == '"' || lpszText[index] == '\t') ++ index;

			// ignore possible "Re: " etc... string could end with non-decoded "=??=" ??
			while (index < iLength-4 && lpszText[index] != '=' && lpszText[index+1] != '?') ++ index;

			if (index < iLength -4) {
				for(index+=2; index < iLength -1; ++index) {
					// look for enclosing '?='
					if( lpszText[index] == '?' && lpszText[index+1] == '=') {
						retval = true;
						break;
					}
				}
			}
		}
	}
	return retval;
}

#pragma warning ( pop )