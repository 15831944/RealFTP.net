// =================================================================
//  class: CUT_Encode
//  File:  UTEncode.h
//  
//  Purpose:
//  
//      Declaration of abstract base class for encoding/decoding.
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

#ifndef UTENCODE_H
#define UTENCODE_H

#include "UTStrLst.h"
#include "UTMessage.h"
#include "ut_strop.h"

// =================================================================
//  CUT_Encode class
//
//  Abstract base class for encoding/decoding.
// =================================================================
class CUT_Encode {
    public:
        CUT_Encode() {
		m_bTextBody = FALSE;
		m_bHTMLBody = FALSE;
		m_pMsg = NULL;
		*m_lpszCharSet = '\0';
		}
        virtual ~CUT_Encode() {}
	void	SetHTMLBody( BOOL flag =FALSE){
		if (flag == TRUE)
			m_bTextBody = FALSE;
		m_bHTMLBody = flag;
	}


    //*************************************************
    // Generic functions
    //*************************************************

    // Gets the name of the encoding/decoding algorithm
    virtual LPCSTR  GetName() const = 0;

    // Gets number of items in the list of encoding attachments
    virtual int     GetAttachmentNumber() const = 0;

    // Empty the list of attachments
    virtual void    EmptyAttachmentList() = 0;


    //*************************************************
    // Encoding functions
    //*************************************************

    // Add data source to the list of encoding attachments
    virtual int     AddAttachment(  CUT_DataSource & source, 
                                    LPCSTR name = NULL,  
                                    CUT_StringList *params = NULL,
									BOOL bAddToTop = FALSE) = 0;

    // Add file to the list of encoding attachments
    int AddAttachment(LPCSTR filename, CUT_StringList *params = NULL)
    { 
        CUT_FileDataSource ds(WC(filename)); 
        return AddAttachment(ds, filename, params); 
    }

#if defined _UNICODE
	int AddAttachment(LPCWSTR filename, CUT_StringList *params = NULL) 
	{
        CUT_FileDataSource ds(filename); 
        return AddAttachment(ds, AC(filename), params); 
	}
#endif

	// Adds an html body as a nameless attachment
	virtual int AddHtmlBody(LPCSTR lpszHtmlBody) = 0;

	// Removes the HTML body nameless attachment 
	virtual int RemoveHtmlBody() = 0;

    // Add custom headers to an individual attachments.
    // Affect only the last added attachment. Can be 
    // called several times to form a list of headers
    virtual int     AddFileHeaderTag(LPCSTR tag) = 0;

    // Get the number of global headers that need to be added 
    // to the message global headers
    virtual int     GetGlobalHeadersNumber() const = 0; 

    // Get global header by index
    virtual LPCSTR  GetGlobalHeader(int index) const = 0;

    // Encode list of attachments into the data source
    virtual int     Encode(CUT_DataSource & msg_body, CUT_DataSource & dest, BOOL append = FALSE) = 0;

    // Encode list of attachments into the file
    int Encode(LPSTR msg_body, LPCTSTR filename) 
    {   
        if(msg_body != NULL && filename != NULL) {
            CUT_BufferDataSource dsMsg(msg_body, strlen(msg_body));
            CUT_FileDataSource ds(filename); 
            return Encode(dsMsg, ds); 
        }
        return UTE_ERROR;
    }
        
	// Encode the file name of the attachment
	virtual int EncodeAttachmentFileName(int iIndex, enumEncodingType EncodingType, LPCSTR lpszCharSet) = 0;

	// Decode the file name of the attachment
	virtual int DecodeAttachmentFileName(int iIndex, LPSTR lpszCharSet) = 0;

    
    //*************************************************
    // Decoding functions
    //*************************************************

    // Sets a data source for decoding
    virtual int     SetDecodeSource(CUT_DataSource & source) = 0;

    // Sets a filename for decoding
    int SetDecodeSource(LPCTSTR filename)
    {   
        CUT_FileDataSource ds(filename); 
        return SetDecodeSource(ds); 
    }

    // Gets a meessage body text from the encoded message
    virtual int     GetMessageBodyText(CUT_DataSource & dest, BOOL append = FALSE) = 0;

    // Gets a meessage body text from the encoded message
    int GetMessageBodyText(LPSTR buffer, long size) 
    {
        if(buffer != NULL) {
            CUT_BufferDataSource ds(buffer, size); 
            return GetMessageBodyText(ds);
        }
        else
            return UTE_ERROR;
    }


    // Gets the size of all or specified attachment
    virtual long    GetAttachmentSize(int index = -1) const = 0;

    // Gets attachment info
    virtual int     GetAttachmentInfo(  int index, 
                                        LPSTR name, 
                                        int namesize,
                                        LPSTR type,
                                        int typesize,
                                        CUT_StringList *params = NULL) const = 0;

    // Decode attachment by index into data source or file
    virtual int     Decode(int index, CUT_DataSource & dest, BOOL append = FALSE) = 0;
    int Decode(int index, LPCTSTR filename) 
    {   
        CUT_FileDataSource ds(filename); 
        return Decode(index, ds); 
    }

	BOOL	m_bTextBody;			// TRUE if plain text body is present
	BOOL	m_bHTMLBody;			// set the body of the message to HTML format
	
	char	m_lpszCharSet[MAX_CHARSET_SIZE+1];		// TD v4.2 currently used only by mime

	CUT_Msg*		m_pMsg;					// A back pointer to the container object

protected:

    // Check if decoding is possible for the specified decode source
    virtual BOOL    CanDecode() = 0;
};


#endif



