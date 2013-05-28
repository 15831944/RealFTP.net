/*=================================================================
// Ultimate TCP/IP v4.2
// This software along with its related components, documentation and files ("The Libraries")
// is © 1994-2007 The Code Project (1612916 Ontario Limited) and use of The Libraries is
// governed by a software license agreement ("Agreement").  Copies of the Agreement are
// available at The Code Project (www.codeproject.com), as part of the package you downloaded
// to obtain this file, or directly from our office.  For a copy of the license governing
// this software, you may contact us at legalaffairs@codeproject.com, or by calling 416-849-8900.
// 
// =================================================================

//  This is a derived work from RFC 1321
	

//  http://www.csl.sony.co.jp/cgi-bin/hyperrfc?rfc1321.txt
//    Copyright (C) 1990-2, RSA Data Security, Inc. Created 1990. All
//   rights reserved.
//  
//  Based on MD5.H - header file for MD5C.C from RFC 1321
//     MDDRIVER.C - test driver for MD2, MD4 and MD5
//     Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
//  rights reserved.
//  
//  License to copy and use this software is granted provided that it
//  is identified as the "RSA Data Security, Inc. MD5 Message-Digest
//  Algorithm" in all material mentioning or referencing this software
//  or this function.
//  
//  License is also granted to make and use derivative works provided
//  that such works are identified as "derived from the RSA Data
//  Security, Inc. MD5 Message-Digest Algorithm" in all material
//  mentioning or referencing the derived work.
//  
//  RSA Data Security, Inc. makes no representations concerning either
//  the merchantability of this software or the suitability of this
//  software for any particular purpose. It is provided "as is"
//  without express or implied warranty of any kind.
//  
//  These notices must be retained in any copies of any part of this
//  documentation and/or software.
//  
// =================================================================*/

#include <windows.h>
#include <stdio.h>

class MD5 {

public:

  MD5              ();  
  int  Update     (unsigned char *input, unsigned int input_length);
  int   Finalize   ();

  // result m_pDigest can be obtained using the following functions
  unsigned char *    GetRawDigest ();  // m_pDigest as a 16-byte binary array
  char *            GetDigestInHex ();  // m_pDigest as a 33-byte ascii-hex string
 

private:

  int	m_nTable[64];	// Table to hold the magic numbers used in rounds 1-4
  void  PrepareTable();  // fill in the table to hold the magic numbers
 

// the private data:
  UINT m_uState[4];
  UINT m_uCount[2];     /* number of bits, modulo 2^64 (lsb first) */
  BYTE m_pBuffer[64];   // /* input Buffer */

  BYTE m_pDigest[16]; 
  BYTE m_Finalized;

// last, the private methods, mostly static:
  void init             ();               // called by  constructors
  void Transform        (BYTE *m_pBuffer);  

   void Encode    (BYTE *dest, UINT *src, UINT length);
   void Decode    (UINT *dest, BYTE *src, UINT length);

    UINT  RotateLeft (UINT x, UINT n);
    UINT  F           (UINT x, UINT y, UINT z);
    UINT  G           (UINT x, UINT y, UINT z);
    UINT  H           (UINT x, UINT y, UINT z);
    UINT  I           (UINT x, UINT y, UINT z);
    void   FF  (UINT& a, UINT b, UINT c, UINT d, UINT x, 
			    UINT s, UINT ac);
    void   GG  (UINT& a, UINT b, UINT c, UINT d, UINT x, 
			    UINT s, UINT ac);
    void   HH  (UINT& a, UINT b, UINT c, UINT d, UINT x, 
			    UINT s, UINT ac);
    void   II  (UINT& a, UINT b, UINT c, UINT d, UINT x, 
			    UINT s, UINT ac);
};
