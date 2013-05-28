/* =================================================================
// Ultimate TCP/IP v4.2
// This software along with its related components, documentation and files ("The Libraries")
// is © 1994-2007 The Code Project (1612916 Ontario Limited) and use of The Libraries is
// governed by a software license agreement ("Agreement").  Copies of the Agreement are
// available at The Code Project (www.codeproject.com), as part of the package you downloaded
// to obtain this file, or directly from our office.  For a copy of the license governing
// this software, you may contact us at legalaffairs@codeproject.com, or by calling 416-849-8900.
// 
// =================================================================
// This is a derived work from RFC 1321
// http://www.csl.sony.co.jp/cgi-bin/hyperrfc?rfc1321.txt
// Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
// rights reserved.
// 
// License to copy and use this software is granted provided that it
// is identified as the "RSA Data Security, Inc. MD5 Message-Digest
// Algorithm" in all material mentioning or referencing this software
// or this function.
// 
// License is also granted to make and use derivative works provided
// that such works are identified as "derived from the RSA Data
// Security, Inc. MD5 Message-Digest Algorithm" in all material
// mentioning or referencing the derived work.
// 
// RSA Data Security, Inc. makes no representations concerning either
// the merchantability of this software or the suitability of this
// software for any particular purpose. It is provided "as is"
// without express or implied warranty of any kind.
// 
// These notices must be retained in any copies of any part of this
// documentation and/or software.
// 
// =================================================================*/
#include "stdafx.h"
#include "md5.h"
#include <windows.h>
#include <assert.h>
#include <string.h>
#include <math.h>

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)

#define S11 7
#define S12 12
#define S13 17
#define S14 22
#define S21 5
#define S22 9
#define S23 14
#define S24 20
#define S31 4
#define S32 11
#define S33 16
#define S34 23
#define S41 6
#define S42 10
#define S43 15
#define S44 21

/************************************************************
************************************************************/
MD5::MD5(){

  // Prepare Table 
	PrepareTable();
  init();
}
/***********************************************************
PrepareTable
	Prepares the magic numbers table.
	As specified by RFC 1321 these numbers are generated as follow:

	"a 64-element table T[1 ... 64] constructed from the
	sine function. Let T[i] denote the i-th element of the table, which
	is equal to the integer part of 4294967296 times abs(sin(i)), where i
	is in radians "

REMARK
	 The reason I added the implementation instead of 
	 hard coding the values is that this will be more secure 
	 since no possibility of finding the finger print of hard coded values 
	 in the executables !!! 
	 If you disagree with me, then feel free to hard code 
	 the values of m_nTable with the ones from RFC 1321
PARAM:
	NONE
RET:
	VOID
***********************************************************/
void MD5::PrepareTable()
{
	double sinRes = 0; // result of the sine value
	double prod = 4294967296; // MD5 Magic Number

	int index = 0;
	for (double i = 1; i < 65 ; i++)
	{
		// get the sine form the item
		sinRes = sin (i);
		prod = fabs(sinRes) * 4294967296;

		// we are only intersted in the integer part 
		m_nTable[index] = (int)prod;
		index++;
	}

}
/********************************************************* 
MD5 block update operation. Continues an MD5 message-digest
operation, processing another message block, and updating the context. 

**********************************************************/
int MD5::Update (BYTE *In, UINT Inlength) {

  UINT uInputIndex, buffer_index;
  UINT uBufferSpace;                // how much space is left in Buffer

  if (m_Finalized)
  {  
    //Can't Update a Finalized Digest!;
    return 1;
  }

  // Compute number of bytes mod 64
  buffer_index = (unsigned int)((m_uCount[0] >> 3) & 0x3F);

  // Update number of bits
  if (  (m_uCount[0] += ((UINT) Inlength << 3))<((UINT) Inlength << 3) )
    m_uCount[1]++;

  m_uCount[1] += ((UINT)Inlength >> 29);


  uBufferSpace = 64 - buffer_index;  // how much space is left in Buffer

  // Transform as many times as possible.
  if (Inlength >= uBufferSpace)
  { // ie. we have enough to fill the pBuffer
    // fill the rest of the m_pBuffer and Transform
    memcpy (m_pBuffer + buffer_index, In, uBufferSpace);
    Transform (m_pBuffer);

    // now, Transform each 64-byte piece of the In, bypassing the m_pBuffer
    for (uInputIndex = uBufferSpace; uInputIndex + 63 < Inlength; 
	 uInputIndex += 64)
      Transform (In+uInputIndex);

    buffer_index = 0; 
  }
  else
    uInputIndex=0;    


  // and here we do the buffering:
  memcpy(m_pBuffer+buffer_index, In+uInputIndex, Inlength-uInputIndex);
  return 0;
}

/**************************************************************

  MD5 finalization. Ends an MD5 message-digest operation, writing the
the message digest and zeroizing the context. 

***************************************************************/
int  MD5::Finalize (){

  unsigned char bits[8];
  unsigned int index, padLen;
  static BYTE PADDING[64]={
    0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

  if (m_Finalized){
    // Already m_Finalized this m_pDigest!
    return 1;
  }

  // Save number of bits
  Encode (bits, m_uCount, 8);

  // Pad out to 56 mod 64.
  index = (UINT) ((m_uCount[0] >> 3) & 0x3f);
  padLen = (index < 56) ? (56 - index) : (120 - index);
  Update (PADDING, padLen);

  // Append length (before padding)
  Update (bits, 8);

  // Store State in Digest
  Encode (m_pDigest, m_uState, 16);

  // Zeroize sensitive information
  memset (m_pBuffer, 0, sizeof(*m_pBuffer));

  m_Finalized=1;
  return 0;

}
/**********************************************************
GetRawDigest
	Returns the Raw Digest.
	Note that if this function succeeded it will allocate 16 Byte
	The caller is responsible for releasing this memory
Return
	unsigned char * - The digest 
**********************************************************/
unsigned char *MD5::GetRawDigest ()
{

  
  if (!m_Finalized){
     //  the Digest is not Finalized the Digest
    return ( (unsigned char*) "");
  }
  
  BYTE *s = new BYTE[16];

  memcpy(s, m_pDigest, 16);
  return s;
}
/**********************************************************

  Returns the finalized digest

**********************************************************/
char *MD5::GetDigestInHex ()
{

  int i;
 
  if (!m_Finalized){
     // disgest is not a m_Finalized m_pDigest
    return "";
  }

 char *s= new char[33];

  for (i=0; i<16; i++)
    sprintf(s+i*2, "%02x", m_pDigest[i]);

  s[32]='\0';

  return s;
}
/*****************************
 MD5 initialization. Begins an MD5 operation, writing a new context.
*****************************/
void MD5::init(){
	m_Finalized=0;  // we just started!

	// Nothing counted, so far
	m_uCount[0] = 0;
	m_uCount[1] = 0;

	// 
	//  A four-word buffer (A,B,C,D) is used to compute the message digest.
	//   Here each of A, B, C, D is a 32-bit register. These registers are
	//   initialized to the following values in hexadecimal, low-order bytes
	//   first):
	// 
	//   word A: 01 23 45 67
	//   word B: 89 ab cd ef
	//   word C: fe dc ba 98
	//   word D: 76 54 32 10
	//
	//
	m_uState[0] = 0x67452301; //word A
	m_uState[1] = 0xefcdab89; //word B
	m_uState[2] = 0x98badcfe; //word C
	m_uState[3] = 0x10325476; // word D
}

/****************************
	MD5 basic transformation. Transforms state based on block.
 
	This step uses a 64-element table T[1 ... 64] constructed from the
	sine function. Let T[i] denote the i-th element of the table, which
	is equal to the integer part of 4294967296 times abs(sin(i)), where i
	is in radians. The elements of the table are given in the appendix
	The data used is generated using the following code




****************************/
void MD5::Transform (BYTE block[64]){

  UINT a = m_uState[0], b = m_uState[1], c = m_uState[2], d = m_uState[3], x[16];

  Decode (x, block, 64);

  assert(!m_Finalized);  // not just a user error, since the method is private

  /* Round 1 */
  FF (a, b, c, d, x[ 0], S11, m_nTable[0]); /* 1  */
  FF (d, a, b, c, x[ 1], S12, m_nTable[1]); /* 2 */
  FF (c, d, a, b, x[ 2], S13, m_nTable[2]); /* 3 */
  FF (b, c, d, a, x[ 3], S14, m_nTable[3]); /* 4 */
  FF (a, b, c, d, x[ 4], S11, m_nTable[4]); /* 5 */
  FF (d, a, b, c, x[ 5], S12, m_nTable[5]); /* 6 */
  FF (c, d, a, b, x[ 6], S13, m_nTable[6]); /* 7 */
  FF (b, c, d, a, x[ 7], S14, m_nTable[7]); /* 8 */
  FF (a, b, c, d, x[ 8], S11, m_nTable[8]); /* 9 */
  FF (d, a, b, c, x[ 9], S12, m_nTable[9]); /* 10 */
  FF (c, d, a, b, x[10], S13, m_nTable[10]); /* 11 */
  FF (b, c, d, a, x[11], S14, m_nTable[11]); /* 12 */
  FF (a, b, c, d, x[12], S11, m_nTable[12]); /* 13 */
  FF (d, a, b, c, x[13], S12, m_nTable[13]); /* 14 */
  FF (c, d, a, b, x[14], S13, m_nTable[14]); /* 15 */
  FF (b, c, d, a, x[15], S14, m_nTable[15]); /* 16 */

 /* Round 2 */
  GG (a, b, c, d, x[ 1], S21, m_nTable[16]); /* 17 */
  GG (d, a, b, c, x[ 6], S22, m_nTable[17]); /* 18 */
  GG (c, d, a, b, x[11], S23, m_nTable[18]); /* 19 */
  GG (b, c, d, a, x[ 0], S24, m_nTable[19]); /* 20 */
  GG (a, b, c, d, x[ 5], S21, m_nTable[20]); /* 21 */
  GG (d, a, b, c, x[10], S22, m_nTable[21]); /* 22 */
  GG (c, d, a, b, x[15], S23, m_nTable[22]); /* 23 */
  GG (b, c, d, a, x[ 4], S24, m_nTable[23]); /* 24 */
  GG (a, b, c, d, x[ 9], S21, m_nTable[24]); /* 25 */
  GG (d, a, b, c, x[14], S22, m_nTable[25]); /* 26 */
  GG (c, d, a, b, x[ 3], S23, m_nTable[26]); /* 27 */
  GG (b, c, d, a, x[ 8], S24, m_nTable[27]); /* 28 */
  GG (a, b, c, d, x[13], S21, m_nTable[28]); /* 29 */
  GG (d, a, b, c, x[ 2], S22, m_nTable[29]); /* 30 */
  GG (c, d, a, b, x[ 7], S23, m_nTable[30]); /* 31 */
  GG (b, c, d, a, x[12], S24, m_nTable[31]); /* 32 */

  /* Round 3 */
  HH (a, b, c, d, x[ 5], S31, m_nTable[32]); /* 33 */
  HH (d, a, b, c, x[ 8], S32, m_nTable[33]); /* 34 */
  HH (c, d, a, b, x[11], S33, m_nTable[34]); /* 35 */
  HH (b, c, d, a, x[14], S34, m_nTable[35]); /* 36 */
  HH (a, b, c, d, x[ 1], S31, m_nTable[36]); /* 37 */
  HH (d, a, b, c, x[ 4], S32, m_nTable[37]); /* 38 */
  HH (c, d, a, b, x[ 7], S33, m_nTable[38]); /* 39 */
  HH (b, c, d, a, x[10], S34, m_nTable[39]); /* 40 */
  HH (a, b, c, d, x[13], S31, m_nTable[40]); /* 41 */
  HH (d, a, b, c, x[ 0], S32, m_nTable[41]); /* 42 */
  HH (c, d, a, b, x[ 3], S33, m_nTable[42]); /* 43 */
  HH (b, c, d, a, x[ 6], S34, m_nTable[43]); /* 44 */
  HH (a, b, c, d, x[ 9], S31, m_nTable[44]); /* 45 */
  HH (d, a, b, c, x[12], S32, m_nTable[45]); /* 46 */
  HH (c, d, a, b, x[15], S33, m_nTable[46]); /* 47 */
  HH (b, c, d, a, x[ 2], S34, m_nTable[47]); /* 48 */

  /* Round 4 */
  II (a, b, c, d, x[ 0], S41, m_nTable[48]); /* 49 */
  II (d, a, b, c, x[ 7], S42, m_nTable[49]); /* 50 */
  II (c, d, a, b, x[14], S43, m_nTable[50]); /* 51 */
  II (b, c, d, a, x[ 5], S44, m_nTable[51]); /* 52 */
  II (a, b, c, d, x[12], S41, m_nTable[52]); /* 53 */
  II (d, a, b, c, x[ 3], S42, m_nTable[53]); /* 54 */
  II (c, d, a, b, x[10], S43, m_nTable[54]); /* 55 */
  II (b, c, d, a, x[ 1], S44, m_nTable[55]); /* 56 */
  II (a, b, c, d, x[ 8], S41, m_nTable[56]); /* 57 */
  II (d, a, b, c, x[15], S42, m_nTable[57]); /* 58 */
  II (c, d, a, b, x[ 6], S43, m_nTable[58]); /* 59 */
  II (b, c, d, a, x[13], S44, m_nTable[59]); /* 60 */
  II (a, b, c, d, x[ 4], S41, m_nTable[60]); /* 61 */
  II (d, a, b, c, x[11], S42, m_nTable[61]); /* 62 */
  II (c, d, a, b, x[ 2], S43, m_nTable[62]); /* 63 */
  II (b, c, d, a, x[ 9], S44, m_nTable[63]); /* =  4294967296 * ABS(Sine (64)  */

  m_uState[0] += a;
  m_uState[1] += b;
  m_uState[2] += c;
  m_uState[3] += d;

  // Zeroize sensitive information.
  memset ( (BYTE *) x, 0, sizeof(x));

}
/**********************************************************
 Encodes In (UINT) into output (unsigned char). Assumes len is
 a multiple of 4.
**********************************************************/
void MD5::Encode (BYTE *output, UINT *In, UINT len) 
{

	unsigned int i, j;

	for (i = 0, j = 0; j < len; i++, j += 4) 
	{
		output[j]   = (BYTE)  (In[i] & 0xff);
		output[j+1] = (BYTE) ((In[i] >> 8) & 0xff);
		output[j+2] = (BYTE) ((In[i] >> 16) & 0xff);
		output[j+3] = (BYTE) ((In[i] >> 24) & 0xff);
	}
}

/**********************************************************
 Decodes input (unsigned char) into output (UINT4). 
 Assumes len is a multiple of 4. 
***********************************************************/
void MD5::Decode (UINT *output, BYTE *In, UINT len)
{

	unsigned int i, j;
	for (i = 0, j = 0; j < len; i++, j += 4)
		output[i] = ((UINT)In[j]) | (((UINT)In[j+1]) << 8) |
		(((UINT)In[j+2]) << 16) | (((UINT)In[j+3]) << 24);
}
/*********************************************************
#define ROTATE_LEFT(x, n) (((x) << (n)) | ((x) >> (32-(n))))
ROTATE_LEFT rotates x left n bits.
**********************************************************/
unsigned int MD5::RotateLeft  (UINT x, UINT n)
{
	return (x << n) | (x >> (32-n))  ;
}

// F, G, H and I are basic MD5 functions.
/*********************************************************
F ()
	F(X,Y,Z) = XY v not(X) Z
	In each bit position F acts as a conditional: if X then Y else Z.
	The function F could have been defined using + instead of v since XY
	and not(X)Z will never have 1's in the same bit position.) It is
	interesting to note that if the bits of X, Y, and Z are independent
	and unbiased, the each bit of F(X,Y,Z) will be independent and
	unbiased. (see RFC 1321)

*********************************************************/
unsigned int MD5::F (UINT x, UINT y, UINT z)
{
  return (x & y) | (~x & z);
}

/********************************************************/
unsigned int MD5::G (UINT x, UINT y, UINT z)
{
	return (x & z) | (y & ~z);
}
/**********************************************************/
unsigned int MD5::H (UINT x, UINT y, UINT z)
{
	return x ^ y ^ z;
}
/**********************************************************/
unsigned int MD5::I (UINT x, UINT y, UINT z)
{
	return y ^ (x | ~z);
}
/**********************************************************
	FF, GG, HH, and II Transformations for rounds 1, 2, 3, and 4.
	Rotation is separate from addition to prevent recomputation.
**********************************************************/
void MD5::FF(UINT& a, UINT b, UINT c, UINT d, UINT x, 
		    UINT  s, UINT ac)
{
	a += F(b, c, d) + x + ac;
	a = RotateLeft (a, s) +b;
}
/**********************************************************/
void MD5::GG(UINT& a, UINT b, UINT c, UINT d, UINT x, 
		    UINT s, UINT ac)
{
	a += G(b, c, d) + x + ac;
	a = RotateLeft (a, s) +b;
}
/**********************************************************/
void MD5::HH(UINT& a, UINT b, UINT c, UINT d, UINT x, 
		    UINT s, UINT ac)
{
	a += H(b, c, d) + x + ac;
	a = RotateLeft (a, s) +b;
}
/**********************************************************/
void MD5::II(UINT& a, UINT b, UINT c, UINT d, UINT x, 
			     UINT s, UINT ac)
{
	a += I(b, c, d) + x + ac;
	a = RotateLeft (a, s) +b;
}

#pragma warning ( pop )