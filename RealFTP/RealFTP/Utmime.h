// =================================================================
//  class: CUT_MimeEncode
//  File:  utmime.h
//  
//  Purpose:
//
//  MIME Decoding/Encoding class declaration.
//  Multipurpose Internet Mail Extension (MIME)
//  
// RFC  2045, 2046
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

#ifndef UTMIME_INCLUDE_H
#define  UTMIME_INCLUDE_H

#include <string.h>
#include <stdio.h>

// header file for base class
#include "UTEncode.h"

// if we are only using this class as stand alone we need to define the CUT_SUCCESS 
// which is defined in CUT_WSClient classheader of Ultimate TCP/IP 
#ifndef CUT_SUCCESS
    #define CUT_SUCCESS             0
#endif
#ifndef CUT_NOT_IMPLEMENTED         
#define CUT_NOT_IMPLEMENTED         -1 // added FEB 1999
#endif

// boundary styles
#define BS_INTERMEDIATE             0       //intermediate boundary
#define BS_ENDOFFILE                1       //final boundary


    // This structure contains information about each attachment
    // in the MIME package.  Each call to AddFile addes a new
    // node to the linked list.
    //
    // When the list is fully prepared, a call to EncodeToFile()
    // will request the library to traverse this list of attachments
    // and to prepare each attachment in the method indicated
    // by the associated encodeType member.
	typedef struct   _MIMEATTACHMENTLISTITEM {
		CUT_DataSource	*ptrDataSource;		// Pointer to the attachment datasource
        LPSTR			lpszName;			// attachment item file name 
        LPSTR			lpszContentType;	// attachment item file name such as text/plain application, image and so on
        LPSTR			lpszCharSet;		// charset portion of the content type
        LPSTR			lpszAttachType;		// attachment type
        int				nEncodeType;		// encoding type
		long			lSize;				// Attachment size
        long			lOffset;			// used when decoding only.
        long			lContentLength;		// used when decoding only.
        CUT_StringList	*ptrHeaders;		// headers for this item including custom headers
        _MIMEATTACHMENTLISTITEM *next;
    } MIMEATTACHMENTLISTITEM;

	// Mime encode types
	enum EncodeTypes {
		CUT_MIME_7BIT,
		CUT_MIME_BASE64,
		CUT_MIME_QUOTEDPRINTABLE,
		CUT_MIME_8BIT,
		CUT_MIME_BINARY,
		CUT_MIME_BINHEX40,			// not supported yet ( added to provide a framework for future implementation )
		CUT_MIME_MULTIPART,

		CUT_MIME_MAX
		};
	enum ContentType {
		// The content type can be one of Two kind 
		//discrete-type  or  composite-type
		// The discrete-type is one of the following 
		CUT_MIME_TEXT, // sub type is plain or html (The default is text Plain)
		CUT_MIME_IMAGE, 
		CUT_MIME_AUDIO,
		CUT_MIME_VIDEO,
		CUT_MIME_APPLICATION,
		//composite-type can be one of three 
		CUT_MIME_MESSAGE,
		CUT_MIME_MULTIPART_CONTENT,
		// EXTENSION_TOKEN can be one of two 
		CUT_MIME_EXTENSION_TOKEN_ITEF, 
		CUT_MIME_EXTENSION_TOKEN_X,
		CUT_MIME_UNRECOGNIZED

	};
	enum CompositeSubType { // 
		CUT_MIME_NA, // not avaialable
		CUT_MIME_TEXT_HTML,
		CUT_MIME_TEXT_PLAIN,
		CUT_MIME_MULTIPART_MIXED,
		CUT_MIME_MULTIPART_ALTERNATIVE,
		CUT_MIME_MULTIPART_PARALLEL,
		CUT_MIME_MULTIPART_DIGEST, // the default is message/RFC822
		CUT_MIME_MESSAGE_RFC822, 
		CUT_MIME_MESSAGE_PARTIAL, // partial rfc 822 message 
		CUT_MIME_MESSAGE_EXTERNAL_BODY, //  for specifying large bodies by refrences of an external data source 
		CUT_MIME_MULTIPART_RELATED

	};
	typedef struct __GlobalContentType {
		ContentType type;
		CompositeSubType sub;
	}GlobalContentType;



class CUT_ComplexMimeNode;  // forward declaration 


//=================================================================
//  class: CUT_MimeEncode
class CUT_MimeEncode : public CUT_Encode {

	friend CUT_ComplexMimeNode; // we want to be able to access this class elements
public:
    CUT_MimeEncode();
    virtual ~CUT_MimeEncode();

protected:
	CUT_DataSource			*m_ptrDecodeDataSource;	// Pointer to the decode data source
    MIMEATTACHMENTLISTITEM  *m_ptrAttachList;		//info for encoding/decoding files
    MIMEATTACHMENTLISTITEM  *m_ptrCurrentAttach;	// current attachment item 
													// used by AddFileHeaderTag to know which one is the last added attachment
	MIMEATTACHMENTLISTITEM* m_ptrHtmlBodyAttachment; // The attachment that holds the html body
	MIMEATTACHMENTLISTITEM* m_ptrTextBodyAttachment; // The attachment that holds the plain text

    CUT_StringList			m_listGlobalHeader;		// list of global headers for encoding
    int						m_nNumberOfAttachmentAdded;		// number of attachments
	char					m_szBase64Table[128];   // buffer to hold the base64 character table
	char					m_szBinHexTable[128];	// buffer to hold the binhex character table
	char					m_szBoundaryString[100];// attachment item boundr string
	char					m_szSubBoundaryString[100];
	char					m_szMainBoundry[100];// attachment item boundry string for the whole message in case it is a MultiPartMixed


	GlobalContentType		m_gctMsgContenType;   // to determine if this message is a basic mime or not


	// The number of attachment found when decoding 
	int						m_nNumberOfAttachmentFound;

protected:

	int GetEntityInformation(int filePos = 0, int index = 0);


	int LocateMultiPartEntities( char *boundaryString, int startingPos = 0);

	int GetMultiPartBoundryString ( int startPos ,int EndPosition, char *boundaryString);

	BOOL DecodeGetGlobalContentType(long startingPos,GlobalContentType &gctMsgContenType  );
    // parse file for attachment information 
    void DecodeGetAttachmentInfo( MIMEATTACHMENTLISTITEM * workItem, long filePos);

    // Enecode a byte to a 6bit base64 byte
    void EncodeBase64Bytes(unsigned char *in,unsigned char *out, int count);

    // Decode a byte from 6bit base64 byte
    int DecodeBase64Bytes(unsigned char *in,unsigned char *out);

    // Get the base64 representation of the charecter from the table
    UCHAR FindBase64Val(char character);

    // Encoding functions
    int		Encode7bit(MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest);
    int		EncodeBase64(MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest);
    int		EncodeQuotedPrintable(MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest);
    int		Encode8bit(MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest);
    int		EncodeBinary(MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest);
    int		EncodeBinHex(MIMEATTACHMENTLISTITEM   *item , CUT_DataSource & dest);

	// Decoding functions
    int		Decode7bit(MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest);
    int		DecodeBase64(MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest);
    int		DecodeQuotedPrintable(MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest);
    int		Decode8bit(MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest);
    int		DecodeBinary(MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest);
    int		DecodeBinHex(MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest);


    // return the decimal representation of a hex string into a decimal value
    long HexToDec(char *hexval, int numDigits);

    // genrate a new boundry string for  a new attachment item or File
    int GetNewBoundaryString(LPSTR boundary, int len);

    // add the boundry string to the output file
    int WriteBoundaryString(CUT_DataSource & dest, LPCSTR boundaryString, int style);


    // internal functions for adding and maintaining the 
    // list of attachments in the file being decoded.
    void AddDecodeFileInfo (LPCSTR fileName, LPCSTR contentType,
                       int encodeType, long fileOffset, long contentLength);
    // add a new item to to attachment list
    void AddDecodeFileInfo (MIMEATTACHMENTLISTITEM *newitem);

    // add a header that is common to all attachment items 
    int EncodeAddGlobalHeader(LPCSTR tag);

    // empty the list of global headers 
    void EmptyGlobalHeaders();

    // Retrieve a list item
    int GetDecodeListItem(int index, MIMEATTACHMENTLISTITEM **work);

    // Get header data from the data source
    LPSTR GetMessageHeader(CUT_DataSource *ds, long &lPosition);

	virtual int DecodeAlternatives(int alternativeStart,int alternativeEnd,int  alterIndex);

	virtual int AddHtmlBody(LPCSTR lpszHtmlBody);

	virtual int RemoveHtmlBody();

public:
	//*************************************************
	// Generic functions
	//*************************************************

	// Gets the name of the encoding/decoding algorithm
	virtual LPCSTR	GetName() const;

	// Gets number of items in the list of encoding attachments
	virtual int		GetAttachmentNumber() const;

	// Empty the list of attachments
	virtual void	EmptyAttachmentList();

	//*************************************************
	// Encoding functions
	//*************************************************

	// Add data source to the list of encoding attachments
	virtual int		AddAttachment(	CUT_DataSource & source, 
									LPCSTR name = NULL,  
									CUT_StringList *params = NULL,
									BOOL bAddToTop = FALSE);


	// Add custom headers to an individual attachments.
	// Affect only the last added attachment. Can be 
	// called several times to form a list of headers
	virtual int		AddFileHeaderTag(LPCSTR tag);

	// Get the number of global headers that need to be added 
	// to the message global headers
	virtual int		GetGlobalHeadersNumber() const;

	// Get global header by index
	virtual LPCSTR	GetGlobalHeader(int index) const;

	// Encode list of attachments into the data source
	virtual int		Encode(CUT_DataSource & msg_body, CUT_DataSource & dest, BOOL append = FALSE);

	virtual int EncodeAttachmentFileName(int iIndex, enumEncodingType EncodingType, LPCSTR lpszCharSet);
	
	virtual int DecodeAttachmentFileName(int iIndex, LPSTR lpszCharSet);

	//*************************************************
	// Decoding functions
	//*************************************************

	// Sets a data source for decoding
	virtual int		SetDecodeSource(CUT_DataSource & source);

	// Gets a meessage body text from the encoded message
	virtual int		GetMessageBodyText(CUT_DataSource & dest, BOOL append = FALSE);

	// Gets the size of all or specified attachment
	virtual long	GetAttachmentSize(int index = -1) const;

	// Gets attachment info
	virtual int		GetAttachmentInfo(	int index, 
										LPSTR name, 
										int namesize,
										LPSTR type,
										int typesize,
										CUT_StringList *params = NULL) const;

	// Decode attchment by index into data source or file
	virtual int		Decode(int index, CUT_DataSource & dest, BOOL append = FALSE);

protected:

    // Check if decoding is possible for the specified decode source
	virtual BOOL	CanDecode();

};

// for Embeded attachments 

class CUT_ComplexMimeNode  
{
public:
	int DecodeComplex(CUT_MimeEncode &mime,
		int alternativeStart,
		int alternativeEnd, 
		int alterIndex);


	CUT_ComplexMimeNode();
	virtual ~CUT_ComplexMimeNode();

};



#endif
//=================================================================
//  End of class: CUT_MimeEncode
