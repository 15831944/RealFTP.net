// =================================================================
//  class: CUT_Msg
//  File:  UTMessage.h
//  
//  Purpose:
//	
//		Encapsulates RFC 822 message format
//	
// =================================================================
// Ultimate TCP/IP v4.2
// This software along with its related components, documentation and files ("The Libraries")
// is © 1994-2007 The Code Project (1612916 Ontario Limited) and use of The Libraries is
// governed by a software license agreement ("Agreement").  Copies of the Agreement are
// available at The Code Project (www.codeproject.com), as part of the package you downloaded
// to obtain this file, or directly from our office.  For a copy of the license governing
// this software, you may contact us at legalaffairs@codeproject.com, or by calling 416-849-8900.
// =================================================================


#ifndef UTMSG_H
#define UTMSG_H

// Header fields IDs enumeration
enum HeaderFieldID {
	UTM_ALL_FIELDS,					// All types of fields
	UTM_MESSAGE_ID,					// Message Id ......................(RFC 822, 1036)
	UTM_TO,							// To ..............................(RFC 822, 1123)
	UTM_CC,							// CC ..............................(RFC 822, 1123)
	UTM_BCC,						// BCC .............................(RFC 822, 1123)
	UTM_FROM,						// From ............................(RFC 822, 1036, 1123)
	UTM_SUBJECT,					// Subject .........................(RFC 822, 1036)
	UTM_RECEIVED,					// Trace of MTAs ...................(RFC 822, 1123)
	UTM_IN_REPLY_TO,				// Reference to replied to message .(RFC 822)
	UTM_KEYWORDS,					// Search keys for DB retrieval ....(RFC 822, 1036)
	UTM_COMMENTS,					// Comments on this message ........(RFC 822)
	UTM_DATE,						// Date ............................(RFC 822, 1036, 1123)
	UTM_REPLY_TO,					// Reply-To ........................(RFC 822, 1036)
	UTM_REFERENCES,					// References ......................(RFC 822, 1036)
	// v4.2 could add RFC Resent-xxx headers.
	UTM_NEWSGROUPS,					// NewsGroups ......................(RFC 1036)
	UTM_XREF,						// XRef ............................(RFC 1036)
	UTM_XMAILER,					// X-Mailer     (non-standard)
	UTM_XNEWS_READER,				// X-Newsreader (non-standard)
	UTM_MIME_VERSION,				// MIME version ....................(RFC 1521)
	UTM_THREAD_TOPIC,				// Proprietary - included for charset decoding
	UTM_CONTENT_TYPE,				// Content-Type .......(RFC 1049, 1123, 1521, 1766)

	UTM_CUSTOM_FIELD,				// Custom field
	UTM_MAX_FIELD
};

#ifndef __midl

#include "UTErr.h"
#include "UTStrLst.h"
#include "UTFile.h"
#include "UTDataSource.h"
#include "UT_HeaderEncoding.h"
#include "utcharset.h"

#define		MAX_LINE_LENGTH			1000		// Size of the buffer for lines of message
#define		MAX_FIELD_NAME_LENGTH	128			// Size of the buffer for field name
												// v4.2 increased (was 30 - not enough for some custom headers)
#define		MAX_ENCODING_OBJECTS	5			// Maximum number od encoding objects

class		CUT_Encode;
class		CUT_DataSource;

// =================================================================
//	CUT_Msg class
//
//	Helper class which encapsulates RFC 822 message format
// =================================================================
class CUT_Msg {

	friend class CUT_SMTPClient;

	// Header field structure
	typedef struct UT_HeaderField {
		LPSTR			lpszFieldName;	// Custom field name
		LPSTR			lpszFieldData;	// Data of the field
		int				nFieldID;		// ID of the field 

		UT_HeaderField	*pNext;				// Pointer to the next structure
	} UT_HeaderField;

public:
	CUT_Msg();
	virtual ~CUT_Msg();

private:
	CUT_Msg(const CUT_Msg& );				// Copy constructor not implemented
	CUT_Msg& operator=(const CUT_Msg&);		// Assigment operator not implemented

	UT_HeaderField	*m_listUT_HeaderFields;	// Pointer to the list of header fields
	LPSTR			m_lpszMessageBody;		// Message body
	LPSTR			m_lpszHtmlMessageBody;  // HTML Message body
	char			m_lpszEmptyString[1];	// Just a empty string
	unsigned int	m_nMaxMessageBodySize;	// Maximum message body size

	CUT_Encode		*m_ptrEncoding[MAX_ENCODING_OBJECTS];		// Default encoding object		
	int				m_nEncodingObjNumber;	// Number od Encoding objects
	int				m_nDefEncodingObj;		// Default encoding object
	int				m_nDecodingObj;			// Last decoding object used
	BOOL			m_bDecoding;			// Set to TRUE if last operation was Decoding			
	BOOL			m_bHtmlBody;			// Set the body of message to html format false by default

	// Helper function to copy strings checking the memory allocation
	void	MsgCopyString(LPSTR &dest, LPCSTR source);

	CUT_CharSetStruct	*m_pCharSet;			// v4.2 maintain a charset struct for the message if one can be
												// determined from the content type.

	// maintain a default charset for encoding new messages	
	char			m_lpszDefaultCharSet[MAX_CHARSET_SIZE+1];
	char			m_lpszTextCharSet[MAX_CHARSET_SIZE+1];
	char			m_lpszHtmlCharSet[MAX_CHARSET_SIZE+1];
	char			m_lpszHeaderCharSet[MAX_CHARSET_SIZE+1];	// TD v4.2 is one enough?
	

public:
	// **********************************
	// Attachments manipulation functions
	// **********************************

	// Adds filename or to the attachments list
	int		AddAttachment(LPCSTR filename, CUT_StringList *params = NULL);
#if defined _UNICODE
	int		AddAttachment(LPCWSTR filename, CUT_StringList *params = NULL);
#endif
	// Adds datasource to the attachments list
	int		AddAttachment(CUT_DataSource & source, LPCSTR name = NULL, CUT_StringList *params = NULL);
#if defined _UNICODE
	int		AddAttachment(CUT_DataSource & source, LPCWSTR name = NULL, CUT_StringList *params = NULL);
#endif
	// Gets the number of the attachments
	int		GetAttachmentNumber();

	// Get the number of global headers that need to be added 
	// to the message global headers
	int		GetGlobalHeadersNumber();	

	// Get global header by index
	LPCSTR	GetGlobalHeader(int index);

	// v4.2 Refactored - added interface with LPSTR and LPWSTR params
	int GetGlobalHeader(LPSTR header, size_t maxSize, int index, size_t *size);
#if defined _UNICODE
	int GetGlobalHeader(LPWSTR header, size_t maxSize, int index, size_t *size);
#endif

	// Delete all attachments 
	void	EmptyAttachmentList();

	// Gets the size of all or specified attachment
	long	GetAttachmentSize(int index = -1);

	// Gets attachment info
	int		GetAttachmentInfo(		int index, 
		LPSTR name, 
		int namesize,
		LPSTR type,
		int typesize,
		CUT_StringList *params = NULL);
#if defined _UNICODE
	int		GetAttachmentInfo(		int index, 
		LPWSTR name, 
		int namesize,
		LPWSTR type,
		int typesize,
		CUT_TStringList *params = NULL);
#endif

	// Encode all attchments into data source or file
	int		Encode(CUT_DataSource & msg_body, CUT_DataSource & dest);
	int		Encode(LPSTR message, LPCTSTR filename);

	// Decode attchment by index into data source or file
	int		Decode(int index, CUT_DataSource & dest);
	int		Decode(int index, LPCTSTR filename);

	// Add custom headers to an individual attachment
	// Affect the last added attachment. Can be called
	// several times to form a list of headers.
	int		AddAttachmentHeaderTag(LPCSTR tag);
#if defined _UNICODE
	int		AddAttachmentHeaderTag(LPCWSTR tag);
#endif

	// Sets default encoding object
	int		SetDefEncoding(LPCSTR name);
#if defined _UNICODE
	int		SetDefEncoding(LPCWSTR name);
#endif

	// Adds encoding object
	int		AddEncodingObject(CUT_Encode &encoding);


	// **********************************


	// Gets field name by field ID
	LPCSTR	GetFieldName(HeaderFieldID nID);
	// v4.2 Refactored - added interface with LPSTR and LPWSTR params
	int GetFieldName(LPSTR name, size_t maxSize, HeaderFieldID nID, size_t *size);
#if defined _UNICODE
	int GetFieldName(LPWSTR name, size_t maxSize, HeaderFieldID nID, size_t *size);
#endif
	// Gets custom field name by index
	LPCSTR	GetCustomFieldName(long lIndex);
	// v4.2 Refactored - added interface with LPSTR and LPWSTR params
	int GetCustomFieldName(LPSTR name, size_t maxSize, long lIndex, size_t *size);
#if defined _UNICODE
	int GetCustomFieldName(LPWSTR name, size_t maxSize, long lIndex, size_t *size);
#endif

	// Set/Get Maximum message body size
	void			SetMaxMessageBodySize(unsigned int maxsize);
	unsigned int	GetMaxMessageBodySize();

	// Set/Get Message Body
	BOOL	SetMessageBody(LPCSTR body);
	BOOL	SetHtmlMessageBody(LPCSTR body);
#if defined _UNICODE
	BOOL	SetMessageBody(LPCWSTR body);
	BOOL	SetHtmlMessageBody(LPCWSTR body);
#endif

	size_t GetMessageBodyLength();
	size_t GetHtmlMessageBodyLength();

	LPCSTR	GetMessageBody();
	// v4.2 Refactored - added interface with LPSTR and LPWSTR params
	int GetMessageBody(LPSTR messageText, size_t maxSize, size_t *size);
#if defined _UNICODE
	int GetMessageBody(LPWSTR messageText, size_t maxSize, size_t *size);
#endif

	LPCSTR GetHtmlMessageBody();
	// v4.2 Refactored - added interface with LPSTR and LPWSTR params
	int GetHtmlMessageBody(LPSTR messageText, size_t maxSize, size_t *size);
#if defined _UNICODE
	int GetHtmlMessageBody(LPWSTR messageText, size_t maxSize, size_t *size);
#endif

	BOOL IsHtmlMessageBodyPresent();

	// *** Message Header list functions ***

	// Add Header to the list
	BOOL	AddHeaderField(LPCSTR lpszData, HeaderFieldID nID, LPCSTR lpszHeader = NULL);
#if defined _UNICODE
	BOOL	AddHeaderField(LPCWSTR lpszData, HeaderFieldID nID, LPCWSTR lpszHeader = NULL);
#endif

	// Clear all header or only headers of specified ID
	BOOL	ClearHeader(HeaderFieldID nID = UTM_ALL_FIELDS, LPCSTR lpszCustomName = NULL);
	// v4.2 removed default params in overload to avoid ambiguity
#if defined _UNICODE
	BOOL	ClearHeader(HeaderFieldID nID, LPCWSTR lpszCustomName);
#endif
	// Get count of all header or only headers of specified ID 
	long	GetHeaderCount(HeaderFieldID nID = UTM_ALL_FIELDS);

	// Get count of all header or only headers of specified name 
	long	GetHeaderCount(LPCSTR name);
#if defined _UNICODE
	long	GetHeaderCount(LPCWSTR name);
#endif

	// Get Header data by index and it's name. 
	LPCSTR	GetHeaderByName(LPCSTR name, long lIndex);
#if defined _UNICODE
	// v4.2 Partial - overload still returns LPCSTR. Note alternate below.
	LPCSTR	GetHeaderByName(LPCWSTR name, long lIndex);
#endif

	// Get all Header data by it's name in the buffer
	BOOL	GetHeaderByName(LPCSTR name, LPSTR buff, int size);
#if defined _UNICODE
	BOOL	GetHeaderByName(LPCWSTR name, LPWSTR buff, int size);
#endif

	// Get Header data by index and it's ID. 
	LPCSTR	GetHeaderByType(HeaderFieldID nID, long lIndex);
	// v4.2 Refactored - added interface with LPSTR and LPWSTR params
	int GetHeaderByType(LPSTR headerText, size_t maxSize, HeaderFieldID nID, long lIndex, size_t *size);
#if defined _UNICODE
	int GetHeaderByType(LPWSTR headerText, size_t maxSize, HeaderFieldID nID, long lIndex, size_t *size);
#endif

	// Get all Header data by it's ID in the buffer
	BOOL	GetHeaderByType(HeaderFieldID nID, LPSTR buff, int size);
#if defined _UNICODE
	BOOL	GetHeaderByType(HeaderFieldID nID, LPWSTR buff, int size);
#endif

	// Saves message into the DataSource or file
	int		SaveMessage(CUT_DataSource &data);

	int		SaveMessage(LPCTSTR filename) { 

		// VC complains with warning C4239 if we use the result of a construct  right away
		// so let's create a dummy one and pass it
		CUT_FileDataSource ds(filename);
		return SaveMessage(ds);  }


	// Loads message from the DataSource or file
	int		LoadMessage(CUT_DataSource &data);

	int		LoadMessage(LPCTSTR filename) { 

		// VC complains with warning C4239 if we use the result of a construct  right away
		// so let's create a dummy one and pass it
		CUT_FileDataSource ds(filename);
		return LoadMessage(ds); 
	}
	void SetBodyAsHTML(BOOL flag = FALSE);
	
	// v4.2 charsets left as ANSI char
	// v4.2 if charset param = NULL - will use default charset
	int EncodeHeader(HeaderFieldID nHeaderID, enumEncodingType EncodingType, LPCSTR lpszCharSet = NULL);

	int DecodeHeader(HeaderFieldID nHeaderID, LPSTR lpszCharSet);

	// v4.2 added for access to custom headers by name
	int DecodeHeader(LPCSTR lpszName, LPSTR lpszCharSet);
#if defined _UNICODE
	int DecodeHeader(LPCWSTR lpszName, LPSTR lpszCharSet);
#endif

	// v4.2 added default charset for new messages - internally, held as ansi
	int SetDefaultCharSet(LPCSTR charset);
#if defined _UNICODE
	int SetDefaultCharSet(LPCWSTR charset);
#endif

	// simplify for ANSI builds
	LPCSTR GetDefaultCharSet() { return m_lpszDefaultCharSet;}
	int GetDefaultCharSet(LPSTR buf, size_t size);
#if defined _UNICODE
	int GetDefaultCharSet(LPWSTR buf, size_t size);
#endif 


	int EncodeAttachmentFileName(int iIndex, enumEncodingType EncodingType, LPCSTR lpszCharSet);

	int DecodeAttachmentFileName(int iIndex, LPSTR lpszCharSet);

	CUT_CharSetStruct * GetCharSetStruct(LPCSTR lpszCharSet = NULL); 
}; 

#endif	//#ifndef __midl
#endif