//=================================================================
//  class: CUT_Msg
//  File:  UTMessage.cpp
//
//  Purpose:
//
//  CUT_Msg class 
//
//	Encapsulates RFC 822 message format
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
#include "utcharset.h"

#include <winnls.h>

#include "UTMessage.h"
#include "UTEncode.h"
#include "UTUUEncode.h"
#include "UTMime.h"

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)


// =================================================================
//	CUT_Msg class
//
//	Helper class which encapsulates RFC 822 message format
// =================================================================

/****************************************
*****************************************/
CUT_Msg::CUT_Msg() : 
m_nMaxMessageBodySize(131072),		// Set Default maximum message body size 20K
m_listUT_HeaderFields(NULL),		// Initialize global headers list
m_lpszMessageBody(NULL),			// Initialize pointer to the message body text
m_lpszHtmlMessageBody(NULL),
m_nEncodingObjNumber(2),			// Number of object for encoding. Def = 2 (UUEncode and MIME)
m_nDefEncodingObj(1),				// Index of default encoding object. 0-UUEncode, 1-MIME
m_nDecodingObj(1),					// Index of current decoding object
m_bDecoding(FALSE),					// Clear decoding mode flag
m_bHtmlBody(FALSE)					// the the body of the message is sent as plain text
{
	// Clear encoding objects array
	memset(m_ptrEncoding, 0, sizeof(m_ptrEncoding[0])* MAX_ENCODING_OBJECTS);

	// First element always UUEncode
	m_ptrEncoding[0] = new CUT_UUEncode;
	m_ptrEncoding[0]->m_pMsg = this;

	// Second element always Mime
	m_ptrEncoding[1] = new CUT_MimeEncode;
	m_ptrEncoding[1]->m_pMsg = this;

	// Allocate message body buffer
	m_lpszMessageBody = new char[m_nMaxMessageBodySize];
	m_lpszMessageBody[0] = NULL;


	m_lpszEmptyString[0] = 0;

	
	// v4.2 default charset

	// get default charset based on locale settings
	LCID lcid = GetUserDefaultLCID();
	strcpy(m_lpszDefaultCharSet, CUT_CharSet::GetCharSetStringFromLocaleID(NULL, lcid));

	m_pCharSet = CUT_CharSet::GetCharSetStruct(m_lpszDefaultCharSet);	

	// init html and text (MIME) charsets 
	*m_lpszTextCharSet = '\0';
	*m_lpszHtmlCharSet = '\0';
	*m_lpszHeaderCharSet = '\0';

}

/****************************************
*****************************************/
CUT_Msg::~CUT_Msg()
{
	ClearHeader();						// Clear message field data

	if(m_lpszMessageBody)
		delete [] m_lpszMessageBody;	// Delete message body
	if(m_lpszHtmlMessageBody)
		delete [] m_lpszHtmlMessageBody;// Delete the HTML message body

	// Delete standart Encoding
	delete m_ptrEncoding[0];
	delete m_ptrEncoding[1];
}


/***********************************************
SaveMessage
Saves message into the DataSource
PARAM:
data	- datasource to save the message
RETURN:
UTE_SUCCESS					- success
UTE_MSG_OPEN_FAILED			- failed to open data source
UTE_MSG_WRITE_LINE_FAILED	- writing line failed
************************************************/
int	CUT_Msg::SaveMessage(CUT_DataSource &data)
{
	int		i, error = UTE_SUCCESS, nMaxLineSize = 100;
	LPSTR	ptrLine = new char[nMaxLineSize + 1];
	LPSTR	ptrPrevCustomName = new char[MAX_FIELD_NAME_LENGTH + 1];

	// If there is an HTML body add it as an attachment for now and remove it after
	// the message is sent
	bool bMustRemoveHTMLAttachment = false;
	if (m_lpszHtmlMessageBody != NULL)
	{
		SetHtmlMessageBody(m_lpszHtmlMessageBody);
		bMustRemoveHTMLAttachment = true;
	}

	// Pass through the encoding one time so the mime headers are generated properly
	{
		// Encode all the attachments & message body and write them to the datasource
		if(m_ptrEncoding[m_nDefEncodingObj] != NULL) 
			if(m_ptrEncoding[m_nDefEncodingObj]->GetAttachmentNumber() > 0)
			{
				char	empty_msg[5] = "";
				LPSTR	msg_buffer;

				if (m_lpszMessageBody != NULL)
					msg_buffer = m_lpszMessageBody;
				else
					msg_buffer = &empty_msg[0];

				CUT_BufferDataSource dsTemp(msg_buffer, strlen(msg_buffer)) ; 
				error = m_ptrEncoding[m_nDefEncodingObj]->Encode(dsTemp , data, TRUE);
			}
	}

	// Open message data source for writing
	if(data.Open(UTM_OM_WRITING) != -1) {
		int		nCurField;

		// Remove fields that will be duplicated by the encoding headers
		if(m_ptrEncoding[m_nDefEncodingObj] != NULL) 
			if(m_ptrEncoding[m_nDefEncodingObj]->GetAttachmentNumber() > 0) {
				// Loop through all encoding attachments headers
				for(i = 0; i < m_ptrEncoding[m_nDefEncodingObj]->GetGlobalHeadersNumber(); i++) {
					// Get the header
					LPCSTR  lpszHeader = m_ptrEncoding[m_nDefEncodingObj]->GetGlobalHeader(i);
					if(lpszHeader != NULL && *lpszHeader != ' ' && *lpszHeader != '\t') {
						// Get the header name
						if(CUT_StrMethods::ParseString(lpszHeader, ":", 0, ptrPrevCustomName, MAX_FIELD_NAME_LENGTH) == UTE_SUCCESS) {
							// Clear headers
							strcat(ptrPrevCustomName, ":");
							ClearHeader(UTM_CUSTOM_FIELD, ptrPrevCustomName);
						}
					}
				}
			}

			ptrPrevCustomName[0] = 0;

			// Write all the fields
			for( nCurField = UTM_MESSAGE_ID; nCurField <= UTM_CUSTOM_FIELD && error == UTE_SUCCESS; nCurField++ ) {
				LPCSTR	ptrField, ptrFieldName = NULL;
				int		nFieldIndex = 0;
				BOOL	bFirstFieldLine = TRUE;

				// Get known field name
				if(nCurField != UTM_CUSTOM_FIELD) 
					ptrFieldName = GetFieldName((HeaderFieldID)nCurField);

				// Field can have several lines. Write down all of them 
				// and put the name of the field on the very first line
				bFirstFieldLine = TRUE;
				while( error == UTE_SUCCESS && (ptrField = GetHeaderByType((HeaderFieldID)nCurField, nFieldIndex)) != NULL) {

					// Get custom field name
					if(nCurField == UTM_CUSTOM_FIELD) {
						ptrFieldName = GetCustomFieldName(nFieldIndex);
						if(_stricmp(ptrPrevCustomName, ptrFieldName) != 0)
							bFirstFieldLine = TRUE;
						strncpy(ptrPrevCustomName, ptrFieldName, MAX_FIELD_NAME_LENGTH);
					}


					// If buffer too small - realloc
					int		nLineLength = (int)strlen(ptrField) + (int)strlen(ptrFieldName) + 1;
					if(nLineLength >= nMaxLineSize) {
						delete [] ptrLine;
						nMaxLineSize = nLineLength + 10;
						ptrLine = new char[nMaxLineSize];
					}

					ptrLine[0] = 0;

					// Add field name to the first line for every known field
					if(bFirstFieldLine && ptrFieldName != NULL) {
						strcpy(ptrLine, ptrFieldName);
					}

					// Add a space if needed
					if(ptrField[0] != ' ' && ptrField[0] != '\t')
						if (ptrLine != NULL)
						{
							int iLength = (int)::strlen(ptrLine);
							if (iLength > 0 && ptrLine[iLength - 1] != ' ')
								strcat(ptrLine, " ");
						}

						// Add field data
						strcat(ptrLine, ptrField);

						// Write field data
						if(data.WriteLine(ptrLine) == -1) 
							error = UTE_MSG_WRITE_LINE_FAILED;

						bFirstFieldLine = FALSE;

						++nFieldIndex;
				}
			}

			// Write encoding headers
			if(m_ptrEncoding[m_nDefEncodingObj] != NULL) 
				if(m_ptrEncoding[m_nDefEncodingObj]->GetAttachmentNumber() > 0) {
					for(i = 0; i < m_ptrEncoding[m_nDefEncodingObj]->GetGlobalHeadersNumber(); i++) {
						// Write field data
						if(data.WriteLine(m_ptrEncoding[m_nDefEncodingObj]->GetGlobalHeader(i)) == -1) 
							error = UTE_MSG_WRITE_LINE_FAILED;
					}
				}

				// Write empty line
				if(data.WriteLine("") == -1) 
					error = UTE_MSG_WRITE_LINE_FAILED;

				// Write message body
				if(m_ptrEncoding[m_nDefEncodingObj] == NULL || m_ptrEncoding[m_nDefEncodingObj]->GetAttachmentNumber() <= 0) 
					if(m_lpszMessageBody != NULL)
						if(data.WriteLine(m_lpszMessageBody) == -1) 
							error = UTE_MSG_WRITE_LINE_FAILED;

				// Close message
				if(data.Close() == -1)
					error = UTE_MSG_CLOSE_FAILED;

				// If there is an HTML body add it as an attachment for now and remove it after
				// the message is sent
				bool bMustRemoveHTMLAttachment = false;
				if (m_lpszHtmlMessageBody != NULL)
				{
					SetHtmlMessageBody(m_lpszHtmlMessageBody);
					bMustRemoveHTMLAttachment = true;
				}

				// Encode all the attachments & message body and write them to the datasource
				if(m_ptrEncoding[m_nDefEncodingObj] != NULL) 
					if(m_ptrEncoding[m_nDefEncodingObj]->GetAttachmentNumber() > 0)
					{
						char	empty_msg[5] = "";
						LPSTR	msg_buffer;

						if (m_lpszMessageBody != NULL)
							msg_buffer = m_lpszMessageBody;
						else
							msg_buffer = &empty_msg[0];

						CUT_BufferDataSource dsTemp(msg_buffer, strlen(msg_buffer)) ; 
						error = m_ptrEncoding[m_nDefEncodingObj]->Encode(dsTemp , data, TRUE);
					}
	}
	else
		error = UTE_MSG_OPEN_FAILED;

	if (bMustRemoveHTMLAttachment)
		SetHtmlMessageBody((LPTSTR)NULL);

	// Delete line buffer
	delete [] ptrLine;

	// Delete Prev. Custom field name buffer
	delete [] ptrPrevCustomName;

	return error;
}

/***********************************************
LoadMessage
Loads message from the DataSource
PARAM:
data	- datasource to save the message
RETURN:
UTE_SUCCESS					- success
UTE_MSG_OPEN_FAILED			- failed to open data source
UTE_MSG_READ_LINE_FAILED	- reading line failed
UTE_OUT_OF_MEMORY			- out of memory
UTE_BUFFER_TOO_SHORT		- message body buffer too short
************************************************/
int	CUT_Msg::LoadMessage(CUT_DataSource &data)
{
	int		error = UTE_SUCCESS;
	LPSTR	ptrLine = new char[MAX_LINE_LENGTH];	// Line buffer

	// Empty attachments list
	EmptyAttachmentList();

	// Clear all headers
	ClearHeader();

	// Open message data source for reading
	if(data.Open(UTM_OM_READING) != -1) {
		int		nPrevFieldID = UTM_CUSTOM_FIELD;
		LPSTR	ptrPrevCustomName = new char[MAX_FIELD_NAME_LENGTH];

		// Read header fields
		while(data.ReadLine(ptrLine, MAX_LINE_LENGTH) != 0)  {

			// Remove CrLf
			CUT_StrMethods::RemoveCRLF(ptrLine);

			// Empty line found. The rest is the message body.
			if(strlen(ptrLine) == 0)
				break;

			// That's the next line of the previous field
			if(ptrLine[0] == ' ' || ptrLine[0] == '\t') {
				if(nPrevFieldID == UTM_CUSTOM_FIELD) 
					AddHeaderField(ptrLine, UTM_CUSTOM_FIELD, ptrPrevCustomName);
				else 
					AddHeaderField(ptrLine, (HeaderFieldID)nPrevFieldID);
			}

			// A new field
			else 
			{
				int		id;	
				LPSTR	ptrFieldData;

				// Is it a known field?
				for(id = UTM_MESSAGE_ID; id < UTM_CUSTOM_FIELD; id++ ) {
					if(_strnicmp(ptrLine, GetFieldName((HeaderFieldID)id), strlen(GetFieldName((HeaderFieldID)id))) == 0) 
						break;
				}
				// Save previous field id
				nPrevFieldID = id;


				// Find pointer to the field data
				if((ptrFieldData = strchr(ptrLine, ':')) == NULL)
					ptrFieldData = ptrLine;
				else
					ptrFieldData += 1;

				// According to RFC 2822 we can have "Subject: Hi there" or "Subject:Hi there"
				// So if the data starts with a space - remove it
				if (ptrFieldData[0] == ' ')
					ptrFieldData += 1;

				// If it's a custom field - save the name of the field
				if(id == UTM_CUSTOM_FIELD) {
					strncpy(ptrPrevCustomName, ptrLine, MAX_FIELD_NAME_LENGTH);
					LPSTR ptrFieldEnd = strchr(ptrPrevCustomName, ':');
					if(ptrFieldEnd) 
						ptrFieldEnd[1] = 0;
					else 
						ptrPrevCustomName[0] = 0;

				}

				// Add field
				if(id == UTM_CUSTOM_FIELD) 
					AddHeaderField(ptrFieldData, UTM_CUSTOM_FIELD, ptrPrevCustomName);
				else 
					AddHeaderField(ptrFieldData, (HeaderFieldID)id);
			}
		}


		// Allocate buffer for message body
		unsigned int		nMessageBodySize = 0;
		if (m_lpszMessageBody)
			delete [] m_lpszMessageBody;

		if (m_lpszHtmlMessageBody)
			delete [] m_lpszHtmlMessageBody;

		try
		{
			m_lpszMessageBody = new char[m_nMaxMessageBodySize];
			m_lpszMessageBody[0] = 0;
			m_lpszHtmlMessageBody = new char[m_nMaxMessageBodySize];
			m_lpszHtmlMessageBody[0] = 0;
		}
		catch(...)
		{
			error = UTE_OUT_OF_MEMORY;
		}	


		// Remember message start position
		long	lMessageStart	= data.Seek(0, SEEK_CUR);
		BOOL	bMessageRead	= FALSE;

		m_bHtmlBody = FALSE;

		// Try to decode message body
		int		i;
		for(i = 0; i < m_nEncodingObjNumber; i ++)
		{
			if(m_ptrEncoding[i]->SetDecodeSource(data) == UTE_SUCCESS)
			{
				m_nDecodingObj	= i;
				m_bDecoding		= TRUE;

				CUT_BufferDataSource	ds(m_lpszMessageBody, m_nMaxMessageBodySize - 1);
				if(m_ptrEncoding[m_nDecodingObj]->GetMessageBodyText(ds) == UTE_SUCCESS)
				{
					bMessageRead = TRUE;
					m_bHtmlBody = m_ptrEncoding[m_nDecodingObj]->m_bHTMLBody;

					// TD v4.2 grab charset
					if(*m_ptrEncoding[m_nDecodingObj]->m_lpszCharSet != 0) {
						if(m_bHtmlBody) {
							strcpy(m_lpszHtmlCharSet, m_ptrEncoding[m_nDecodingObj]->m_lpszCharSet);
						}
						else {
							strcpy(m_lpszTextCharSet, m_ptrEncoding[m_nDecodingObj]->m_lpszCharSet);
						}
					}
				}
				else {
					m_lpszMessageBody[0] = 0;
				}
				// v4.2 changes here - affects msgs where both text and messages are set decoded
				// Get the html body if any
				//				CUT_BufferDataSource dsHtml(m_lpszHtmlMessageBody, m_nMaxMessageBodySize - 1);
				//				m_ptrEncoding[m_nDecodingObj]->GetMessageBodyText(dsHtml);

				CUT_BufferDataSource dsHtml(m_lpszHtmlMessageBody, m_nMaxMessageBodySize - 1);
				if(m_ptrEncoding[m_nDecodingObj]->GetMessageBodyText(dsHtml) == UTE_SUCCESS) {
					bMessageRead = TRUE;
					m_bHtmlBody = m_ptrEncoding[m_nDecodingObj]->m_bHTMLBody;

					// TD v4.2 grab charset
					if(*m_ptrEncoding[m_nDecodingObj]->m_lpszCharSet != 0) {
						if(m_bHtmlBody) {
							strcpy(m_lpszHtmlCharSet, m_ptrEncoding[m_nDecodingObj]->m_lpszCharSet);
						}
						else {
							strcpy(m_lpszTextCharSet, m_ptrEncoding[m_nDecodingObj]->m_lpszCharSet);
						}
					}
				}
				else {
					m_lpszHtmlMessageBody[0] = 0;
				}

				break;
			}
		}

		// Close message
		if(data.Close() == -1)
			error = UTE_MSG_CLOSE_FAILED;

		// If message body was not read during decoding - do also in the case that both message body and message html are blank
		if(!bMessageRead || (*m_lpszMessageBody == 0 && *m_lpszHtmlMessageBody == 0)) {
			// Open message data source for reading
			if(data.Open(UTM_OM_READING) != -1) {
				data.Seek(lMessageStart, SEEK_SET);			

				// Read message body
				while(error == UTE_SUCCESS && data.ReadLine(ptrLine, MAX_LINE_LENGTH) != 0)  {

					// Test for buffer overflow
					nMessageBodySize += (int)strlen(ptrLine);
					if((nMessageBodySize + 3) >= m_nMaxMessageBodySize) {
						error = UTE_BUFFER_TOO_SHORT;
						break;
					}

					// Add line to the body
					strcat(m_lpszMessageBody, ptrLine);
					if(!CUT_StrMethods::IsWithCRLF(ptrLine)) {
						strcat(m_lpszMessageBody, "\r\n");
						nMessageBodySize += 2;
					}

				}

				// Close message
				if(data.Close() == -1){
					error = UTE_MSG_CLOSE_FAILED;
				}

				// find out if this should be copied into the html message body.
				char buf[MAX_PATH];
				*buf = 0;
				GetHeaderByType(UTM_CONTENT_TYPE, buf, MAX_PATH-1);
				if(*buf != 0) {
					if(strstr(buf, "html") != NULL) {
						strcpy(m_lpszHtmlMessageBody, m_lpszMessageBody);
						m_bHtmlBody = TRUE;
					}
				}
			}
			else {
				error = UTE_MSG_OPEN_FAILED;
			}
		}


		// Delete previous custom field name buffer
		delete [] ptrPrevCustomName;
	}
	else
		error = UTE_MSG_OPEN_FAILED;

	// Delete line buffer
	delete [] ptrLine;

	// v4.2 try to determine charset if any - could make this a sub routine
	if(error == UTE_SUCCESS) {
		int count = GetHeaderCount(UTM_CONTENT_TYPE);
		if(count != 0) {
			for(int i = 0; i < count; ++i) {
				LPCSTR buf;
				buf = GetHeaderByType(UTM_CONTENT_TYPE, i);
				if(strstr(buf,"charset=") != NULL) {
					char lpszCharset[MAX_PATH];
					*lpszCharset = '\0';
					CUT_StrMethods::ParseString(buf,"\"", 1, lpszCharset, MAX_PATH);
					if(*lpszCharset != '\0') {
						m_pCharSet = CUT_CharSet::GetCharSetStruct(lpszCharset);
						strcpy(m_lpszDefaultCharSet, lpszCharset);
					}
				}
			}
		}
	}


	return error;
}


/***********************************************
GetCustomFieldName
Gets custom field name by index
PARAM:
lIndex		- index of field data
RETURN:
pointer to field name
NULL - failed
************************************************/
LPCSTR CUT_Msg::GetCustomFieldName(long lIndex)
{
	if(m_listUT_HeaderFields == NULL)
		return NULL;

	UT_HeaderField		*pItem = m_listUT_HeaderFields;
	long			lCount = -1;

	while(pItem != NULL) {
		if(pItem->nFieldID == UTM_CUSTOM_FIELD)
			lCount ++;
		if(lCount == lIndex)
			return pItem->lpszFieldName;
		pItem = pItem->pNext;
	}

	return NULL;
}

/***********************************************
GetFieldName
Gets field name text by field ID
PARAM:
[nID]	- id enum
RETURN:
pointer to field name
NULL - failed
************************************************/
LPCSTR CUT_Msg::GetFieldName(HeaderFieldID nID)
{
	static const char *FieldNames[UTM_MAX_FIELD];

	FieldNames[UTM_ALL_FIELDS]		= "All Fields ";
	FieldNames[UTM_TO]				= "To:";
	FieldNames[UTM_MESSAGE_ID]		= "Message-ID:";
	FieldNames[UTM_CC]				= "Cc:";
	FieldNames[UTM_BCC]				= "Bcc:";
	FieldNames[UTM_FROM]			= "From:";
	FieldNames[UTM_SUBJECT]			= "Subject:";
	FieldNames[UTM_RECEIVED]		= "Received:";
	FieldNames[UTM_IN_REPLY_TO]		= "In-Reply-To:";
	FieldNames[UTM_COMMENTS]		= "Comments:";
	FieldNames[UTM_KEYWORDS]		= "Keywords:";
	FieldNames[UTM_DATE]			= "Date:";
	FieldNames[UTM_NEWSGROUPS]		= "Newsgroups:";
	FieldNames[UTM_XREF]			= "Xref:";
	FieldNames[UTM_REPLY_TO]		= "Reply-To:";
	FieldNames[UTM_XMAILER]			= "X-Mailer:";
	FieldNames[UTM_XNEWS_READER]	= "X-Newsreader:";
	FieldNames[UTM_REFERENCES]		= "References:";
	FieldNames[UTM_MIME_VERSION]	= "MIME-Version:";
	FieldNames[UTM_CONTENT_TYPE]	= "Content-Type:";
	FieldNames[UTM_THREAD_TOPIC]	= "Thread-Topic:";
	FieldNames[UTM_CUSTOM_FIELD]	= "Custom";

	if(nID >= UTM_ALL_FIELDS && nID <= UTM_CUSTOM_FIELD)
		return FieldNames[nID]; 

	return NULL;
}

/*************************************************
GetFieldName()
Gets the text representing a standard header caption
PARAM
name	- [out] pointer to buffer to receive title
maxSize - length of buffer
nID		- enum of the header
size    - [out] length of name

RETURN				
UTE_SUCCES			- ok - 
UTE_NULL_PARAM		- name and/or size is a null pointer
UTE_INDEX_OUTOFRANGE  - name not found
UTE_BUFFER_TOO_SHORT  - space in name buffer indicated by maxSize insufficient, realloc 
based on size returned.
UTE_OUT_OF_MEMORY		- possible in wide char overload
**************************************************/
#if defined _UNICODE
int CUT_Msg::GetFieldName(LPWSTR name, size_t maxSize, HeaderFieldID nID, size_t *size){

	int retval;

	if(maxSize > 0) {
		char * nameA = new char [maxSize];

		if(nameA != NULL) {
			retval = GetFieldName( nameA, maxSize, nID, size);

			if(retval == UTE_SUCCESS) {
				CUT_Str::cvtcpy(name, maxSize, nameA);
			}
			delete [] nameA;
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
	}
	else {
		if(size == NULL) (retval = UTE_NULL_PARAM);
		else {
			LPCSTR lpStr = GetFieldName(nID);
			if(lpStr != NULL) {
				*size = strlen(lpStr)+1;
				retval = UTE_BUFFER_TOO_SHORT;
			}
			else {
				retval = UTE_INDEX_OUTOFRANGE;
			}
		}
	}
	return retval;
}	
#endif
/**************************************************/
/**************************************************/
int CUT_Msg::GetFieldName(LPSTR name, size_t maxSize, HeaderFieldID nID, size_t *size){

	int retval = UTE_SUCCESS;

	if(name == NULL || size == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {

		LPCSTR str = GetFieldName(nID);

		if(str == NULL) {
			retval = UTE_INDEX_OUTOFRANGE;
		}
		else {
			*size = strlen(str);
			if(*size >= maxSize) {
				++(*size);
				retval = UTE_BUFFER_TOO_SHORT;
			}
			else {
				strcpy(name, str);
			}
		}
	}
	return retval;
}

/*************************************************
GetCustomFieldName()
Gets the text representing a non-standard header caption
PARAM
name	  - [out] pointer to buffer to receive title
maxSize - length of buffer
lIndex   - index custom header 
size    - [out] length of title

RETURN				
UTE_SUCCES			- ok - 
UTE_NULL_PARAM		- name and/or size is a null pointer
UTE_INDEX_OUTOFRANGE  - name not found
UTE_BUFFER_TOO_SHORT  - space in name buffer indicated by maxSize insufficient, realloc 
based on size returned.
UTE_OUT_OF_MEMORY		- possible in wide char overload
**************************************************/
#if defined _UNICODE
int CUT_Msg::GetCustomFieldName(LPWSTR name, size_t maxSize, long lIndex, size_t *size){

	int retval;

	if(maxSize > 0) {
		char * nameA = new char [maxSize];

		if(nameA != NULL) {
			retval = GetCustomFieldName( nameA, maxSize, lIndex, size);

			if(retval == UTE_SUCCESS) {
				CUT_Str::cvtcpy(name, maxSize, nameA);
			}
			delete [] nameA;
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
	}
	else {
		if(size == NULL) (retval = UTE_NULL_PARAM);
		else {
			LPCSTR lpStr = GetCustomFieldName(lIndex);
			if(lpStr != NULL) {
				*size = strlen(lpStr)+1;
				retval = UTE_BUFFER_TOO_SHORT;
			}
			else {
				retval = UTE_INDEX_OUTOFRANGE;
			}
		}
	}
	return retval;
}	
#endif
/**************************************************/
/**************************************************/
int CUT_Msg::GetCustomFieldName(LPSTR name, size_t maxSize, long lIndex, size_t *size){

	int retval = UTE_SUCCESS;

	if(name == NULL || size == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {

		LPCSTR str = GetCustomFieldName(lIndex);

		if(str == NULL) {
			retval = UTE_INDEX_OUTOFRANGE;
		}
		else {
			*size = strlen(str);
			if(*size >= maxSize) {
				++(*size);
				retval = UTE_BUFFER_TOO_SHORT;
			}
			else {
				strcpy(name, str);
			}
		}
	}
	return retval;
}

/***********************************************
AddHeaderField
Adds a new message field. If nID == UTM_CUSTOM_FIELD
you must specify the name of the field in lpszFieldName
PARAM:
lpszData			- pointer to the field's field data
nID					- ID of the field 
[lpszFieldName]		- pointer to the field's field name
RETURN:
TRUE	- success
FALSE	- fail
************************************************/
#if defined _UNICODE
BOOL CUT_Msg::AddHeaderField(LPCWSTR lpszData, HeaderFieldID nID, LPCWSTR lpszFieldName){
	return AddHeaderField(AC(lpszData), nID, AC(lpszFieldName));}
#endif
BOOL CUT_Msg::AddHeaderField(LPCSTR lpszData, HeaderFieldID nID, LPCSTR lpszFieldName)
{
	UT_HeaderField		*pItem = m_listUT_HeaderFields;

	// Check parameters
	if(lpszData == NULL)	
		return FALSE;						// Error. Field data is not optional

	if(nID == UTM_CUSTOM_FIELD && lpszFieldName == NULL)	
		return FALSE;						// Error. You must specify Field field name for unknown fields

	char szFieldNameCopy[1024];
	if (lpszFieldName != NULL)
	{
		::strcpy(szFieldNameCopy, lpszFieldName);
		lpszFieldName = szFieldNameCopy;

		// If the field name does not end with a space, add a space to the end
		int iLength = (int)::strlen(lpszFieldName);
		if (iLength > 0 && lpszFieldName[iLength - 1] != ' ')
		{
			::strncat(szFieldNameCopy, " ", 1);
			lpszFieldName = szFieldNameCopy;
		}
	}
	// Create new item
	if(m_listUT_HeaderFields == NULL) {			// If list is empty
		pItem	= new UT_HeaderField;
		m_listUT_HeaderFields = pItem;
	}
	else {										// If list is not empty
		while(pItem->pNext != NULL)				// Search for the last item in the list
			pItem = pItem->pNext;

		pItem->pNext	= new UT_HeaderField;
		pItem = pItem->pNext;
	}

	pItem->nFieldID = nID;						// Set Field ID

	pItem->lpszFieldData = NULL;
	pItem->lpszFieldName = NULL;

	MsgCopyString(pItem->lpszFieldData, lpszData);	// Copy field data

	if(nID == UTM_CUSTOM_FIELD) 
		MsgCopyString(pItem->lpszFieldName, lpszFieldName);	// Copy field data
	else
		MsgCopyString(pItem->lpszFieldName,GetFieldName (nID));

	pItem->pNext = NULL;						// Initialize pointer to the next structure

	return TRUE;
}

/***********************************************
ClearHeader
Clears fields
PARAM:
[nID]	- what kind of fields to remove
RETURN:
none
************************************************/
#if defined _UNICODE
BOOL CUT_Msg::ClearHeader(HeaderFieldID nID, LPCWSTR lpszCustomName){
	return ClearHeader(nID, AC(lpszCustomName));}
#endif
BOOL CUT_Msg::ClearHeader(HeaderFieldID nID, LPCSTR lpszCustomName)
{

	if(m_listUT_HeaderFields == NULL)							// List is already cleared
		return TRUE;

	UT_HeaderField	*pCurItem = m_listUT_HeaderFields,
		*pNextItem = NULL, 
		*pPrevItem = NULL;


	while(pCurItem != NULL) {
		pNextItem = pCurItem->pNext;					// Save pointer to the next structure

		if(nID == UTM_ALL_FIELDS || 
			(nID != UTM_CUSTOM_FIELD && nID == pCurItem->nFieldID) ||
			(nID == UTM_CUSTOM_FIELD && (lpszCustomName == NULL || (pCurItem->lpszFieldName != NULL && _stricmp(lpszCustomName, pCurItem->lpszFieldName) == 0))) ) {

				if(pCurItem->lpszFieldName)
					delete[] pCurItem->lpszFieldName;		// Delete name

				if(pCurItem->lpszFieldData)
					delete[] pCurItem->lpszFieldData;		// Delete data

				if(pCurItem == m_listUT_HeaderFields)			// If the first element was deleted
					if(pPrevItem)
						m_listUT_HeaderFields = pPrevItem;		// reinitialize pointer to the list
					else
						m_listUT_HeaderFields = pNextItem;

				if(pPrevItem)
					pPrevItem->pNext = pNextItem;			// Adjust next structure pointer

				delete pCurItem;							// Delete structure
		}
		else
			pPrevItem = pCurItem;

		pCurItem = pNextItem;							// Go to next structure
	}

	return TRUE;
}

/***********************************************
GetHeaderCount
Returns number of fields
PARAM:
[nID]	- what kind of fields to count
RETURN:
long	- number of headers count
************************************************/
long CUT_Msg::GetHeaderCount(HeaderFieldID nID)
{

	if(m_listUT_HeaderFields == NULL)							// If list is empty
		return 0L;

	UT_HeaderField	*pNextItem = m_listUT_HeaderFields;
	long			lCount = 0;

	while(pNextItem != NULL) {								// count fields
		if(nID == UTM_ALL_FIELDS || nID == pNextItem->nFieldID) 
			lCount ++;
		pNextItem = pNextItem->pNext;
	}

	return lCount;
}

/***********************************************
GetHeaderCount
Returns number of field 
PARAM:
name	- name of the field to count
RETURN:
TRUE	- success
FALSE	- fail
************************************************/
#if defined _UNICODE
long CUT_Msg::GetHeaderCount(LPCWSTR name){
	return GetHeaderCount(AC(name));}
#endif
long CUT_Msg::GetHeaderCount(LPCSTR name)
{

	if(m_listUT_HeaderFields == NULL || name == NULL)			// If list is empty
		return 0;

	UT_HeaderField	*pNextItem = m_listUT_HeaderFields;
	long			lCount = 0;

	while(pNextItem != NULL) {								// count fields
		if(pNextItem->lpszFieldName)
			if(_stricmp(name, pNextItem->lpszFieldName) == 0) 
				lCount ++;
		pNextItem = pNextItem->pNext;
	}

	return lCount;
}

/***********************************************
GetHeaderByType
Get Field data with specified index by it's ID.
PARAM:
nID			- id of fields
lIndex		- index of field data
RETURN:
pointer to header string
************************************************/
LPCSTR CUT_Msg::GetHeaderByType(HeaderFieldID nID, long lIndex)
{
	if(m_listUT_HeaderFields == NULL)
		return NULL;

	UT_HeaderField		*pItem = m_listUT_HeaderFields;
	long			lCount = -1;

	while(pItem != NULL) {
		if(nID == UTM_ALL_FIELDS || nID == pItem->nFieldID)
			lCount ++;
		if(lCount == lIndex)
			return pItem->lpszFieldData;
		pItem = pItem->pNext;
	}

	return NULL;
}
/***********************************************
GetHeaderByType
Get Field data with specified index by it's ID.
PARAM:
nID			- id of fields
lIndex		- index of field data
headerText  - [out] buffer to receive text
maxSize		- [in] lenght of buffer in chars
size		- [out]actual length of string returned or required.
RETURN:
RETURN				
UTE_SUCCES			- ok - 
UTE_NULL_PARAM		- headerText and/or size is a null pointer
UTE_INDEX_OUTOFRANGE  - not found
UTE_BUFFER_TOO_SHORT  - space in title buffer indicated by maxSize insufficient, realloc 
based on size returned.
UTE_OUT_OF_MEMORY		- possible in wide char overload

************************************************/
int CUT_Msg::GetHeaderByType(LPSTR headerText, size_t maxSize, HeaderFieldID nID, long lIndex, size_t *size)
{
	int retval = UTE_SUCCESS;

	if(headerText == NULL || size == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {

		LPCSTR str = GetHeaderByType(nID,lIndex);

		if(str == NULL) {
			retval = UTE_INDEX_OUTOFRANGE;
		}
		else {
			*size = strlen(str);
			if(*size >= maxSize) {
				++(*size);
				retval = UTE_BUFFER_TOO_SHORT;
			}
			else {
				strcpy(headerText, str);
			}
		}
	}
	return retval;
}
#if defined _UNICODE
int CUT_Msg::GetHeaderByType(LPWSTR headerText, size_t maxSize, HeaderFieldID nID, long lIndex, size_t *size)
{
	int retval;

	if(maxSize > 0) {
		char * headerTextA = new char [maxSize];

		if(headerTextA != NULL) {
			retval = GetHeaderByType( headerTextA, maxSize, nID, lIndex, size);

			if(retval == UTE_SUCCESS) {
				// TD v4.2 determine charset 
				CUT_CharSetStruct *pCharSet;
				if(*m_lpszHeaderCharSet != '\0') {
					pCharSet = CUT_CharSet::GetCharSetStruct(m_lpszHeaderCharSet);
				}
				else {
					pCharSet = m_pCharSet;
				}
				CUT_Str::cvtcpy(headerText, maxSize, headerTextA, pCharSet->nCodePage);
			}
			delete [] headerTextA;
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
	}
	else {
		if(size == NULL) (retval = UTE_NULL_PARAM);
		else {
			LPCSTR lpStr = GetHeaderByType(nID,lIndex);
			if(lpStr != NULL) {
				*size = strlen(lpStr)+1;
				retval = UTE_BUFFER_TOO_SHORT;
			}
			else {
				retval = UTE_INDEX_OUTOFRANGE;
			}
		}
	}
	return retval;
}
#endif

/***********************************************
GetHeaderByType
Get Field data by it's type
PARAM:
nID			- field type
buff		- buffer for the data
size		- size of buffer
RETURN:
TRUE	- success
FALSE	- fail
************************************************/
#if defined _UNICODE
BOOL CUT_Msg::GetHeaderByType(HeaderFieldID nID, LPWSTR buff, int size){
	char * buffA = (char*) alloca(size+1);
	*buffA = '\0';
	*buff = _T('\0');
	BOOL result = GetHeaderByType(nID, buffA, size);
	if(result == TRUE) {
		CUT_Str::cvtcpy(buff, size, buffA, m_pCharSet->nCodePage);
	}
	return result;
}
#endif
BOOL CUT_Msg::GetHeaderByType(HeaderFieldID nID, LPSTR buff, int size)
{
	int		index, nBytesCopied = 0, nHeaderCount = GetHeaderCount(nID);
	LPCSTR	ptrData;

	if(buff == NULL || size <= 0 || nHeaderCount < 1)
		return FALSE;								// Wrong parameters

	buff[0] = 0;

	for(index = 0; index < nHeaderCount; index ++) {
		if((ptrData = GetHeaderByType(nID, index)) != NULL) {
			nBytesCopied += (int)strlen(ptrData);
			if(nBytesCopied >= size)
				return FALSE;						// Buffer too small

			strcat(buff, ptrData);
		}
	}

	return TRUE;
}

/***********************************************
GetHeaderByName
Get Field data with specified index by it's name
PARAM:
name		- field name
lIndex		- index of field data
RETURN:
TRUE	- success
FALSE	- fail
************************************************/
#if defined _UNICODE
LPCSTR CUT_Msg::GetHeaderByName(LPCWSTR name, long lIndex){
	return GetHeaderByName( AC(name), lIndex);}
#endif
LPCSTR CUT_Msg::GetHeaderByName(LPCSTR name, long lIndex)
{
	if(m_listUT_HeaderFields == NULL || name == NULL)		
		return NULL;

	UT_HeaderField	*pItem = m_listUT_HeaderFields;
	long			lCount = -1;

	while(pItem != NULL) {
		if(pItem->lpszFieldName != NULL && _stricmp(name,pItem->lpszFieldName) == 0)
			lCount ++;
		if(lCount == lIndex)
			return pItem->lpszFieldData;
		pItem = pItem->pNext;
	}

	return NULL;
}

/***********************************************
MsgCopyString
Copying string checking the memory allocation
PARAM:
dest	- destination address
source	- source string
RETURN:
none
************************************************/
void CUT_Msg::MsgCopyString(LPSTR &dest, LPCSTR source)
{
	if(source != NULL) {
		if(dest)	
			delete [] dest;
		dest = new char[strlen(source) + 1];
		strcpy(dest, source);
	}
}

/***********************************************
GetHeaderByName
Get Field data by it's name
PARAM:
name		- field name
buff		- buffer for the data
size		- size of buffer
RETURN:
TRUE	- success
FALSE	- fail
************************************************/
#if defined _UNICODE
BOOL CUT_Msg::GetHeaderByName(LPCWSTR name, LPWSTR buff, int size){
	char * buffA = (char*) alloca(size+1);
	*buffA = '\0';
	BOOL result = GetHeaderByName(AC(name), buffA, size);
	if(result == TRUE) {
		// TD v4.2 determine charset 
		CUT_CharSetStruct *pCharSet;
		if(*m_lpszHeaderCharSet != '\0') {
			pCharSet = CUT_CharSet::GetCharSetStruct(m_lpszHeaderCharSet);
		}
		else {
			pCharSet = m_pCharSet;
		}
		CUT_Str::cvtcpy(buff, size, buffA, m_pCharSet->nCodePage);
	}
	return result;
}
#endif
BOOL CUT_Msg::GetHeaderByName(LPCSTR name, LPSTR buff, int size)
{
	int		index, nBytesCopied = 0, nHeaderCount = GetHeaderCount(name);
	LPCSTR	ptrData;

	if(buff == NULL || name == NULL || size <= 0 || nHeaderCount < 1)
		return FALSE;								// Wrong parameters

	buff[0] = 0;

	for(index = 0; index < nHeaderCount; index ++) {
		if((ptrData = GetHeaderByName(name, index)) != NULL) {
			nBytesCopied += (int)strlen(ptrData);
			if(nBytesCopied >= size)
				return FALSE;						// Buffer too small

			strcat(buff, ptrData);
		}
	}

	return TRUE;
}
/***********************************************
SetMaxMessageBodySize
Sets maximum message body buffer size
PARAM:
maxsize	- maximum buffer size
RETURN:
none
************************************************/
void CUT_Msg::SetMaxMessageBodySize(unsigned int maxsize)
{
	m_nMaxMessageBodySize = maxsize;
}

/***********************************************
SetMaxMessageBodySize
Gets maximum message body buffer size
PARAM:
none
RETURN:
maximum buffer size
************************************************/
unsigned int CUT_Msg::GetMaxMessageBodySize()
{
	return m_nMaxMessageBodySize;
}

/***********************************************
SetMessageBody
Sets Message Body
PARAM:
body	- message body
RETURN:
TRUE	- success
FALSE	- error
************************************************/
#if defined _UNICODE
// v4.2 use cvtcpy here rather than AC() macro - size may be too large for alloca.
BOOL CUT_Msg::SetMessageBody(LPCWSTR body) {
	BOOL retval = FALSE;

	if(body != NULL) {
		size_t len = _tcslen(body);
		char * bodyA = new char[len+1];
		CUT_Str::cvtcpy(bodyA, len, body);
		retval = SetMessageBody(bodyA);
		delete [] bodyA;
	}
	return retval;
}
#endif
BOOL CUT_Msg::SetMessageBody(LPCSTR body) 
{
	if( (strlen(body) + 1) > m_nMaxMessageBodySize)
		return FALSE;

	MsgCopyString(m_lpszMessageBody, body);
	return TRUE;
}

/***********************************************
SetHtmlMessageBody
Sets the HTML Message Body.
PARAM:
body	- message body, pass NULL to remove the HTML body
RETURN:
TRUE	- success
FALSE	- error
************************************************/
#if defined _UNICODE
// v4.2 use cvtcpy here rather than AC() macro - size may be too large for alloca.
BOOL CUT_Msg::SetHtmlMessageBody(LPCWSTR body) {
	BOOL retval = FALSE;

	if(body != NULL) {
		size_t len = _tcslen(body);
		char * bodyA = new char[len+1];
		CUT_Str::cvtcpy(bodyA, len, body);
		retval = SetHtmlMessageBody(bodyA);
		delete [] bodyA;
	}
	return retval;
}
#endif
BOOL CUT_Msg::SetHtmlMessageBody(LPCSTR body) 
{
	int iRes;

	if (body == NULL || body[0] == 0)
	{
		// Remove the html body attachment
		if(m_ptrEncoding[m_nDefEncodingObj] != NULL)
			iRes = m_ptrEncoding[m_nDefEncodingObj]->RemoveHtmlBody();
		else
			return FALSE;
	}
	else
	{
		// Add the html body attachment
		if(m_ptrEncoding[m_nDefEncodingObj] != NULL)
			iRes = m_ptrEncoding[m_nDefEncodingObj]->AddHtmlBody(body);
		else
			return FALSE;
	}

	if (iRes == UTE_SUCCESS)
		return TRUE;
	else
		return FALSE;
}

/***********************************************
GetMessageBodyLength / GetHtmlMessageBodyLength
retrieves the length of the message body, 
including termination.
PARAM:
none
RETURN:
message body length
************************************************/
size_t CUT_Msg::GetMessageBodyLength() {
	size_t retval = 0;
	if(m_lpszMessageBody != NULL && *m_lpszMessageBody != '\0') {

		// note that this optional unicode section could return a more accurate 
		// length, but it would sometimes be smaller than the ASCII length for 
		// message bodys that are raw unicode scars (i.e. not encoded). Simpler 
		// just to return the actual ascii length.
//#if defined _UNICODE
//		retval = (size_t)MultiByteToWideChar(m_pCharSet->nCodePage, 0, m_lpszMessageBody, -1, NULL, (int)retval);
//#else
		retval = strlen(m_lpszMessageBody)+1;
//#endif 
	}
	return retval;
}

size_t CUT_Msg::GetHtmlMessageBodyLength() {
	size_t retval = 0;
	if(m_lpszHtmlMessageBody != NULL && *m_lpszHtmlMessageBody != '\0') {
		retval = strlen(m_lpszHtmlMessageBody)+1;
	}
	return retval;
}		

/***********************************************
GetMessageBody
Gets Message Body
PARAM:
none
RETURN:
message body
************************************************/
LPCSTR CUT_Msg::GetMessageBody()
{
	// v4.2  seems ok to allow both text and html to be retrieved...
	return m_lpszMessageBody;

	//	if (m_bHtmlBody)
	//		return m_lpszEmptyString;
	//	else
	//		return m_lpszMessageBody;
}

/*************************************************
GetMessageBody()
Retrieves the message body (text) - may return a zero
lenghth string if the body text has been flagged as HTML.
See GetHtmlMessageBody

PARAM
messageText	  - [out] pointer to buffer to receive text
maxSize - length of buffer
size    - [out] length of message text

RETURN				
UTE_SUCCES			- ok - 
UTE_NULL_PARAM		- title and/or size is a null pointer
UTE_BUFFER_TOO_SHORT  - space in title buffer indicated by maxSize insufficient, realloc 
based on size returned.
UTE_OUT_OF_MEMORY		- possible in wide char overload

  
**************************************************/
#if defined _UNICODE
int CUT_Msg::GetMessageBody(LPWSTR messageText, size_t maxSize, size_t *size){

	int retval;

	if(maxSize > 0) {
		char * messageTextA = new char [maxSize+1];

		if(messageTextA != NULL) {
			retval = GetMessageBody( messageTextA, maxSize, size);

			if(retval == UTE_SUCCESS) {
				// TD v4.2 determine charset 
				CUT_CharSetStruct *pCharSet;
				if(*m_lpszTextCharSet != '\0') {
					pCharSet = CUT_CharSet::GetCharSetStruct(m_lpszTextCharSet);
				}
				else {
					pCharSet = m_pCharSet;
				}
				CUT_Str::cvtcpy(messageText, maxSize, messageTextA, pCharSet->nCodePage);
			}
			delete [] messageTextA;
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
	}
	else {
		if(size == NULL) (retval = UTE_NULL_PARAM);
		else {
			LPCSTR lpStr = GetMessageBody();
			if(lpStr != NULL) {
				*size = strlen(lpStr)+1;
				retval = UTE_BUFFER_TOO_SHORT;
			}
			else {
				retval = UTE_INDEX_OUTOFRANGE;
			}
		}
	}
	return retval;
}	
#endif
/**************************************************/
/**************************************************/
int CUT_Msg::GetMessageBody(LPSTR messageText, size_t maxSize, size_t *size){

	int retval = UTE_SUCCESS;

	if(messageText == NULL || size == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {

		LPCSTR str = GetMessageBody();

		if(str == NULL) {
			*messageText = 0;
		}
		else {
			*size = strlen(str);
			if(*size >= maxSize) {
				++(*size);
				retval = UTE_BUFFER_TOO_SHORT;
			}
			else {
				strcpy(messageText, str);
			}
		}
	}
	return retval;
}

/***********************************************
GetHtmlMessageBody
Retrieves the HTML body of the message (if present)
PARAM:
none

RETURN:
pointer to the html message body
************************************************/
LPCSTR CUT_Msg::GetHtmlMessageBody()
{
	// v4.2  seems ok to allow both text and html to be retrieved...
	return m_lpszHtmlMessageBody;
	//	if (m_bHtmlBody)
	//		return m_lpszHtmlMessageBody;
	//	else
	//		return m_lpszEmptyString;
}

/*************************************************
GetHtmlMessageBody()
Retrieves the html message body (text) - may return a zero
lenghth string if the body text has not been flagged as HTML.
See GetMessageBody

PARAM
messageText	  - [out] pointer to buffer to receive text
maxSize - length of buffer
size    - [out] length of message text

RETURN				
UTE_SUCCES			- ok - 
UTE_NULL_PARAM		- title and/or size is a null pointer
UTE_BUFFER_TOO_SHORT  - space in title buffer indicated by maxSize insufficient, realloc 
based on size returned.
UTE_OUT_OF_MEMORY		- possible in wide char overload
**************************************************/
#if defined _UNICODE
int CUT_Msg::GetHtmlMessageBody(LPWSTR messageText, size_t maxSize, size_t *size){

	int retval;

	if(maxSize > 0) {
		char * messageTextA = new char [maxSize];

		if(messageTextA != NULL) {
			retval = GetHtmlMessageBody( messageTextA, maxSize, size);

			if(retval == UTE_SUCCESS) {
				// TD v4.2 determine charset 
				CUT_CharSetStruct *pCharSet;
				if(*m_lpszHtmlCharSet != '\0') {
					pCharSet = CUT_CharSet::GetCharSetStruct(m_lpszHtmlCharSet);
				}
				else {
					pCharSet = m_pCharSet;
				}
				CUT_Str::cvtcpy(messageText, maxSize, messageTextA, pCharSet->nCodePage);
			}
			delete [] messageTextA;
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
	}
	else {
		if(size == NULL) (retval = UTE_NULL_PARAM);
		else {
			LPCSTR lpStr = GetHtmlMessageBody();
			if(lpStr != NULL) {
				*size = strlen(lpStr)+1;
				retval = UTE_BUFFER_TOO_SHORT;
			}
			else {
				retval = UTE_INDEX_OUTOFRANGE;
			}
		}
	}
	return retval;
}	
#endif
/**************************************************/
/**************************************************/
int CUT_Msg::GetHtmlMessageBody(LPSTR messageText, size_t maxSize, size_t *size){

	int retval = UTE_SUCCESS;

	if(messageText == NULL || size == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {

		LPCSTR str = GetHtmlMessageBody();

		if(str == NULL) {
			*messageText = 0;
		}
		else {
			*size = strlen(str);
			if(*size >= maxSize) {
				++(*size);
				retval = UTE_BUFFER_TOO_SHORT;
			}
			else {
				strcpy(messageText, str);
			}
		}
	}
	return retval;
}


/***********************************************
IsHtmlMessageBodyPresent
Determines if there is an HTML body
PARAM:
none

RETURN:
TRUE	:	There is an HTML body
FALSE	:	There is no HTML body
************************************************/
BOOL CUT_Msg::IsHtmlMessageBodyPresent()
{
	if (m_bHtmlBody || m_lpszHtmlMessageBody[0] != NULL)
		return TRUE;
	else
		return FALSE;
}

/***********************************************
AddAttachment
Adds filename to the attachments list
PARAM:
filename	- filename to add
[params]	- list of optional parameters

lpszFileNameCharSet - Character set of the filename

RETURN:
UTE_SUCCESS			- success
UTE_DS_OPEN_FAILED	- failed to open attachment data source
************************************************/
#if defined _UNICODE
int	CUT_Msg::AddAttachment(LPCWSTR filename, CUT_StringList *params) {
	return AddAttachment(AC(filename), params);}
#endif
int	CUT_Msg::AddAttachment(LPCSTR filename, CUT_StringList *params) 
{
	// Clear flag of decoding operation
	m_bDecoding	= FALSE;

	if(m_ptrEncoding[m_nDefEncodingObj] != NULL)
		return m_ptrEncoding[m_nDefEncodingObj]->AddAttachment(filename, params);//, FileNameEncodingType, lpszFileNameCharSet);
	else
		return FALSE;
}

/***********************************************
AddAttachment
Adds datasource to the attachments list
PARAM:
source		- datasource to add
name		- name of the datasource
[params]	- list of optional parameters
RETURN:
UTE_SUCCESS			- success
UTE_DS_OPEN_FAILED	- failed to open attachment data source
************************************************/
#if defined _UNICODE 
int	CUT_Msg::AddAttachment(CUT_DataSource & source, LPCWSTR name, CUT_StringList *params){
	return AddAttachment(source, AC(name), params);}
#endif
int	CUT_Msg::AddAttachment(CUT_DataSource & source, LPCSTR name, CUT_StringList *params)
{
	// Clear flag of decoding operation
	m_bDecoding	= FALSE;

	if(m_ptrEncoding[m_nDefEncodingObj] != NULL)
		return m_ptrEncoding[m_nDefEncodingObj]->AddAttachment(source, name, params);
	else
		return FALSE;
}

/***********************************************
GetAttachmentNumber
Gets number of the attachments. After decoding
Decode() returns the number of decoded attachments,
after AddAttachment() returns number of attachments 
prepared to Encode.
PARAM:
none
RETURN:
int		- number of attachments
************************************************/
int	CUT_Msg::GetAttachmentNumber()
{
	// If last operation was decoding call current decode object method
	if(m_bDecoding && m_ptrEncoding[m_nDecodingObj] != NULL)
		return m_ptrEncoding[m_nDecodingObj]->GetAttachmentNumber();

	// If last operation was not decoding call default object method
	else if(m_ptrEncoding[m_nDefEncodingObj] != NULL)
		return m_ptrEncoding[m_nDefEncodingObj]->GetAttachmentNumber();

	// Error. Cant find any encoding objects
	else
		return 0;
}

/***********************************************
GetGlobalHeadersNumber
Get the number of global headers that need to be added 
to the message global headers
PARAM:
none
RETURN:
int		- number of global headers
************************************************/
int	CUT_Msg::GetGlobalHeadersNumber()
{
	if(m_ptrEncoding[m_nDefEncodingObj] != NULL)
		return m_ptrEncoding[m_nDefEncodingObj]->GetGlobalHeadersNumber();
	else
		return 0;
}

/***********************************************
GetGlobalHeader
Get global header by index
PARAM:
index	- index of the header
RETURN:
pointer to the header
NULL	- if error
************************************************/
LPCSTR CUT_Msg::GetGlobalHeader(int index)
{
	if(m_ptrEncoding[m_nDefEncodingObj] != NULL)
		return m_ptrEncoding[m_nDefEncodingObj]->GetGlobalHeader(index);
	else
		return NULL;
}
/*************************************************
GetGlobalHeader()
Gets a header which is common to all MIME attachments available in the message
PARAM
header	- [out] pointer to buffer to receive title
maxSize - length of buffer
index	- index of the header
size    - [out] length of title

RETURN				
UTE_SUCCES			- ok - 
UTE_NULL_PARAM		- title and/or size is a null pointer
UTE_INDEX_OUTOFRANGE  - title not found
UTE_BUFFER_TOO_SHORT  - space in title buffer indicated by maxSize insufficient, realloc 
based on size returned.
UTE_OUT_OF_MEMORY		- possible in wide char overload
**************************************************/
int CUT_Msg::GetGlobalHeader(LPSTR header, size_t maxSize, int index, size_t *size){
	int retval = UTE_SUCCESS;

	if(header == NULL || size == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {

		LPCSTR str = GetGlobalHeader(index);

		if(str == NULL) {
			retval = UTE_INDEX_OUTOFRANGE;
		}
		else {
			*size = strlen(str);
			if(*size >= maxSize) {
				++(*size);
				retval = UTE_BUFFER_TOO_SHORT; 
			}
			else {
				strcpy(header, str);
			}
		}
	}
	return retval;	
}
#if defined _UNICODE
int CUT_Msg::GetGlobalHeader(LPWSTR header, size_t maxSize, int index, size_t *size){
	int retval;

	if(maxSize > 0) {
		char * headerA = new char [maxSize];

		if(headerA != NULL) {
			retval = GetGlobalHeader( headerA, maxSize, index, size);

			if(retval == UTE_SUCCESS) {
				CUT_Str::cvtcpy(header, maxSize, headerA);
			}
			delete [] headerA;
		}
		else {
			retval = UTE_OUT_OF_MEMORY;
		}
	}
	else {
		if(size == NULL) (retval = UTE_NULL_PARAM);
		else {
			LPCSTR lpStr = GetGlobalHeader(index);
			if(lpStr != NULL) {
				*size = strlen(lpStr)+1;
				retval = UTE_BUFFER_TOO_SHORT;
			}
			else {
				retval = UTE_INDEX_OUTOFRANGE;
			}
		}
	}
	return retval;
}
#endif

/***********************************************
EmptyAttachmentList
Delete all attachments 
PARAM:
index	- index of the header
RETURN:
none	
************************************************/
void CUT_Msg::EmptyAttachmentList()
{
	// Clear flag of decoding operation
	m_bDecoding	= FALSE;
	for(int i = 0; i < m_nEncodingObjNumber; i ++) {
		if(m_ptrEncoding[i] != NULL)
			m_ptrEncoding[i]->EmptyAttachmentList();
	}
}

/***********************************************
AddAttachmentHeaderTag
Add custom headers to an individual attachment
Affect the last added attachment. Can be called
several times to form a list of headers.
PARAM:
tag		- header to add
RETURN:

************************************************/
#if defined _UNICODE
int CUT_Msg::AddAttachmentHeaderTag(LPCWSTR tag){
	return AddAttachmentHeaderTag(AC(tag));}
#endif
int CUT_Msg::AddAttachmentHeaderTag(LPCSTR tag)
{
	if(m_ptrEncoding[m_nDefEncodingObj] != NULL)
		return m_ptrEncoding[m_nDefEncodingObj]->AddFileHeaderTag(tag);
	else
		return FALSE;
}
/***********************************************
GetAttachmentInfo
Fills in attachment information
PARAM:
index	- index of the attachment
name	- buffer for the name of the attachment
type	- buffer for the type of the attachment
maxsize	- size of each buffer
[params]- optinal parameters
RETURN:

************************************************/
#if defined _UNICODE
int CUT_Msg::GetAttachmentInfo(	int index, LPWSTR name, int namesize, LPWSTR type, int typesize, CUT_TStringList *params){
	char *nameA = (char*) alloca(namesize+1);
	char *typeA = (char*) alloca(typesize+1);
	*nameA = '\0';
	*typeA = '\0';

	CUT_StringList	paramsA;
	int result = GetAttachmentInfo(index, nameA, namesize, typeA, typesize, &paramsA);
	if(name != NULL) {
		CUT_Str::cvtcpy(name, namesize, nameA);
	}
	if(type != NULL) {
		CUT_Str::cvtcpy(type, typesize, typeA);
	}

	// the params string list will probably jusr retrieve one string, the encoding type,
	// but we'll treat this as a list.
	if(params != NULL) {
		_TCHAR temp[MAX_PATH];
		for(int i = 0; i < paramsA.GetCount(); ++i ) {
			CUT_Str::cvtcpy(temp, MAX_PATH, paramsA.GetString(i));
			params->AddString(temp);
		}
	}
	return result;
}
#endif 

int CUT_Msg::GetAttachmentInfo(		int index, 
							   LPSTR name, 
							   int namesize,
							   LPSTR type,
							   int typesize,
							   CUT_StringList *params)
{
	if(m_ptrEncoding[m_nDecodingObj] != NULL)
		return m_ptrEncoding[m_nDecodingObj]->GetAttachmentInfo(index, name, namesize, type, typesize, params);
	else
		// v4.2 changed - was returning FALSE
		return UTE_ERROR;
}

/***********************************************
Encode
Encode all attchments into the data source
PARAM:

RETURN:

************************************************/
int CUT_Msg::Encode(CUT_DataSource & msg_body, CUT_DataSource & dest)
{
	// Clear flag of decoding operation
	m_bDecoding	= FALSE;

	if(m_ptrEncoding[m_nDefEncodingObj] != NULL) 
	{
		m_ptrEncoding[m_nDefEncodingObj]->SetHTMLBody(m_bHtmlBody);

		return m_ptrEncoding[m_nDefEncodingObj]->Encode(msg_body, dest);
	}
	else
		return FALSE;
}
/***********************************************
Encode
Encode all attchments into the file
PARAM:

RETURN:

************************************************/
int CUT_Msg::Encode(LPSTR message, LPCTSTR filename)
{
	// Clear flag of decoding operation
	m_bDecoding	= FALSE;

	if(m_ptrEncoding[m_nDefEncodingObj] != NULL)
		return m_ptrEncoding[m_nDefEncodingObj]->Encode(message, filename);
	else
		return FALSE;
}

/***********************************************
Decode
Decode attchment by index into the data source
PARAM:

RETURN:

************************************************/
int CUT_Msg::Decode(int index, CUT_DataSource & dest)
{
	if(m_ptrEncoding[m_nDecodingObj] != NULL)
		return m_ptrEncoding[m_nDecodingObj]->Decode(index, dest);
	else
		return FALSE;
}
/***********************************************
Decode
Decode attchment by index into the file
PARAM:

RETURN:

************************************************/
int CUT_Msg::Decode(int index, LPCTSTR filename)
{
	if(m_ptrEncoding[m_nDecodingObj] != NULL)
		return m_ptrEncoding[m_nDecodingObj]->Decode(index, filename);
	else
		return FALSE;
}

/***********************************************
SetDefEncoding
Sets default encoding object by name
PARAM:
name	- name of the encoding object ("UUEncode", "Mime", ...)
RETURN:
UTE_SUCCESS	- success
UTE_ERROR	- error
************************************************/
#if defined _UNICODE 
int CUT_Msg::SetDefEncoding(LPCWSTR name) {
	return SetDefEncoding(AC(name));}
#endif
int CUT_Msg::SetDefEncoding(LPCSTR name)
{
	int		i;

	for (i=0; i<m_nEncodingObjNumber; i++)
		if(m_ptrEncoding[i] != NULL && _stricmp(name, m_ptrEncoding[i]->GetName()) == 0) {
			m_nDefEncodingObj = i;
			return UTE_SUCCESS;
		}

		return UTE_ERROR;
}

/***********************************************
AddEncodingObject
Adds new encoding object
PARAM:
name	- name of the encoding object ("UUEncode", "Mime", ...)
RETURN:
UTE_SUCCESS	- success
UTE_ERROR	- error
************************************************/
int CUT_Msg::AddEncodingObject(CUT_Encode &encoding)
{
	int		i;
	LPCSTR	name = encoding.GetName();

	// First try to find it
	for (i=0; i<m_nEncodingObjNumber; i++)
		if(m_ptrEncoding[i] != NULL && _stricmp(name, m_ptrEncoding[i]->GetName()) == 0) {
			m_nDefEncodingObj = i;
			return UTE_SUCCESS;
		}

		// Add a new object
		if((m_nEncodingObjNumber + 1) < MAX_ENCODING_OBJECTS) {
			m_ptrEncoding[m_nEncodingObjNumber] = &encoding;
			++m_nEncodingObjNumber;
			return UTE_SUCCESS;
		}

		return UTE_ERROR;
}

/********************************
GetAttachmentSize
Gets the size of all or specified attachment
PARAM:
index		- index of the attachment
RETURN:
length of attachment
*********************************/
long CUT_Msg::GetAttachmentSize(int index) 
{
	if(m_ptrEncoding[m_nDecodingObj] != NULL)
		return m_ptrEncoding[m_nDecodingObj]->GetAttachmentSize(index);
	else
		return 0L;
}
/************************************
Calling this function and passing TRUE prior to calling SetMessageBody(...)
will set the message body to html.

This function is to be used only if the body of the e-mail is HTML only.
If the body contains a plain text part and an html part use SetMessageBody(...)
to set the plain text and SetHtmlMessageBody(...) to set the html part.

PARAM:
flag	- TRUE will set it to html
FALSE will set it to plain text

*************************************/
void CUT_Msg::SetBodyAsHTML(BOOL flag)
{

	m_bHtmlBody = flag;

	if (m_bHtmlBody)
		AddHeaderField ("text/html; charset=US-ASCII", UTM_CONTENT_TYPE );
}

/***********************************************
EncodeHeader
Encodes a header
PARAM:
nHeaderID		- ID of the header (UTM_SUBJECT, UTM_DATE, etc.)
EncodingType	- encoding algorithm (ENCODING_BASE64 or ENCODING_QUOTED_PRINTABLE)
lpszCharSet		- character set of the header ("utf-8", ...)
RETURN:
UTE_SUCCESS	- success
UTE_ERROR	- error
************************************************/
int CUT_Msg::EncodeHeader(HeaderFieldID nHeaderID, enumEncodingType EncodingType, LPCSTR lpszCharSet)
{
	if (EncodingType == ENCODING_NONE)
		return UTE_ERROR; // cannot use none!

	// Find the header
	if (m_listUT_HeaderFields == NULL)
		return UTE_ERROR;

	// v4.2 if charset NULL, use default charset 
	if(lpszCharSet == NULL) {
		lpszCharSet = m_lpszDefaultCharSet;
	}

	UT_HeaderField*	pItem = m_listUT_HeaderFields;
	while (pItem != NULL)
	{
		if (nHeaderID == pItem->nFieldID && pItem->lpszFieldData != NULL)
		{
			// Encode this header
			CUT_HeaderEncoding HE;
			LPCSTR lpszEncoded = HE.Encode(pItem->lpszFieldData, lpszCharSet, EncodingType);
			if(lpszEncoded != NULL) {
				delete [] pItem->lpszFieldData;
				pItem->lpszFieldData = new char[::strlen(lpszEncoded) + 1];
				::strcpy(pItem->lpszFieldData, lpszEncoded);
			}
			else {
				return HE.GetLastError();
			}
		}
		pItem = pItem->pNext;
	}

	return UTE_SUCCESS;
}
/***********************************************
DecodeHeader
Decodes a header
PARAM:
nHeaderID	- ID of the header (UTM_SUBJECT, UTM_DATE, etc.)
lpszCharSet	- (out) character set of the decoded text
RETURN:
UTE_SUCCESS	- success
UTE_ERROR	- error
************************************************/
#if defined _UNICODE
int CUT_Msg::DecodeHeader(LPCWSTR lpszName, LPSTR lpszCharSet) {
	return DecodeHeader(AC(lpszName),  lpszCharSet);
}
#endif
int CUT_Msg::DecodeHeader(LPCSTR lpszName, LPSTR lpszCharSet)
{
	// Find the header
	if (m_listUT_HeaderFields == NULL || lpszName == NULL)
		return UTE_ERROR;

	UT_HeaderField*	pItem = m_listUT_HeaderFields;
	while (pItem != NULL)
	{
		if (strcmp(lpszName,pItem->lpszFieldName) == 0 && pItem->lpszFieldData != NULL)
		{
			// Decode this header
			CUT_HeaderEncoding HE;
			if (HE.IsEncoded(pItem->lpszFieldData))
			{
				if(lpszCharSet != NULL) {
					*lpszCharSet = '\0';
				}
				LPCSTR lpszDecoded = HE.Decode(pItem->lpszFieldData, lpszCharSet);
				delete [] pItem->lpszFieldData;
				pItem->lpszFieldData = new char[::strlen(lpszDecoded) + 1];
				::strcpy(pItem->lpszFieldData, lpszDecoded);
				if(*lpszCharSet != '\0') {
					strcpy(m_lpszHeaderCharSet, lpszCharSet);
				}
			}
		}
		pItem = pItem->pNext;
	}

	return UTE_SUCCESS;
}

/***********************************************
DecodeHeader
Decodes a header
PARAM:
nHeaderID	- ID of the header (UTM_SUBJECT, UTM_DATE, etc.)
lpszCharSet	- (out) character set of the decoded text
RETURN:
UTE_SUCCESS	- success
UTE_ERROR	- error
************************************************/
int CUT_Msg::DecodeHeader(HeaderFieldID nHeaderID, LPSTR lpszCharSet)
{
	// Find the header
	if (m_listUT_HeaderFields == NULL)
		return UTE_ERROR;

	UT_HeaderField*	pItem = m_listUT_HeaderFields;
	while (pItem != NULL)
	{
		if (nHeaderID == pItem->nFieldID && pItem->lpszFieldData != NULL)
		{
			// Decode this header
			CUT_HeaderEncoding HE;
			if (HE.IsEncoded(pItem->lpszFieldData))
			{
				if(lpszCharSet != NULL) {
					*lpszCharSet = '\0';
				}
				LPCSTR lpszDecoded = HE.Decode(pItem->lpszFieldData, lpszCharSet);
				delete [] pItem->lpszFieldData;
				pItem->lpszFieldData = new char[::strlen(lpszDecoded) + 1];
				::strcpy(pItem->lpszFieldData, lpszDecoded);

				// v4.2 set this as the header encoding charset for the message
				// * should be more checking on this param *
				if(*lpszCharSet != '\0') {
					strcpy(m_lpszHeaderCharSet, lpszCharSet);
				}
			}
		}
		pItem = pItem->pNext;
	}

	return UTE_SUCCESS;
}

/***********************************************
EncodeAttachmentFileName
Encodes the filename of the attachment
PARAM:
iIndex			- index of the attachment
EncodingType	- encoding algorithm (ENCODING_BASE64 or ENCODING_QUOTED_PRINTABLE)
lpszCharSet		- character set of the header ("utf-8", ...)
RETURN:
UTE_SUCCESS	- success
UTE_ERROR	- error
************************************************/
int CUT_Msg::EncodeAttachmentFileName(int iIndex, enumEncodingType EncodingType, LPCSTR lpszCharSet)
{
	if(m_ptrEncoding[m_nDefEncodingObj] != NULL)
		return m_ptrEncoding[m_nDefEncodingObj]->EncodeAttachmentFileName(iIndex, EncodingType, lpszCharSet);
	else
		return UTE_ERROR;
}

/***********************************************
DecodeAttachmentFileName
Decodes the filename of the attachment
PARAM:
iIndex			- index of the attachment
lpszCharSet		- (out) character set of the decoded text
RETURN:
UTE_SUCCESS	- success
UTE_ERROR	- error
************************************************/
int CUT_Msg::DecodeAttachmentFileName(int iIndex, LPSTR lpszCharSet)
{
	if(m_ptrEncoding[m_nDefEncodingObj] != NULL)
		return m_ptrEncoding[m_nDefEncodingObj]->DecodeAttachmentFileName(iIndex, lpszCharSet);
	else
		return UTE_ERROR;
}

/***********************************************
GetCharSetStruct
If strCharSet = NULL (default) returns a pointer to
the CUT_CharSet associated with the text of the message.
Otherwise, attempts to find a suitable CUT_CharSet for the 
charset string passed in.
PARAM:
lpszCharSet		- (in) character set or NULL
RETURN:
CUT_CharSet * 
************************************************/
CUT_CharSetStruct * CUT_Msg::GetCharSetStruct(LPCSTR lpszCharSet) 
{
	CUT_CharSetStruct * retval = m_pCharSet;
	if(lpszCharSet != NULL) {
		// find in charset array...
		for( int index = 0; index < MAX_CHARSETS; ++ index) {
			if(stricmp(lpszCharSet, utCharSets[index].strCharSet) == 0) {
				retval = &utCharSets[index];
				break;
			}
		}
	}
	return retval;
}

/***********************************************
GetDefaultCharSet

  Returns the default charset to be used for encoding.

  May be set by user or during loading of a message.

  PARAM:
	lpszCharSet		- (in) character set or NULL
  RETURN:
	UTE_SUCCESS	
	UTE_NULL_PARAM
	UTE_BUFFER_TOO_SHORT
************************************************/
int CUT_Msg::GetDefaultCharSet(LPSTR buf, size_t size) {

	int retval = UTE_SUCCESS;

	if(buf == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {
		if(size < strlen(m_lpszDefaultCharSet)) {
			retval = UTE_BUFFER_TOO_SHORT;
		}
		else {
			strcpy(buf, m_lpszDefaultCharSet);
		}
	}
	return retval;
}
	
#if defined _UNICODE
int CUT_Msg::GetDefaultCharSet(LPWSTR buf, size_t size) {
	
	int retval = UTE_SUCCESS;

	if(buf == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {
		if(size < strlen(m_lpszDefaultCharSet)) {
			retval = UTE_BUFFER_TOO_SHORT;
		}
		else {
			char * buffA = (char*) alloca(size+1);
			*buffA = '\0';
			retval = GetDefaultCharSet(buf, size);
			if(retval == UTE_SUCCESS) {
				CUT_Str::cvtcpy(buf, size, buffA);
			}
		}
	}

	return retval;
}
#endif 


// v4.2 added default charset for new messages - internally, held as ansi
/***********************************************
SetDefaultCharSet

  The default charset is set in the constructor, 
  and may also be set in loading a message. 

  This is the charset that will be used for encoding the 
  text part of a MIME format message or a header if no
  charset is specified in the Encode calls.

  PARAM:
	charset		- string to set
  RETURN:
	UTE_SUCCESS	
	UTE_NULL_PARAM			- charset is null
	UTE_CHARSET_TOO_BIG		- limited to 40 chars as per RFC 1700
************************************************/
int CUT_Msg::SetDefaultCharSet(LPCSTR charset) {

	int retval = UTE_SUCCESS;

	if(charset == NULL) {
		retval = UTE_NULL_PARAM;
	}
	else {
		if(strlen(charset) > MAX_CHARSET_SIZE) {
			retval = UTE_CHARSET_TOO_BIG;
		}
		else {
			strcpy(m_lpszDefaultCharSet,charset);
		}
	}

	return retval;
}

#if defined _UNICODE
int CUT_Msg::SetDefaultCharSet(LPCWSTR charset) {
	return SetDefaultCharSet(AC(charset));
}
#endif

#pragma warning ( pop )