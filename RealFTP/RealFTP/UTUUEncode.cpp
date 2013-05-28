//=================================================================
//  class: CUT_UUEncode
//  File:  Utuu.cpp
//
//  Purpose:
//  
//  Implemetation of UUEncoding class
//
//=================================================================
// Ultimate TCP/IP v4.2
// This software along with its related components, documentation and files ("The Libraries")
// is © 1994-2007 The Code Project (1612916 Ontario Limited) and use of The Libraries is
// governed by a software license agreement ("Agreement").  Copies of the Agreement are
// available at The Code Project (www.codeproject.com), as part of the package you downloaded
// to obtain this file, or directly from our office.  For a copy of the license governing
// this software, you may contact us at legalaffairs@codeproject.com, or by calling 416-849-8900.
//=================================================================

#include "stdafx.h"

#include <stdio.h>

#include "ut_strop.h"
#include "UTUUEncode.h"

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)


/***************************************************
****************************************************/
CUT_UUEncode::CUT_UUEncode() : 
		m_lMsgBodyTextStart(-1),
		m_lMsgBodyTextEnd(-1),
		m_decodeFileInfo(NULL),
		m_currentAttachment(NULL),
		m_DecodeDataSource(NULL), 
		m_numberAttachments(0),
		m_nActualMsgSize(-1)
{
}
/***************************************************
****************************************************/
CUT_UUEncode::~CUT_UUEncode()
{
    EmptyAttachmentList();

	if(m_DecodeDataSource)
		delete m_DecodeDataSource;
}

/********************************
DecodeBytes
    Decodes 4 characters (possibly less) 
	to 3 characters
PARAM:
	in		- input buffer
	out		- output buffer
	count	- number of characters in the input buffer
RETURN:
	none
*********************************/
void CUT_UUEncode::DecodeBytes(char *in,char *out,int count){

     int c1, c2, c3;

     c1 = (DEC(*in)   << 2) | (DEC(in[1]) >> 4);
     c2 = (DEC(in[1]) << 4) | (DEC(in[2]) >> 2);
     c3 = (DEC(in[2]) << 6) |  DEC(in[3]);
     if (count >= 1)
            out[0] = (char)c1;
     if (count >= 2)
            out[1] = (char)c2;
     if (count >= 3)
            out[2] = (char)c3;
}

/********************************
EncodeBytes
	Encodes 3 characters and returns 4 characters
PARAM:
	in		- input buffer
	out		- output buffer
RETURN:
	none
*********************************/
void CUT_UUEncode::EncodeBytes(char *in,char *out){

    out[0] = (char)(in[0] >> 2);
    out[1] = (char)(((in[0] << 4) & 060) | ((in[1] >> 4) & 017));
    out[2] = (char)(((in[1] << 2) & 074) | ((in[2] >> 6) & 03));
    out[3] = (char)(in[2] & 077);

    out[0] = (char)ENC(out[0]);
    out[1] = (char)ENC(out[1]);
    out[2] = (char)ENC(out[2]);
    out[3] = (char)ENC(out[3]);
}

/********************************
AddDecodeFileInfo
    Used in the process of decoding a file.

    This method accepts an ATTACHMENTLISTITEM
    structure, and appends it to a linked list of
    attachments (m_decodeFileInfo).
PARAM: 
	newitem		- pointer to the new item
RETURN:
	none
*********************************/
void CUT_UUEncode::AddDecodeFileInfo (ATTACHMENTLISTITEM *newitem)
{

    ATTACHMENTLISTITEM *item;

    if (m_decodeFileInfo != NULL) {

        item = m_decodeFileInfo;
        while (item->next != NULL)
            item = item->next;

        item->next = new ATTACHMENTLISTITEM;
        item=item->next;
		} 
	else {
        m_decodeFileInfo = new ATTACHMENTLISTITEM;
        item = m_decodeFileInfo;
		}

    strcpy(item->szFileName, newitem->szFileName);

    item->nOffset			= newitem->nOffset;
    item->nContentLength	= newitem->nContentLength;
    item->nSize	            = newitem->nSize;
    item->next				= NULL;
	item->ptrDataSource		= NULL;

	++ m_numberAttachments;
}

/********************************
GetMessageBodyText
	Gets a meessage body text from the encoded message
PARAM:
    dest		- where to put the message text
	[append]	- should we append it to the data source
RETURN:
    UTE_SUCCESS			- success
	UTE_ERROR			- error
	UTE_DS_OPEN_FAILED	- failed to open data source
	UTE_DS_WRITE_FAILED		- data source writing error
*********************************/
int CUT_UUEncode::GetMessageBodyText(CUT_DataSource & dest, BOOL append) 
{
	int		nBytesRead, error = UTE_SUCCESS;
	long	nTotalBytesLeft = 0L;
	char	buffer[512];

	// Decode data source was not set
    if (m_DecodeDataSource == NULL)
        return UTE_ERROR;						

	// Check if we found message boundarus during the call to SetDecodeSource
	if(m_lMsgBodyTextStart == -1 || m_lMsgBodyTextEnd == -1 || m_lMsgBodyTextEnd < m_lMsgBodyTextStart)
		return UTE_ERROR;

	// Open data source for writing
	if(dest.Open( (append) ? UTM_OM_APPEND : UTM_OM_WRITING) == -1)
		return UTE_DS_OPEN_FAILED;

	// Write message text
	nTotalBytesLeft = m_lMsgBodyTextEnd - m_lMsgBodyTextStart;
	if(m_DecodeDataSource->Seek(m_lMsgBodyTextStart, SEEK_SET) != -1)
		while((nBytesRead = m_DecodeDataSource->Read(buffer, min(nTotalBytesLeft, sizeof(buffer) - 1) )) > 0) {
			if(dest.Write(buffer, nBytesRead) != nBytesRead) {
				error = UTE_DS_WRITE_FAILED;
				break;
				}
			nTotalBytesLeft -= nBytesRead;
			}

	// Error. We don't get the whole message
	if(nTotalBytesLeft != 0)
		error = UTE_ERROR;

	// Close data source
	dest.Close();

	return error;
}

/********************************
GetName
	Gets the name of encoding/decoding
	algorithm 
PARAM:
    none
RETURN:
    name "UUEncode"
*********************************/
LPCSTR	CUT_UUEncode::GetName() const
{
	static LPCSTR lpName = "UUEncode";
	return lpName;
}

/********************************
GetAttachmentNumber
	Used to determine the number of decodable files
	contained in an encoded file.  The encoded file
	is set with a call to SetDecodeSource()
PARAM:
    none
RETURN:
    int     number of attachments
*********************************/
int CUT_UUEncode::GetAttachmentNumber() const
{

    int count = 0;

    ATTACHMENTLISTITEM *item= m_decodeFileInfo;

    while(item != NULL) {
        count++;
        item=item->next;
		}

    return count;
}

/********************************
EmptyAttachmentList
	Deletes all elements in the Attachment List, including
	all elements of any include custom header lists.
PARAM:
    none
RETURN:
	none
*********************************/
void CUT_UUEncode::EmptyAttachmentList() {

    ATTACHMENTLISTITEM *item;
    while (m_decodeFileInfo != NULL){
        item = m_decodeFileInfo->next;

		if(m_decodeFileInfo->ptrDataSource)
			delete m_decodeFileInfo->ptrDataSource;

        if (m_decodeFileInfo->szFileName  != NULL)
            m_decodeFileInfo->szFileName[0] = '\0';

        delete [] m_decodeFileInfo;
        m_decodeFileInfo = item;
    }
    m_decodeFileInfo = NULL;
}

/********************************
AddAttachment
	Used in the process of encoding. Adds a new attachment 
	to the attachment list. 
PARAM:
	source	- datasource to be included
	name	- name of the data source
	[params]- pointer to the list of optional parameters
	bAddToTop - not used
RETURN:
	UTE_SUCCESS			- success
	UTE_DS_OPEN_FAILED	- failed to open attachment data source
*********************************/
int CUT_UUEncode::AddAttachment(	CUT_DataSource & source, 
									LPCSTR name,  
									CUT_StringList*  /* params */,
									BOOL /* bAddToTop */) 
{
	
	// verify that the supplied data source can be opened for reading
	if (source.Open(UTM_OM_READING) == -1)
		return UTE_DS_OPEN_FAILED;

	long	lSize = source.Seek(0, SEEK_END);

	source.Close();

	ATTACHMENTLISTITEM	*item;

	if (m_decodeFileInfo != NULL) {

		item = m_decodeFileInfo;
		while (item->next != NULL)
			item = item->next;

		item->next = new ATTACHMENTLISTITEM;
		item=item->next;
		} 
	else {
		m_decodeFileInfo = new ATTACHMENTLISTITEM;
		item = m_decodeFileInfo;
		}
	
	// Save pointer to the datasource in the list
	item->ptrDataSource = source.clone();

	// Put datasource size
	item->nSize = lSize;

	if ( name != NULL ) 
		strncpy(item->szFileName, name, MAX_PATH);
	else
		item->szFileName[0] = 0;

	item->next = NULL;

	// used in the event that the user wants to 
	// add custom headers to individual attachments
	m_currentAttachment = item;

	m_numberAttachments++;

	return UTE_SUCCESS;
}

/********************************
AddFileHeaderTag
	Add custom headers to an individual attachments.
	Affect only the last added attachment. Can be 
	called several times to form a list of headers
PARAM:
    tag			- custom header  to add
RETURN:
	UTE_SUCCESS	- success
*********************************/
int	CUT_UUEncode::AddFileHeaderTag(LPCSTR /*  tag */  )
{
	return UTE_SUCCESS;
}

/********************************
GetGlobalHeadersNumber
	Get the number of global headers that need to be added 
	to the message global headers
PARAM:
    none
RETURN:
	number of headers
*********************************/
int CUT_UUEncode::GetGlobalHeadersNumber() const
{
	return 0;
}

/********************************
GetGlobalHeader
	Get global header by index
PARAM:
    index - index of the header
RETURN:
	pointer to the header
	NULL - error
*********************************/
LPCSTR CUT_UUEncode::GetGlobalHeader(int /* index */ ) const
{
	return NULL;
}

/********************************
Encode
	Encode list of attachments into the data source
PARAM:
    dest	- destination data source
	[append]	- should we append it to the data source
RETURN:
	UTE_SUCCESS				- success
	UTE_MSG_NO_ATTACHMENTS	- no attachments
	UTE_DS_OPEN_FAILED		- failed to open data source
	UTE_DS_WRITE_FAILED		- data source writing error
*********************************/
int CUT_UUEncode::Encode(CUT_DataSource & msg_body, CUT_DataSource & dest, BOOL append) {

	ATTACHMENTLISTITEM	*item = m_decodeFileInfo;
	int					error = UTE_SUCCESS;

	// Open the datasource for writing
	if(dest.Open( (append) ? UTM_OM_APPEND : UTM_OM_WRITING) == -1)
		return UTE_DS_OPEN_FAILED;				// problem opening data source

	// Open message body datasource for reading
	if ( msg_body.Open(UTM_OM_READING) == -1 ) {
		dest.Close();
		return UTE_DS_OPEN_FAILED;				// problem opening data source
		}

	// Write down the message body
	char	buffer[512];
	int		nCharsRead;

		// If the data source for the attachment is not set it means
        // that we are trying to encode already encoded attachment.
		// so don't add the message body
        if( m_DecodeDataSource == NULL )
			while((nCharsRead=msg_body.Read(buffer, sizeof(buffer) - 1)) > 0) {
				if(dest.Write(buffer, nCharsRead) != nCharsRead)
					error = UTE_DS_WRITE_FAILED;
			}

	// No attachments - return
	if ( item == NULL ) {
		error = UTE_SUCCESS;
		if (dest.Close() == -1)
			error = UTE_DS_CLOSE_FAILED;		// problem closing file
		if (msg_body.Close() == -1)
			error = UTE_DS_CLOSE_FAILED;		// problem closing file
		return error;
		}

	// Write empty line
	if( m_DecodeDataSource == NULL )
		if(dest.Write("\r\n\r\n", 4) != 4) 
		error = UTE_DS_WRITE_FAILED;

	// Loop through all attacments list
	while (error == UTE_SUCCESS && item != NULL) {

		// If the data source for the attachment is not set it means
        // that we are trying to encode already encoded attachment.
        if(item->ptrDataSource == NULL && m_DecodeDataSource != NULL &&
           item->nContentLength  > 0 && item->nOffset  > 0 ) {

            if(m_DecodeDataSource->Open(UTM_OM_READING) != -1) {

                // Find the begining of the attachment header
                long    nCurOffset = max(item->nOffset - 1000 - 4, 0);
                long    lReturn, lLastFoundPos = 0;
                for(int i = 1; lLastFoundPos == 0 && i <= 10; i++) {
                    nCurOffset = max(item->nOffset - 1000*i - 4, 0);
                    while((lReturn = m_DecodeDataSource->Find("\r\n\r\n", nCurOffset, FALSE, 1000)) > 0) {
                        if(lReturn >= item->nOffset - 4)
                            break;
                        lLastFoundPos = lReturn;
                        nCurOffset = lReturn + 4;
                        }
                    }

                if(lLastFoundPos == 0)
                    lLastFoundPos = item->nOffset;
               
				
				if(m_DecodeDataSource->Seek(lLastFoundPos, SEEK_SET) == lLastFoundPos) {

                    // In this case just copy the encoded attachment to the destination
                    char    szBuffer[MAX_LINE_LENGTH + 1];
                 //   char    *lpszPtrEmptyLine = NULL;
                    int     nBytesRead ,nBytesToRead = item->nContentLength + (item->nOffset-lLastFoundPos);

                    // Copy encoded data to the destination
                    while((nBytesRead=m_DecodeDataSource->Read(szBuffer, MAX_LINE_LENGTH)) > 0) {
                        if(nBytesRead > nBytesToRead) {
                            dest.Write(szBuffer, nBytesToRead);
                            break;
                            }   
                        else {
                            dest.Write(szBuffer, nBytesRead);
                            }

                        nBytesToRead -= nBytesRead;
                        }
                    }

            	// Close data sources
	            m_DecodeDataSource->Close();

                // Go to the next item
                item=item->next;
                continue;
                }
            }


		char	buf[MAX_LINE_LENGTH];
		int		filenamepos, bufferlen;

		// Open the attacment's datasource
		if(item->ptrDataSource == NULL || item->ptrDataSource->Open(UTM_OM_READING) == -1) {
			error = UTE_DS_OPEN_FAILED;
			break;
			}

		// Get the filename without the path
		filenamepos = 0;
		for( int t = (int)strlen(item->szFileName); t > 0; t-- ) {
			if( item->szFileName[t]=='\\') {
				filenamepos = t+1;
				break;
				}
			}

		//make the header
		_snprintf(buf,sizeof(buf)-1, "begin %d %s\r\n", 660, &item->szFileName[filenamepos]);
		bufferlen = (int)strlen(buf);
		if(dest.Write(buf, bufferlen) != bufferlen) {
			error = UTE_DS_WRITE_FAILED;
			break;
			}

		//encode the source file
		char	out[4];
		int		n = item->ptrDataSource->Read(buf, 45); 

		while(n > 0) {

			char	c = (char) ENC(n);			
			if(dest.Write(&c, 1) != 1) {
				error = UTE_DS_WRITE_FAILED;
				break;
				}

			//encode the line
			for (int t=0; t < n; t += 3) {
				EncodeBytes( &buf[t], out );
				if(dest.Write(out, 4) != 4) {
					error = UTE_DS_WRITE_FAILED;
					break;
					}
				}

			if(dest.Write("\r\n", 2) != 2) {
				error = UTE_DS_WRITE_FAILED;
				break;
				}

			n = item->ptrDataSource->Read(buf, 45); 
			}

		//write the footer
		sprintf(buf,"end\r\n");
		bufferlen = (int)strlen(buf);
		if(dest.Write(buf, bufferlen) != bufferlen) {
			error = UTE_DS_WRITE_FAILED;
			break;
			}
	

		// Close attachment data source
		item->ptrDataSource->Close();

		// Go to the next attacment
		item = item->next;
		}

	if (dest.Close() == -1)
		error = UTE_DS_CLOSE_FAILED;		// problem closing file
	if (msg_body.Close() == -1)
		error = UTE_DS_CLOSE_FAILED;		// problem closing file

	return UTE_SUCCESS;
}


/********************************
SetDecodeSource
     Sets a data source for decoding and
	 searchs for attachments
PARAM:
    none
RETURN:
	UTE_SUCCESS				- success. We can decode this file
	UTE_ERROR				- failed. Not a UUEncode file
	UTE_DS_OPEN_FAILED		- failed to open data source
*********************************/
int CUT_UUEncode::SetDecodeSource(CUT_DataSource & source) {

    char				buf[MAX_PATH+1];
    ATTACHMENTLISTITEM	item;
    int					numberOfAttachments = 0;
    char				attachFileName[MAX_PATH+1];

	// Initialize message body text position
	m_lMsgBodyTextStart = m_lMsgBodyTextEnd = -1;

	// Sets the data source
	if(m_DecodeDataSource)	delete m_DecodeDataSource;
	m_DecodeDataSource = source.clone();

	// Open data source
	if(m_DecodeDataSource->Open(UTM_OM_READING) == -1)
		return UTE_DS_OPEN_FAILED;

    // Ensure this is a UUEncoded Encoded File
    if (!CanDecode()) {
		delete m_DecodeDataSource;
		m_DecodeDataSource = NULL;
		return UTE_ERROR;				// Not a UUEncode file
		}

    int		beginPos;
    int		endPos;
	BOOL	bSizeSet = FALSE;

    // find end of message header
    beginPos			= m_DecodeDataSource->Find("\r\n\r\n");
	m_nActualMsgSize	= beginPos + 4;
	m_lMsgBodyTextStart = m_nActualMsgSize;

    do {

            beginPos = m_DecodeDataSource->Find( "begin", beginPos );  // case insensitive search by default
			if(m_lMsgBodyTextEnd == -1)
				m_lMsgBodyTextEnd = beginPos;

			// Set actual message size
			if(!bSizeSet) {
				if(beginPos < 0)
					m_nActualMsgSize = -1;
				else 
					m_nActualMsgSize = beginPos - m_nActualMsgSize;
				bSizeSet = TRUE;
				}
            
            if ( beginPos < 0 )       // we're at the end of the file
                break;

			m_DecodeDataSource->Seek(beginPos, SEEK_SET);

			m_DecodeDataSource->ReadLine(buf, MAX_PATH);

            sscanf(buf, "begin %o %[^\n\r]", &item.nFileAccessMode, attachFileName);
            strcpy(item.szFileName, attachFileName);

            item.nOffset = beginPos;

            endPos = m_DecodeDataSource->Find( "end\r\n", beginPos );  // case insensitive search by default
            if (endPos <=0 )
                return UTE_ERROR;  // Bad file a begin without an end

            item.nContentLength =  ((endPos) - beginPos) + 7;  // include 'end' in content
            item.next = NULL;
			item.nSize = 0;

            AddDecodeFileInfo( &item );

            numberOfAttachments++;

            beginPos = beginPos + 12 ;
        }while (beginPos >= 0 );

    return UTE_SUCCESS;
}
	
/********************************
CanDecode
     Check if decoding is possible for t
	 he specified decode source
PARAM:
    none
RETURN:
	TRUE		- we can decode it
	FALSE		- we can't decode it
*********************************/
BOOL CUT_UUEncode::CanDecode() {

    char    buf[100];
    BOOL    returnCode = TRUE;
    char    attachFileName[MAX_PATH+1];
    int     fileAccessRights = 0;

    if (m_DecodeDataSource == NULL)
        return FALSE;						// decode data source was not set

    // seek the beginning of the file to be decoded
	m_DecodeDataSource->Seek(0, SEEK_SET);

     // search for header line
   do {

        if ( m_DecodeDataSource->ReadLine(buf, sizeof(buf)) == 0 ) {
            returnCode = FALSE;
            break;
	        }
      }while (strncmp(buf, "begin ", 6) != 0);


    sscanf(buf, "begin %d %s", &fileAccessRights, attachFileName);

    // ensure that fileAccessRights are 6xx series.  See RFC for details.
    if (fileAccessRights/100 == 6)
        return TRUE;
    else
        return FALSE;
}

/********************************
GetAttachmentInfo
    Gets attachment info
PARAM:
    index		- index of the attachment
    name 		- name of the attachment
    params		- list of additional parameters
	maxsize		- maxsize of buffers
RETURN:
	UTE_SUCCESS	- success
	UTE_ERROR	- fail
*********************************/
int CUT_UUEncode::GetAttachmentInfo(	int index, 
										LPSTR name, 
										int namesize,
										LPSTR type,
										int /*  typesize  */ ,
										CUT_StringList*  /*  params  */ ) const
{
	if(name == NULL)	return UTE_ERROR;

    int					count = 0;
    ATTACHMENTLISTITEM	*item = m_decodeFileInfo;

    while( item != NULL ) {
		if(count == index) {
			if(name != NULL)
				strncpy(name, item->szFileName, namesize);
			if(type != NULL)
				strcpy(type, "");
			return UTE_SUCCESS;
			}
        count ++;
        item = item->next;
		}

	return UTE_ERROR;
}


/********************************
Decode
    Decode attachment by index into destination 
	data source
PARAM:
    index		- index of the attachment
	dest		- destination data source
	[append]	- should we append it to the data source
RETURN:
	UTE_SUCCESS				- success
	UTE_DS_OPEN_FAILED		- failed to open data source
	UTE_DS_WRITE_FAILED		- data source writing error
	UTE_INDEX_OUTOFRANGE	- index is out of range
	UTE_FILE_FORMAT_ERROR	- file format error

*********************************/
int CUT_UUEncode::Decode(int index, CUT_DataSource & dest, BOOL append) {

    int		returnCode = UTE_SUCCESS;
    int		count = 0;
    int		fileAccessRights;

    char	attachFileName[MAX_PATH+1];
    char	*bp;
    char	bufOut[5];
    char	buf[100];

	// Decode data source not set
    if(m_DecodeDataSource == NULL)
		return UTE_DS_OPEN_FAILED;

    ATTACHMENTLISTITEM *item = m_decodeFileInfo;

    while( (item != NULL) && (count != index) ) {
        count++;
        item = item->next;
		}

    // index out of range
    if (item == NULL)
        return UTE_INDEX_OUTOFRANGE;          

	// Open destination data source
    if(dest.Open( (append) ? UTM_OM_APPEND : UTM_OM_WRITING) == -1)
        return UTE_DS_OPEN_FAILED;

    if(m_DecodeDataSource->Open(UTM_OM_READING) == -1)	// data source open failed
        return UTE_DS_OPEN_FAILED;

    // seek the beginning of the file to be decoded
    m_DecodeDataSource->Seek(item->nOffset, SEEK_SET);

    // search for header line
    do {
		if ( m_DecodeDataSource->ReadLine(buf, sizeof(buf)) == 0) {
			returnCode = 7;
            break;
			}

	}while (strncmp(buf, "begin ", 6) != 0);

    sscanf(buf, "begin %o %[^\n\r]", &fileAccessRights, attachFileName);
    
	 // get one line to decode
    if ( m_DecodeDataSource->ReadLine(buf, sizeof(buf)) == 0) 
        returnCode = UTE_FILE_FORMAT_ERROR;

    //main decode loop
    while( returnCode == 0 ) {

        count = DEC(buf[0]);

        //if done then break;
        if (count <= 0)
			break;

        bp = &buf[1];

        //decode the line
        while ( count > 0 && returnCode == 0) {
            DecodeBytes( bp, bufOut, count );

            if(count >= 3) {
				if(dest.Write(bufOut, 3) != 3)
					returnCode = UTE_DS_WRITE_FAILED;
				}
            else {
				if(dest.Write(bufOut, count) != count)
					returnCode = UTE_DS_WRITE_FAILED;
				}

            bp += 4;
            count -= 3;
			}

        //check for end marker
        if ( m_DecodeDataSource->ReadLine(buf, sizeof(buf)) == 0) {
            returnCode = UTE_FILE_FORMAT_ERROR;
            break;
			}

        if ( !strcmp(buf, "end") || !strcmp(buf, "end\r\n"))
            break;
		}

	// Close data source
    dest.Close();

    return returnCode;
}
/********************************
GetAttachmentSize
    Gets the size of all or specified attachment
    This function will return the size of the attachments 
    which were added by AddAttachment(). If the attachments 
    are encoded this function will return the encoding content
    lenght. In this case the attachment size is 3/4 of returned 
    value.

PARAM:
    index		- index of the attachment
RETURN:
	length of attachment
*********************************/
long CUT_UUEncode::GetAttachmentSize(int index) const
{
    int					count	= 0;
	long				size	= 0;
    ATTACHMENTLISTITEM	*item = m_decodeFileInfo;

    while( item != NULL ) {
		if(count == index) 
            return (item->nSize == 0 && item->ptrDataSource == NULL) ? (long)(item->nContentLength*0.75) : item->nSize;
			
		size += (item->nSize == 0 && item->ptrDataSource == NULL) ? (long)(item->nContentLength*0.75) : item->nSize;
        count ++;
        item = item->next;
		}

	return size;
}

#pragma warning ( pop )