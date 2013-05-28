// =================================================================
//  class: CUT_UUEncode
//  File:  utuu.h
//
//  Purpose:
//       Derived from UTEncode to implement UUEncoding
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
#ifndef UTUU_INCLUDE_H
#define UTUU_INCLUDE_H


#include "UTEncode.h"
#include "UTErr.h"

/**********************************
***********************************/


#ifndef ENC
    #define ENC(c)  ((c) ? ((c) & 077) + ' ': '`')
#endif

#ifndef DEC
    #define DEC(c)  (((c) - ' ') & 077)
#endif


#define	BUFFER_SIZE		4096

    typedef struct _ATTACHMENTLISTITEM {
		CUT_DataSource	*ptrDataSource;				// Pointer to the attachment datasource
        char			szFileName[MAX_PATH+1];		// Name of the attachment
		long			nSize;
        long			nFileAccessMode;	
        long			nOffset;					
        long			nContentLength;			
        _ATTACHMENTLISTITEM *next;
    } ATTACHMENTLISTITEM;
    

#include "UTEncode.h"

class CUT_UUEncode : public CUT_Encode {

    public:
        CUT_UUEncode();
        virtual ~CUT_UUEncode();
    
    protected:
		CUT_DataSource		*m_DecodeDataSource;	// Pointer to the decode data source
		long				m_nActualMsgSize;		// Size of the message body

        ATTACHMENTLISTITEM  *m_decodeFileInfo;		// Pointer to the attachments list
		ATTACHMENTLISTITEM  *m_currentAttachment;	// Pointer to the current attachment

		int					m_numberAttachments;	// Number of the attachments

		long				m_lMsgBodyTextStart;	// Start of the message body text
		long				m_lMsgBodyTextEnd;		// End of the message body text

        void		EncodeBytes(char *in, char *out);
        void		DecodeBytes(char *in, char *out, int count);
        void		AddDecodeFileInfo (ATTACHMENTLISTITEM *newitem);


		//*************************************************
		// Generic encoding/decoding functions
		//*************************************************
public:

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

		// Not implemented for UUEncode
		virtual int EncodeAttachmentFileName(int iIndex, enumEncodingType EncodingType, LPCSTR lpszCharSet)
		{
			UNREFERENCED_PARAMETER(lpszCharSet);
			UNREFERENCED_PARAMETER(EncodingType);
			UNREFERENCED_PARAMETER(iIndex);
			return UTE_ERROR;
		}

		// Not implemented for UUEncode
		virtual int DecodeAttachmentFileName(int iIndex, LPSTR lpszCharSet)
		{
			UNREFERENCED_PARAMETER(iIndex);
			UNREFERENCED_PARAMETER(lpszCharSet);
			return UTE_ERROR;
		}

		// Not implemented for UUEncode
		virtual int AddHtmlBody(LPCSTR lpszHtmlBody)
		{
			UNREFERENCED_PARAMETER(lpszHtmlBody);
			return UTE_ERROR;
		}

		// Not implemented for UUEncode
		virtual int RemoveHtmlBody()
		{
			return UTE_ERROR;
		}

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


#endif
