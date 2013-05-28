/* =================================================================
//  class: CUT_CramMd5
//  File:  UT_CramMd5.h
//  
//  Purpose:
//
//  Challenge-Response Authentication Mechanism 
//	
// ===================================================================
// Ultimate TCP/IP v4.2
// This software along with its related components, documentation and files ("The Libraries")
// is © 1994-2007 The Code Project (1612916 Ontario Limited) and use of The Libraries is
// governed by a software license agreement ("Agreement").  Copies of the Agreement are
// available at The Code Project (www.codeproject.com), as part of the package you downloaded
// to obtain this file, or directly from our office.  For a copy of the license governing
// this software, you may contact us at legalaffairs@codeproject.com, or by calling 416-849-8900.
// 
// =================================================================*/
#ifndef EA3A1804_7185_11d5_AA0F_0050BAAAE90D
#define EA3A1804_7185_11d5_AA0F_0050BAAAE90D

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "Base64.h"
#include "md5.h"
#include <assert.h>



//  Challenge-Response Authentication Mechanism 
class CUT_CramMd5{

protected :
	// hold the user password
	char *m_szPassword; 

	// holds the user name
	char *m_szUserName; 

	 // internal buffer to hold the result 
	char *m_ChallengeResponse;

public:

	// constructor
	CUT_CramMd5();

	// destructor
	~CUT_CramMd5();

	// HMAC MD5 Function
	void HmacMd5( unsigned char*  , // pointer to data stream 
			  int , //length of data stream 
			  unsigned char* , //pointer to authentication key 
			  int , //length of authentication key 
			  MD5 & // refrence to the context that receives the result digest
			  );

	// set the user name
	void SetUserName(LPCSTR name);
	// set the user password
	void SetPassword(LPCSTR pass);

	// retrive the user name
	const char * GetUserName();
	//retrieve the user password
	const char *GetPassword();

	// get the MD5 Maced response for the challenge specified
	char *GetClientResponse(LPCSTR ServerChallenge);

};
#endif //EA3A1804_7185_11d5_AA0F_0050BAAAE90D
