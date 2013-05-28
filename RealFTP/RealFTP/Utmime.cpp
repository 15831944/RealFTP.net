// =================================================================
//  class: CUT_MimeEncode
//  File:  utmime.cpp
//
//  Purpose:
//
//  MIME Decoding/Encoding Class
//
//  Implementation of Multipurpose Internet Mail Extension (MIME)
//
//  RFC  2045, 2046
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

#include "utmime.h"

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)

/*===================================================================
WHY DO WE NEED MIME?

"Since its publication in 1982, RFC 822 has defined the standard
format of textual mail messages on the Internet.  Its success has
been such that the RFC 822 format has been adopted, wholly or
partially, well beyond the confines of the Internet and the Internet
SMTP transport defined by RFC 821.  As the format has seen wider use,
a number of limitations have proven increasingly restrictive for the
user community.
RFC 822 was intended to specify a format for text messages.  As such,
non-text messages, such as multimedia messages that might include
audio or images, are simply not mentioned.  Even in the case of text,
however, RFC 822 is inadequate for the needs of mail users whose
languages require the use of character sets richer than US-ASCII.
Since RFC 822 does not specify mechanisms for mail containing audio,
video, Asian language text, or even text in most European languages,
additional specifications are needed.
One of the notable limitations of RFC 821/822 based mail systems is
the fact that they limit the contents of electronic mail messages to
relatively short lines (e.g. 1000 characters or less [RFC-821]) of
7bit US-ASCII.  This forces users to convert any non-textual data
that they may wish to send into seven-bit bytes representable as
printable US-ASCII characters before invoking a local mail UA (User
Agent, a program with which human users send and receive mail).
Examples of such encodings currently used in the Internet include
pure hexadecimal, uuencode, the 3-in-4 base 64 scheme specified in
RFC 1421, the Andrew Toolkit Representation [ATK], and many others.
The limitations of RFC 822 mail become even more apparent as gateways
are designed to allow for the exchange of mail messages between RFC
822 hosts and X.400 hosts.  X.400 [X400] specifies mechanisms for the
inclusion of non-textual material within electronic mail messages.
The current standards for the mapping of X.400 messages to RFC 822
messages specify either that X.400 non-textual material must be
converted to (not encoded in) IA5Text format, or that they must be
discarded, notifying the RFC 822 user that discarding has occurred.
This is clearly undesirable, as information that a user may wish to
receive is lost.  Even though a user agent may not have the
capability of dealing with the non-textual material, the user might
have some mechanism external to the UA that can extract useful
information from the material.  Moreover, it does not allow for the
fact that the message may eventually be gatewayed back into an X.400
message handling system (i.e., the X.400 message is "tunneled"
through Internet mail), where the non-textual information would
definitely become useful again."
RFC 2045 Freed &n Borenstein.

MIME introduces several mechanisms that combine to solve most
of these problems without introducing any serious incompatibilities
with the existing world of RFC 822 mail.

=====================================================================*/

/***************************************************
CUT_MimeEncode() - default constructor:
****************************************************/
CUT_MimeEncode::CUT_MimeEncode() :
m_nNumberOfAttachmentAdded(0),
m_ptrAttachList(NULL),
m_ptrCurrentAttach(NULL),
m_ptrDecodeDataSource(NULL),
m_nNumberOfAttachmentFound(0),
m_ptrHtmlBodyAttachment(NULL),
m_ptrTextBodyAttachment(NULL)
{
	srand(GetTickCount());

	m_szMainBoundry[0] = NULL;

	strcpy(m_szBase64Table,"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");
	strcpy(m_szBinHexTable,"!\"#$%&'()*+,- 012345689@ABCDEFGHIJKLMNPQRSTUVXYZ[`abcdefhijklmpqr");

	m_gctMsgContenType.type = CUT_MIME_UNRECOGNIZED;
	m_gctMsgContenType.sub  = CUT_MIME_NA;

	GetNewBoundaryString(m_szBoundaryString, min(sizeof(m_szBoundaryString)-1, 30));
	GetNewBoundaryString(m_szSubBoundaryString, min(sizeof(m_szSubBoundaryString)-1, 30));
}
/***************************************************
~CUT_MimeEncode() - default destructor
****************************************************/
CUT_MimeEncode::~CUT_MimeEncode()
{
	EmptyAttachmentList();

	EmptyGlobalHeaders();

	if(m_ptrDecodeDataSource)
		delete m_ptrDecodeDataSource;
}

/***************************************************
EmptyGlobalHeaders()
deletes all elements in a custom header linked list.
PARAM
NONE
RETURN
VOID    
****************************************************/
void CUT_MimeEncode::EmptyGlobalHeaders(){
	m_listGlobalHeader.ClearList();
}

/***************************************************
GetNewBoundaryString()
Generates a unique string suitable for use as a
boundary marker in a MIME message.  MIME boundary
markers must be less than 70 characters in length, and
must not appear in the body of the message itself. 
The boundary delimiter line is then defined as a line
consisting entirely of two hyphen characters ("-", decimal value 45)
followed by the boundary parameter value from the Content-Type header
field, optional linear whitespace, and a terminating CRLF
PARAM
LPSTR  boundary - pointer to destination buffer
int len - length of destination buffer
RETURN
UTE_SUCCESS -   success
1           -   buffer size provided too small function
requires minimum 20 characters.  This is
not a MIME requirement, but is required by
this function's implementation.
2           -   buffer size provided too large. Mime
restricts boundary markers to 70 character
not including the leading and trailing
'--'.                         
****************************************************/
int CUT_MimeEncode::GetNewBoundaryString( LPSTR boundary,int len ){

	if (len<20)
		return -1;  // buffer too small

	if (len>70)
		return -2;  // buffer too large MIME restricted to 70 char boundries.

	SYSTEMTIME	s;
	GetSystemTime(&s);

	// the =_ combination is included here to help avoid difficulty with 
	// quoted-printable MIME encoding.  Other factors are readability 
	// and randomness.
	_snprintf(boundary,len, "Mark=_%d%d%d%d%d%d%d",s.wYear,s.wMonth,s.wDay,s.wHour,s.wMinute,s.wSecond,s.wMilliseconds);

	// add random characters onto the end of the boundary to arrive at the
	// length requested by the caller.
	for (int i = (int)strlen(boundary); i<(len-1); i++) 
		boundary[i]= m_szBase64Table[rand()%63];

	boundary[len-1]='\0';

	return UTE_SUCCESS;
}
/***************************************************
EncodeAddGlobalHeader()
Used in the process of encoding a file.
Allows the encoding class to provide header information
to the caller.  The provided header information 
should be included in the message when prepared
for transport.
Example:
The caller places multiple files into the encoding
list using multiple calls to AddFile().  When 
the caller requests that the listed files be 
encoded with a call to EncodeToFile(), the encoding class
calls the appropriate encoder for each attachment
(7BIT, BASE64, etc.).
When multiple attachments are encoded in a MIME file
each attachment must be separated from the next 
with a boundary marker. This boundary marker, along
with a 'Content-Type: Multipart/*' header must
be included in the Global message header that
precedes the encoded material in the message.
AddGlobalHeader() allows methods within the encoding
class to add to the list of global headers (m_listGlobalHeader).
See also:
GetNumberGlobalHeaders()
GetGlobalHeader()
PARAM
LPCSTR  tag - a string octet to be added to the whole result file
RETURN
UTE_SUCCESS	- success
UTE_ERROR	- error
****************************************************/
int CUT_MimeEncode::EncodeAddGlobalHeader(LPCSTR tag)
{
	// If the global header we are trying to add is already present in
	// CUT_Msg::m_listUT_HeaderFields we need to remove it from there in order to
	// avoid duplicates

	// Get the header name
	const char* pchNameEnd = ::strrchr(tag, ':');
	if (pchNameEnd != NULL)
	{
		int iIndex = (int)(pchNameEnd - tag);
		char szName[1024];
		::strncpy(szName, tag, iIndex + 1);
		szName[iIndex + 1] = ' '; // put a space after the colon
		szName[iIndex + 2] = 0;

		// Remove this header if present
		m_pMsg->ClearHeader(UTM_CUSTOM_FIELD, szName);
	}

	return (m_listGlobalHeader.AddString(tag)) ? UTE_SUCCESS : UTE_ERROR;
}

/***************************************************
Encode7bit() 
Internal function. Encode the item in 7 bit format
7bit encoding refers to data that is limited to 
byte values of 127 or less.  Lines have a maximum
length of 998 bytes, not including the line terminating
CRLF.  NULLs (0 ASCII) are not allowed, and CR (13
ASCII) and LF (10 ASCII) are only permitted to mark
the end of a line.
RETURN
UTE_SUCCESS					- success
UTE_DS_OPEN_FAILED			- failed to open data source
UTE_DS_WRITE_FAILED			- data source writing error
UTE_ENCODING_INVALID_CHAR	- invalid character in stream
UTE_ENCODING_LINE_TOO_LONG	- too many characters on one line
UTE_PARAMETER_INVALID_VALUE - item parameter not valid
****************************************************/
int CUT_MimeEncode::Encode7bit(MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest)
{
	// Check iten parameter
	if(item == NULL)
		return UTE_PARAMETER_INVALID_VALUE;

	CUT_StringList	*hitem = item->ptrHeaders;
	int				t, retval = UTE_SUCCESS, charsSinceCRLF;
	char			buf[1000];					// MUST BE 1000

	// Open attachment data source
	if(item->ptrDataSource == NULL)
		return UTE_DS_OPEN_FAILED;
	else if(item->ptrDataSource->Open(UTM_OM_READING) == -1)
		return UTE_DS_OPEN_FAILED;

	// check the attachment header and see if we have been
	// provided with a content type, etc.
	if (item->lpszContentType == NULL)
		_snprintf(buf,sizeof(buf)-1, "Content-Type: text/plain; charset=US-ASCII\r\n");
	else 
		_snprintf(buf,sizeof(buf)-1, "Content-Type: %s; charset=US-ASCII\r\n",item->lpszContentType);

	if(dest.Write(buf, strlen(buf)) != (int)strlen(buf)) {
		item->ptrDataSource->Close();
		return UTE_DS_WRITE_FAILED;
	}

	_snprintf(buf,sizeof(buf)-1, "Content-Transfer-Encoding: 7BIT\r\n" );
	if(dest.Write(buf, strlen(buf)) != (int)strlen(buf)) {
		item->ptrDataSource->Close();
		return UTE_DS_WRITE_FAILED;
	}

	// add any custom headers if they exist
	LPCSTR	lpszHeader;
	int		index = 0;
	if(hitem != NULL)
		while( (lpszHeader=hitem->GetString(index++)) != NULL ) {
			if(	dest.Write(lpszHeader, strlen(lpszHeader)) != (int)strlen(lpszHeader) ||
				dest.Write("\r\n", 2) != 2){
					item->ptrDataSource->Close();
					return UTE_DS_WRITE_FAILED;
			}
		}

		// add a one line space between the headers and the body of the message
		if(dest.Write("\r\n", 2) != 2) {
			item->ptrDataSource->Close();
			return UTE_DS_WRITE_FAILED;
		}

		charsSinceCRLF =0;

		// encode the source file contents and send to the output stream

		int bytesRead;
		do {

			bytesRead = item->ptrDataSource->Read(buf, 997);

			if( bytesRead <= 0 ) break;

			//test the line
			for ( t=0; t<bytesRead; t++ ) {
				if (buf[t]==0 || buf[t]>127) {
					retval = UTE_ENCODING_INVALID_CHAR;     // invalid character in stream 
					// (GW) changed from 2 to 4 to avoid dup return values of 2
					break;
				}

				if (buf[t] == '\r' && buf[t+1] == '\n')
					charsSinceCRLF = 0;
				else
					charsSinceCRLF++;

				//TODO:  maybe we should copy the buffer information from
				//one string to another, and automatically insert the CRLF if
				// it is necessary.?

				if ( charsSinceCRLF > 996 ){
					retval = UTE_ENCODING_LINE_TOO_LONG;	// too many characters on one line
					break;      
				}

			}

			if ( retval != UTE_SUCCESS )
				break;      // terminate processing, invalid character in stream


			// write the information to the output stream.
			if(dest.Write(buf, bytesRead) != bytesRead) {
				item->ptrDataSource->Close();
				return UTE_DS_WRITE_FAILED;
			}
		}while (bytesRead > 0);

		// Close data source
		item->ptrDataSource->Close();

		return retval;
}

/***************************************************
EncodeBase64()
base64 encoding coverts binary information from
8 bits per byte to 6 bits per byte.  Information
in 3 8bit bytes (24 bits) is converted into
4 6bit bytes (same 24 bits), transfered via
email, and reassembled into the original stream at
the receiving end. When the information is converted from 8 bits to
6, the 6bit value is used as an index into a 
standardized array of characters.
"The Base64 Content-Transfer-Encoding is designed to represent
arbitrary sequences of octets in a form that need not be humanly
readable.  The encoding and decoding algorithms are simple, but the
encoded data are consistently only about 33 percent larger than the
unencoded data.  This encoding is virtually identical to the one used
in Privacy Enhanced Mail (PEM) applications, as defined in RFC 1421.
A 65-character subset of US-ASCII is used, enabling 6 bits to be
represented per printable character. (The extra 65th character, "=",
is used to signify a special processing function."
RETURN
UTE_SUCCESS					- success
UTE_DS_OPEN_FAILED			- failed to open data source
UTE_DS_WRITE_FAILED			- data source writing error
UTE_PARAMETER_INVALID_VALUE - item parameter not valid
****************************************************/
int CUT_MimeEncode::EncodeBase64(MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest)
{
	// Check iten parameter
	if(item == NULL)
		return UTE_PARAMETER_INVALID_VALUE;

	CUT_StringList	*hitem = item->ptrHeaders;
	int				t;
	char			buf[2*MAX_PATH], out[5];

	// Open attachment data source
	if(item->ptrDataSource == NULL)
		return UTE_DS_OPEN_FAILED;
	else if(item->ptrDataSource->Open(UTM_OM_READING) == -1)
		return UTE_DS_OPEN_FAILED;


	// get the appropriate header information for the output
	// stream.  This is the information we need to contstruct the 
	// headers for the attachment

	//get the filename without the path
	char	szFileName[MAX_PATH + 1] = "";
	if(item->lpszName != NULL)
		for(t = (int)strlen(item->lpszName); t >= 0; t--)
		{
			if(item->lpszName[t] == '\\' || item->lpszName[t] == '/')
			{
				strcpy(szFileName, &item->lpszName[t+1]);
				break;
			}
			if(t == 0)
				strcpy(szFileName, item->lpszName);
		}

		// check the attachment header and see if we have been
		// provided with a content type, etc.

		if (item->lpszContentType == NULL)
			_snprintf(buf,sizeof(buf)-1, "Content-Type: Application/Octet-stream; name=\"%s\"; type=Unknown\r\n", &(szFileName));
		else 
			_snprintf(buf,sizeof(buf)-1, "Content-Type: %s; name=\"%s\"; type=Unknown\r\n",item->lpszContentType, &(szFileName));

		if(dest.Write(buf, strlen(buf)) != (int)strlen(buf)) {
			item->ptrDataSource->Close();
			return UTE_DS_WRITE_FAILED;
		}

		_snprintf(buf,sizeof(buf)-1,"Content-Transfer-Encoding: BASE64\r\n");
		if(dest.Write(buf, strlen(buf)) != (int)strlen(buf)) {
			item->ptrDataSource->Close();
			return UTE_DS_WRITE_FAILED;
		}


		// add any custom headers if they exist
		LPCSTR	lpszHeader;
		int		index = 0;
		if(hitem != NULL)
			while( (lpszHeader=hitem->GetString(index++)) != NULL ) {
				if(	dest.Write(lpszHeader, strlen(lpszHeader)) != (int)strlen(lpszHeader) ||
					dest.Write("\r\n", 2) != 2){
						item->ptrDataSource->Close();
						return UTE_DS_WRITE_FAILED;
				}
			}

			// add a one line space between the headers and the body of the message
			if(dest.Write("\r\n", 2) != 2) {
				item->ptrDataSource->Close();
				return UTE_DS_WRITE_FAILED;
			}

			// encode the source file contents and send to the output stream
			int bytesRead;
			do {
				// read up to 57 chars. when encoded the line expands by 4/3 or 1.333x
				// and results in an output line length within the BASE64 encoding
				// limit of 76 characters.
				bytesRead = item->ptrDataSource->Read(buf, 57);

				if(bytesRead <= 0)
					break;

				//encode the line
				for (t=0; t<bytesRead; t += 3) {
					EncodeBase64Bytes((unsigned char*)&buf[t], (unsigned char*)out, (bytesRead-t>3 ? 3 : bytesRead-t) );
					if(dest.Write(out, 4) != 4) {
						item->ptrDataSource->Close();
						return UTE_DS_WRITE_FAILED;
					}
				}

				if(dest.Write("\r\n", 2) != 2) {
					item->ptrDataSource->Close();
					return UTE_DS_WRITE_FAILED;
				}
			}while (bytesRead > 0 );

			// Close data source
			item->ptrDataSource->Close();

			return UTE_SUCCESS; 
}

/***************************************************
EncodeQuotedPrintable()
Encode the attachemnt item in QuotedPrintable. 
" The Quoted-Printable encoding is intended to represent data that
largely consists of octets that correspond to printable characters in
the US-ASCII character set.  It encodes the data in such a way that
the resulting octets are unlikely to be modified by mail transport.
If the data being encoded are mostly US-ASCII text, the encoded form
of the data remains largely recognizable by humans.  A body which is
entirely US-ASCII may also be encoded in Quoted-Printable to ensure
the integrity of the data should the message pass through a
character-translating, and/or line-wrapping gateway."
For more information see RFC 2045 SECTION 6.6
PARAM
MIMEATTACHMENTLISTITEM *item - pointer to attachment item to encoded
RETURN 
UTE_SUCCESS					- success
UTE_DS_OPEN_FAILED			- failed to open data source
UTE_DS_WRITE_FAILED			- data source writing error
UTE_PARAMETER_INVALID_VALUE - item parameter not valid
***************************************************/
int CUT_MimeEncode::EncodeQuotedPrintable(MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest)
{
	// Check item parameter
	if(item == NULL)
		return UTE_PARAMETER_INVALID_VALUE;

	CUT_StringList *hitem = item->ptrHeaders;
	int		t, retval = UTE_SUCCESS, outputPos, processed;
	char	buf[100];                  // input buffer
	char	output[sizeof(buf)*3];     // output buffer

	unsigned char hexList[] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

	// Open attachment data source
	if(item->ptrDataSource == NULL)
		return UTE_DS_OPEN_FAILED;
	else if(item->ptrDataSource->Open(UTM_OM_READING) == -1)
		return UTE_DS_OPEN_FAILED;

	// check the attachment header and see if we have been
	// provided with a content type, etc.
	if (item->lpszContentType == NULL)
		_snprintf(buf,sizeof(buf)-1, "Content-Type: Text/Plain; charset=US-ASCII\r\n");
	else 
		_snprintf(buf,sizeof(buf)-1, "Content-Type: %s; charset=US-ASCII\r\n",item->lpszContentType);

	if(dest.Write(buf, strlen(buf)) != (int)strlen(buf)) {
		item->ptrDataSource->Close();
		return UTE_DS_WRITE_FAILED;
	}	

	_snprintf(buf,sizeof(buf)-1,"Content-Transfer-Encoding: Quoted-Printable\r\n");
	if(dest.Write(buf, strlen(buf)) != (int)strlen(buf)) {
		item->ptrDataSource->Close();
		return UTE_DS_WRITE_FAILED;
	}	


	// add any custom headers if they exist
	LPCSTR	lpszHeader;
	int		index = 0;
	if(hitem != NULL)
		while( (lpszHeader=hitem->GetString(index++)) != NULL ) {
			if(	dest.Write(lpszHeader, strlen(lpszHeader)) != (int)strlen(lpszHeader) ||
				dest.Write("\r\n", 2) != 2){
					item->ptrDataSource->Close();
					return UTE_DS_WRITE_FAILED;
			}
		}

		// add a one line space between the headers and the body of the message
		if(dest.Write("\r\n", 2) != 2) {
			item->ptrDataSource->Close();
			return UTE_DS_WRITE_FAILED;
		}	

		outputPos = 0;

		// encode the source file contents and send to the output stream
		int bytesRead;
		do {

			bytesRead = item->ptrDataSource->Read(buf, sizeof(buf) - 1);

			if(bytesRead <= 0) break;

			for (t=0; t < bytesRead; t++) {
				processed = FALSE;

				// if we're near the 76 character per line limit, force a soft line 
				// break and reset the counter.  it is possible to add a maximum of 
				// 3 characters to the output stream each pass, so we should force the
				// line break if we are within 3 characters of 76.

				if (outputPos > 72) {
					output[outputPos++]='=';
					output[outputPos++]='\r';
					output[outputPos]='\n';

					if(dest.Write(output, outputPos+1) != outputPos+1) {
						item->ptrDataSource->Close();
						return UTE_DS_WRITE_FAILED;
					}	

					outputPos = 0;
				}

				// if the character value is in the defined 'acceptable'
				// range, it is appropriate to simply copy the character 
				// into the output message

				// rfc 2045 has also indicated that quoted printable encoding 
				// may not be reliable over EBCDIC gateways common on IBM mainframe
				// systems.  In order to improve reliability it may be necessary to 
				// for the following symbols to also be 'quoted' as hexidecimal values
				// in the output stream, rather than forwarding them through normally
				// the symbols:  !"#@[\]^`{|}~
				// as it is not required by the rfc to implement this EBCDIC special feature,
				// we have chosen to explain it, but not implement it.

				if ((buf[t] >= 33 && buf[t] <= 60) || (buf[t] >= 62 && buf[t] <= 126)) {
					output[outputPos] = buf[t];
					processed = TRUE;
					outputPos++;
				}

				// special handling may be appropriate for tab and space
				// characters.  As our implementation will always end a 
				// line with soft line break '=', we can simply copy the 
				// space and tab characters across without worry.

				if (buf[t] == '\t' || buf[t] == ' ') { 
					output[outputPos++]=buf[t];
					processed = TRUE;
				}

				// for any other character that does not fit into the previous
				// categories, we must convert it into hexidecimal form, prepend it
				// with an '=' and add it to the output stream.

				if (!processed) {
					output[outputPos++]='=';
					output[outputPos++]=hexList[(int)((unsigned char)buf[t]/16)];
					output[outputPos++]=hexList[(int)((unsigned char)buf[t]%16)];
				}
			} // for loop processing 100 chars at a time
		}while (bytesRead > 0);  // while loop processing entire file

		if ( outputPos > 0 )      // there are still characters in the output stream
			if(dest.Write(output, outputPos) != outputPos) {
				item->ptrDataSource->Close();
				return UTE_DS_WRITE_FAILED;
			}

			// Close data source
			item->ptrDataSource->Close();

			return retval;
}

/***************************************************
Encode8bit()
Encode the item in 8bit format.
"8bit data" refers to data that is all represented as relatively
short lines with 998 octets or less between CRLF line separation
sequences [RFC-821]), but octets with decimal values greater than 127
may be used. For More information see RFC 2045 section 2.8
PARAM
MIMEATTACHMENTLISTITEM *item - pointer to attachment item to encoded
RETURN
UTE_SUCCESS					- success
UTE_DS_OPEN_FAILED			- failed to open data source
UTE_DS_WRITE_FAILED			- data source writing error
UTE_PARAMETER_INVALID_VALUE - item parameter not valid
UTE_ENCODING_LINE_TOO_LONG	- too many characters on one line
****************************************************/
int CUT_MimeEncode::Encode8bit(MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest)
{
	// Check item parameter
	if(item == NULL)
		return UTE_PARAMETER_INVALID_VALUE;

	CUT_StringList	*hitem = item->ptrHeaders;
	int				t, retval = UTE_SUCCESS, charsSinceCRLF;
	char			buf[1000];  // MUST BE 1000 characters


	// Open attachment data source
	if(item->ptrDataSource == NULL)
		return UTE_DS_OPEN_FAILED;
	else if(item->ptrDataSource->Open(UTM_OM_READING) == -1)
		return UTE_DS_OPEN_FAILED;

	// check the attachment header and see if we have been
	// provided with a content type, etc.

	if (item->lpszContentType == NULL)
		_snprintf(buf,sizeof(buf)-1, "Content-Type: text/plain; charset=US-ASCII\r\n");
	else 
		_snprintf(buf,sizeof(buf)-1, "Content-Type: %s; charset=US-ASCII\r\n",item->lpszContentType);

	if(dest.Write(buf, strlen(buf)) != (int)strlen(buf)) {
		item->ptrDataSource->Close();
		return UTE_DS_WRITE_FAILED;
	}	

	_snprintf(buf,sizeof(buf)-1,"Content-Transfer-Encoding: 8BIT\r\n");
	if(dest.Write(buf, strlen(buf)) != (int)strlen(buf)) {
		item->ptrDataSource->Close();
		return UTE_DS_WRITE_FAILED;
	}	


	// add any custom headers if they exist
	LPCSTR	lpszHeader;
	int		index = 0;
	if(hitem != NULL)
		while( (lpszHeader=hitem->GetString(index++)) != NULL ) {
			if(	dest.Write(lpszHeader, strlen(lpszHeader)) != (int)strlen(lpszHeader) ||
				dest.Write("\r\n", 2) != 2){
					item->ptrDataSource->Close();
					return UTE_DS_WRITE_FAILED;
			}
		}


		// add a one line space between the headers and the body of the message
		if(dest.Write("\r\n", 2) != 2) {
			item->ptrDataSource->Close();
			return UTE_DS_WRITE_FAILED;
		}	


		charsSinceCRLF =0;

		// encode the source file contents and send to the output stream
		int bytesRead;
		do{

			bytesRead = item->ptrDataSource->Read(buf, 998);

			if(bytesRead <= 0)
				break;

			//test the line.  Check to make sure that there is a CRLF combination
			// at least every 1000.  Required for the MIME RFC for 7BIT.
			for (t=0; t < bytesRead; t++) {

				if (buf[t] == '\r' && buf[t+1] == '\n')
					charsSinceCRLF = 0;
				else
					charsSinceCRLF++;

				//TODO:  maybe we should copy the buffer information from
				//one string to another, and automatically insert the CRLF if
				// it is necessary.?

				if ( charsSinceCRLF > 997 ) {
					retval = UTE_ENCODING_LINE_TOO_LONG;     // to many characters on one line
					break;      
				}	
			}

			if ( retval != UTE_SUCCESS )
				break;      // terminate processing, invalid character in stream


			// write the information to the output stream.
			if(dest.Write( buf,bytesRead) != bytesRead) {
				item->ptrDataSource->Close();
				return UTE_DS_WRITE_FAILED;
			}
		}while (bytesRead > 0);

		// Close data source
		item->ptrDataSource->Close();

		return retval;
}

/***************************************************
EncodeBinary()
Encode the attachemnet item in Binary format
"Binary data" refers to data where any sequence of octets
is allowed
PARAM
MIMEATTACHMENTLISTITEM *item
RETURN
UTE_SUCCESS					- success
UTE_DS_OPEN_FAILED			- failed to open data source
UTE_DS_WRITE_FAILED			- data source writing error
UTE_PARAMETER_INVALID_VALUE - item parameter not valid
****************************************************/
int CUT_MimeEncode::EncodeBinary(MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest)
{
	// Check item parameter
	if(item == NULL)
		return UTE_PARAMETER_INVALID_VALUE;

	CUT_StringList	*hitem = item->ptrHeaders;
	int				t;
	char			buf[1000];
	char			tempAttachType[100];
	char			tempContentType[100];

	// Open attachment data source
	if(item->ptrDataSource == NULL)
		return UTE_DS_OPEN_FAILED;
	else if(item->ptrDataSource->Open(UTM_OM_READING) == -1)
		return UTE_DS_OPEN_FAILED;


	// get the appropriate header information for the output
	// stream.  This is the information we need to contstruct the 
	// headers for the attachment

	//get the filename without the path
	char	szFileName[MAX_PATH + 1] = "";
	if(item->lpszName != NULL)
		for(t = (int)strlen(item->lpszName); t >= 0; t--)
		{
			if(item->lpszName[t] == '\\' || item->lpszName[t] == '/')
			{
				strcpy(szFileName, &item->lpszName[t+1]);
				break;
			}
			if(t == 0)
				strcpy(szFileName, item->lpszName);
		}

		// check the attachment header and see if we have been
		// provided with a content type, etc.

		if (item->lpszContentType == NULL)
			strcpy(tempContentType, "Application/Octet-stream");
		else
			strcpy(tempContentType, item->lpszContentType);

		if (item->lpszAttachType == NULL)
			strcpy(tempAttachType, "Unknown");
		else
			strcpy(tempAttachType, item->lpszAttachType);


		_snprintf(buf,sizeof(buf)-1,"Content-Type: %s; name=\"%s\"; type=%s\r\n", tempContentType, &szFileName, tempAttachType);
		if(dest.Write(buf, strlen(buf)) != (int)strlen(buf)) {
			item->ptrDataSource->Close();
			return UTE_DS_WRITE_FAILED;
		}	

		_snprintf(buf,sizeof(buf)-1,"Content-Transfer-Encoding: BINARY\r\n");
		if(dest.Write(buf, strlen(buf)) != (int)strlen(buf)) {
			item->ptrDataSource->Close();
			return UTE_DS_WRITE_FAILED;
		}	


		// add any custom headers if they exist
		LPCSTR	lpszHeader;
		int		index = 0;
		if(hitem != NULL)
			while( (lpszHeader=hitem->GetString(index++)) != NULL ) {
				if(	dest.Write(lpszHeader, strlen(lpszHeader)) != (int)strlen(lpszHeader) ||
					dest.Write("\r\n", 2) != 2){
						item->ptrDataSource->Close();
						return UTE_DS_WRITE_FAILED;
				}
			}

			// add a one line space between the headers and the body of the message
			if(dest.Write("\r\n", 2) != 2) {
				item->ptrDataSource->Close();
				return UTE_DS_WRITE_FAILED;
			}	


			// encode the source file contents and send to the output stream
			int bytesRead;
			do {
				bytesRead = item->ptrDataSource->Read(buf, sizeof(buf) - 1);

				if(bytesRead <= 0)
					break;

				if(dest.Write(buf, bytesRead ) != bytesRead ) {
					item->ptrDataSource->Close();
					return UTE_DS_WRITE_FAILED;
				}	
			}while ((bytesRead > 0));

			// Close data source
			item->ptrDataSource->Close();

			return UTE_SUCCESS;
}

/***************************************************
// BinHex format (NOT SUPPORTED, Here to allow for future integration)
****************************************************/
int CUT_MimeEncode::EncodeBinHex(MIMEATTACHMENTLISTITEM * item, CUT_DataSource & dest)
{
	//TODO:
	UNREFERENCED_PARAMETER(item);
	UNREFERENCED_PARAMETER(dest);
	return CUT_NOT_IMPLEMENTED;

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
int CUT_MimeEncode::DecodeBase64Bytes(unsigned char *in, unsigned char *out){

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
void CUT_MimeEncode::EncodeBase64Bytes(unsigned char *in, unsigned char *out, int count){

	int index1 = ((in[0] >> 2) & 63);
	int index2 = (((in[0] << 4) & 48) | ((in[1] >> 4) & 15));
	int index3 = (((in[1] << 2) & 60) | ((in[2] >> 6) & 3));
	int index4 = (in[2] & 63);

	out[0] = m_szBase64Table[index1];
	out[1] = m_szBase64Table[index2];
	out[2] = m_szBase64Table[index3];
	out[3] = m_szBase64Table[index4];

	switch(count){
	case 1:
		out[2]='=';
	case 2:
		out[3]='=';
	}
}

/***************************************************
FindBase64Val()
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
UCHAR CUT_MimeEncode::FindBase64Val(char character){

	for (UCHAR pos = 0; pos <64; pos ++)
		if (m_szBase64Table[pos] == character)
			return pos;

	return (UCHAR)-1;
}

/***************************************************
Decode7bit()
Decode the attachment item to the data souce using 7Bit scheme
PARAM
MIMEATTACHMENTLISTITEM *item	- Attachment item to be decoded
dest						- data source to decode to
RETURN
UTE_SUCCESS			- success
UTE_DS_OPEN_FAILED	- failed to open data source
UTE_DS_WRITE_FAILED	- data source writing error
****************************************************/
int CUT_MimeEncode::Decode7bit(MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest)
{

	int		rt = UTE_SUCCESS, bytesRead, bytesToRead;
	char	buf[512];

	// Open decode data source
	if(m_ptrDecodeDataSource == NULL || m_ptrDecodeDataSource->Open(UTM_OM_READING) == -1)
		return UTE_DS_OPEN_FAILED;


	// seek the beginning of the input file.
	m_ptrDecodeDataSource->Seek(item->lOffset, SEEK_SET);

	// calculated the number of 512 byte chunks we need, and one
	// extra iteration to pick up leftover piece at the end.
	long contentLeft = item->lContentLength;

	long loops = (contentLeft/(sizeof(buf)-1)) +1 ;

	for(long i = 0; i < loops && rt == UTE_SUCCESS; i++ ) {
		bytesToRead = (int)min( sizeof(buf) - 1, (size_t)contentLeft );

		bytesRead = m_ptrDecodeDataSource->Read(buf, bytesToRead);

		if(bytesRead < 1)	break;

		if(dest.Write(buf, bytesRead) != bytesRead) 
			rt = UTE_DS_WRITE_FAILED;		//error writing data source

		contentLeft -= bytesRead;
	}

	// Close data sources
	m_ptrDecodeDataSource->Close();

	return rt;
}

/***************************************************
DecodeBase64()
Decode the attachment item to the data source using base64 scheme
PARAM
MIMEATTACHMENTLISTITEM *item	- Attachment item to be decoded
dest						- data source to decode to
RETURN
UTE_SUCCESS			- success
UTE_DS_OPEN_FAILED	- failed to open data source
UTE_DS_WRITE_FAILED	- data source writing error
****************************************************/
int CUT_MimeEncode::DecodeBase64( MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest)
{

	int		rt = UTE_SUCCESS, bytesRead, bytesToRead, nBytes;
	int		outputPos,inPos;
	int		x, yy;
	long	loops;

	char	buf[512];
	char	output[384];       // output will have a maximum of 3/4 input length
	char	in[4];
	char	out[3];


	// Open decode data source
	if(m_ptrDecodeDataSource == NULL || m_ptrDecodeDataSource->Open(UTM_OM_READING) == -1)
		return UTE_DS_OPEN_FAILED;


	// seek the beginning of the input file.
	m_ptrDecodeDataSource->Seek(item->lOffset, SEEK_SET);

	long contentLeft = item->lContentLength;

	// calculated the number of bufLen byte chunks we need, and
	// add one extra loop to read the last chunk that will be 
	// less than 1 buffer length.

	loops		= ( contentLeft/(sizeof(buf)-1) ) +1;
	outputPos	= 0;
	inPos		= 0;

	for(long i = 0; i < loops && rt == UTE_SUCCESS; i++ ) {

		bytesToRead = (int)min( sizeof(buf)-1, (size_t)contentLeft );

		bytesRead = m_ptrDecodeDataSource->Read(buf, bytesToRead);

		if ( bytesRead <= 0 )
			break;

		for (x=0; x < bytesRead; x++) {

			if (buf[x]!='\r' && buf[x]!='\n' && buf[x]!=' ') {		// if a valid base64 char then
				in[inPos++]=buf[x];									//add to the decode array

				if (inPos == 4) {									// if we've added 4 characters, then decode a chunk
					// decode and find out how many bytes resulted.
					nBytes = DecodeBase64Bytes((unsigned char *)in, (unsigned char *)out);      
					for (yy = 0; yy < nBytes; yy++) // copy decoded bytes into the output buffer
						output[outputPos++] = out[yy];
					inPos =0;
				}
			}
		}

		if(dest.Write( output, outputPos) != outputPos)
			rt = UTE_DS_WRITE_FAILED;						// Data source write error

		outputPos = 0;

		contentLeft -= bytesRead;
	}
	// Close data sources
	m_ptrDecodeDataSource->Close();

	return rt;
}

/***************************************************
DecodeQuotedPrintable()
Decode the attachment item to the data source using QuotedPrintable scheme
PARAM
MIMEATTACHMENTLISTITEM *item	- Attachment item to be decoded
dest						- data source to decode to
RETURN
UTE_SUCCESS			- success
UTE_DS_OPEN_FAILED	- failed to open data source
UTE_DS_WRITE_FAILED	- data source writing error
****************************************************/
int CUT_MimeEncode::DecodeQuotedPrintable(MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest){

	int			rt = UTE_SUCCESS;
	unsigned int bytesRead;
	long		outputPos;
	char		*buf;
	char		*output;   

	// Open decode data source
	if(m_ptrDecodeDataSource == NULL || m_ptrDecodeDataSource->Open(UTM_OM_READING) == -1)
		return UTE_DS_OPEN_FAILED;

	// seek the beginning of the input file.
	m_ptrDecodeDataSource->Seek(item->lOffset, SEEK_SET);


	buf		= new char[item->lContentLength + 1];
	output	= new char[item->lContentLength + 1];  // decoded file should always be smaller


	bytesRead = m_ptrDecodeDataSource->Read(buf, item->lContentLength);

	outputPos = 0;

	char lastChar		= ' ';
	char secondLastChar = ' ';

	BOOL skip;

	for (unsigned int t = 0; t <= bytesRead; t++) {
		skip = FALSE;

		// check for equal sign
		// if the current character is an equal sign, we don't have
		// enough information to do anything yet.  The equal sign
		// is a flag character than can either signal the beginning of 
		// a 'quoted' character (one that is represented by its hex value
		// or it can represent a soft line break, placed at the end of 
		// a line.
		// we must read another character before doing further output 
		// processing.
		if (buf[t] == '=')
			skip = TRUE;

		// if the previous character was an '=', then test to
		// see if it is followed by space or tab. If so, we're
		// dealing with a soft line break sequence.  We must reposition
		// the buffer pointer to the last character on the line and continue
		// processing.

		if (!skip) {
			if (lastChar == '=' && (buf[t]==' ' || buf[t]=='\t' || buf[t] == '\r')){

				//skip to last character in the line
				while( t < bytesRead && buf[++t]!='\n' ) ;

				secondLastChar = ' ';
				lastChar = ' ';
				skip = TRUE;
			}
		}


		if (!skip) {
			if (lastChar == '=') {
				skip = TRUE;
			}
		}


		// if the second last char is an '=', we must be dealing with
		// a quoted character.  Convert the quoted character back to USASCII
		// place it in the output buffer and continue processing.
		if (!skip) {
			if (secondLastChar == '=') {
				// process quoted character  (eg.  =0D is ASCII 13 decimal)
				output[outputPos++]=(char)HexToDec(&buf[t-1],2);

				skip = TRUE;
				secondLastChar = ' ';
				lastChar = ' ';
			}
		}

		if (!skip)	
			output[outputPos++]=buf[t];

		secondLastChar = lastChar;
		lastChar = buf[t];
	}

	if(dest.Write(output, outputPos-1) != outputPos-1)
		rt = UTE_DS_WRITE_FAILED;

	// Close data sources
	m_ptrDecodeDataSource->Close();

	delete[] buf;
	delete[] output;

	return rt;
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
long CUT_MimeEncode::HexToDec(char *hexVal, int numDigits){

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
Decode8bit()
Decode the attachment item to the data source using 8Bit scheme
PARAM
MIMEATTACHMENTLISTITEM *item	- Attachment item to be decoded
dest						- data source to decode to
RETURN
UTE_SUCCESS			- success
UTE_DS_OPEN_FAILED	- failed to open data source
UTE_DS_WRITE_FAILED	- data source writing error
****************************************************/
int CUT_MimeEncode::Decode8bit(MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest)
{

	int		rt = UTE_SUCCESS, bytesRead, bytesToRead;
	char	buf[512];

	// Open decode data source
	if(m_ptrDecodeDataSource == NULL || m_ptrDecodeDataSource->Open(UTM_OM_READING) == -1)
		return UTE_DS_OPEN_FAILED;

	// seek the beginning of the input file.
	m_ptrDecodeDataSource->Seek(item->lOffset, SEEK_SET);

	long contentLeft = item->lContentLength;

	//calculated the number of bufLen byte chunks we need
	long loops = ( contentLeft/(sizeof(buf)-1) ) +1;

	for (long i = 0; i < loops && rt == UTE_SUCCESS; i++) { 

		bytesToRead = (int)min( sizeof(buf)-1, (size_t)contentLeft );

		bytesRead = m_ptrDecodeDataSource->Read(buf, bytesToRead);

		if (bytesRead < 1) break;

		if(dest.Write(buf, bytesRead) != bytesRead)
			rt = UTE_DS_WRITE_FAILED;

		contentLeft -= bytesRead;
	}

	// Close data sources
	m_ptrDecodeDataSource->Close();

	return rt;
}

/***************************************************
DecodeBinary
Decode the attachment item to the data source using Binary scheme
PARAM
MIMEATTACHMENTLISTITEM *item	- Attachment item to be decoded
dest						- data source to decode to
RETURN
UTE_SUCCESS			- success
UTE_DS_OPEN_FAILED	- failed to open data source
UTE_DS_WRITE_FAILED	- data source writing error
****************************************************/
int CUT_MimeEncode::DecodeBinary(MIMEATTACHMENTLISTITEM *item, CUT_DataSource & dest)
{

	int		rt = UTE_SUCCESS, bytesRead, bytesToRead;
	char	buf[512];

	// Open decode data source
	if(m_ptrDecodeDataSource == NULL || m_ptrDecodeDataSource->Open(UTM_OM_READING) == -1)
		return UTE_DS_OPEN_FAILED;

	// seek the beginning of the input file.
	m_ptrDecodeDataSource->Seek(item->lOffset, SEEK_SET);

	long contentLeft = item->lContentLength;

	//calculated the number of 512 byte chunks we need, and add
	// one iteration to get the leftover at the end

	long loops = ( contentLeft/(sizeof(buf)-1) ) +1;

	for(long i = 0; i < loops && rt == UTE_SUCCESS; i++) {

		bytesToRead = (int)min( sizeof(buf)-1, (size_t)contentLeft );

		bytesRead = m_ptrDecodeDataSource->Read(buf, bytesToRead);

		if (bytesRead < 1)
			break;

		if(dest.Write(buf, bytesRead) != bytesRead)
			rt = UTE_DS_WRITE_FAILED;

		contentLeft -= bytesRead;
	}

	// Close data sources
	m_ptrDecodeDataSource->Close();

	return rt;
}

/***************************************************
// BinHex format (NOT SUPPORTED, Here to allow for future integration)

BinHex is structurally very similar to Base64 encoding.
Binhex encodes specific information about the file, its
length, CRC and name information, etc.  BinHex files are
indended as a trasnfer medium for Macintosh files which have a 
separate data and resource fork.  This information is also encoded
into the file.
Non-Macintosh systems will use only the Data fork.
BinHex also includes a simple RLE compression scheme.
***************************************************/
int CUT_MimeEncode::DecodeBinHex(MIMEATTACHMENTLISTITEM * item, CUT_DataSource & dest){

	//TODO:
	UNREFERENCED_PARAMETER(item);
	UNREFERENCED_PARAMETER(dest);
	return CUT_NOT_IMPLEMENTED;

}

/***************************************************
WriteBoundaryString()
The Boundary string is the point where we will determine 
the start of the whole encoded file attachment or a new attachment item.
The boundary delimiter line is defined as a line
consisting entirely of two hyphen characters ("-", decimal value 45)
followed by the boundary parameter value from the Content-Type header
field, optional linear whitespace, and a terminating CRLF
PARAM
LPCSTR boundaryString - the boundary octet string to be written to the file
int style - an intermediate boundary or an end of file boundry
RETURN
UTE_ERROR	- error
UTE_SUCCESS	- success
****************************************************/
int CUT_MimeEncode::WriteBoundaryString(CUT_DataSource & dest, LPCSTR boundaryString, int style){

	int		rt = 0, len = (int)strlen(boundaryString);

	switch(style){
		case BS_INTERMEDIATE:
			{
				//write the boundaryString to the file with only a leading '--'
				rt += dest.Write("\r\n--", 4);
				rt += dest.Write(boundaryString, len);
				rt += dest.Write("\r\n", 2); 

				// Data source write error
				if(rt != 6 + len)
					return UTE_ERROR;

				break;
			}
		case BS_ENDOFFILE:
			{
				// write the boundaryString to the file with both a leading and trailing '--'
				rt += dest.Write("\r\n--", 4);
				rt += dest.Write(boundaryString, len);
				rt += dest.Write("--\r\n", 4);

				// Data source write error
				if(rt != 8 + len)
					return UTE_ERROR;

				break;
			}
	}


	return UTE_SUCCESS;
}

/***************************************************
AddDecodeFileInfo()
Used in the process of decoding a file.
This function takes the provided parameters, 
uses them to populate an MIMEATTACHMENTLISTITEM
structure, and then passes the created structure
to an Overloaded method with the same name.
PARAM
LPCSTR fileName    - file name of the attachment found
LPCSTR contentType - content type of the attachment found
int encodeType     - encode type of the attachment found
long fileOffset    - location of the attachment in the source file being decoded
long contentLength - length of the attachment file
RETURN
VOID
***************************************************/
void CUT_MimeEncode::AddDecodeFileInfo (LPCSTR fileName, LPCSTR contentType,
										int encodeType, long fileOffset, long contentLength){

											MIMEATTACHMENTLISTITEM item;

											item.lSize          = 0;
											item.lpszName = new char[strlen(fileName)+1];
											item.lpszContentType = new char[strlen(contentType)+1];

											strcpy(item.lpszName, fileName);
											strcpy(item.lpszContentType, contentType);

											item.nEncodeType       = encodeType;
											item.lOffset           = fileOffset;
											item.lContentLength    = contentLength;

											// TODO:  allow user to supply these values
											item.lpszCharSet = NULL;
											item.lpszAttachType = NULL;
											AddDecodeFileInfo(&item);
}

/***************************************************
AddDecodeFileInfo()
Used in the process of decoding a file.
This method accepts an MIMEATTACHMENTLISTITEM
structure, and appends it to a linked list of
attachments (m_ptrAttachList).
SEE ALSO  LocateAttachments()
File Information includes
encodeType  
contentLength 
offset  (location of the attachment in the file)
item->ptrHeaders       = NULL;
PARAM
MIMEATTACHMENTLISTITEM *newitem -  new item to be added 
to the list of attachments information
RETURN 
VOID
***************************************************/
void CUT_MimeEncode::AddDecodeFileInfo (MIMEATTACHMENTLISTITEM *newitem){

	MIMEATTACHMENTLISTITEM *item;

	if (m_ptrAttachList != NULL){

		item = m_ptrAttachList;
		while (item->next != NULL)
			item = item->next;

		item->next = new MIMEATTACHMENTLISTITEM;
		item=item->next;

	} else {

		m_ptrAttachList = new MIMEATTACHMENTLISTITEM;
		item = m_ptrAttachList;
	}


	// copy information if it is provided.
	if (newitem->lpszName != NULL){
		item->lpszName = new char[strlen(newitem->lpszName)+1];
		strcpy(item->lpszName, newitem->lpszName);
	} else {
		item->lpszName = NULL;
	}


	// copy information if it is provided.
	if (newitem->lpszContentType != NULL){
		item->lpszContentType = new char[strlen(newitem->lpszContentType)+1];
		strcpy(item->lpszContentType, newitem->lpszContentType);
	} else {
		item->lpszContentType = NULL;
	}

	// copy information if it is provided.
	if (newitem->lpszAttachType != NULL){
		item->lpszAttachType = new char[strlen(newitem->lpszAttachType)+1];
		strcpy(item->lpszAttachType, newitem->lpszAttachType);
	} else {
		item->lpszAttachType = NULL;
	}

	// copy information if it is provided.
	if (newitem->lpszCharSet != NULL){
		item->lpszCharSet = new char[strlen(newitem->lpszCharSet)+1];
		strcpy(item->lpszCharSet, newitem->lpszCharSet);
	} else {
		item->lpszCharSet = NULL;
	}

	item->ptrDataSource	= NULL;
	if (item->lpszContentType != NULL)
		item->nEncodeType		= newitem->nEncodeType;
	item->lContentLength	= newitem->lContentLength;
	item->lOffset			= newitem->lOffset;
	item->lSize			= newitem->lSize;
	item->next			= NULL;
	item->ptrHeaders		= NULL;
}


/***************************************************
GetDecodeListItem()

Returns the list item corresponding to the index provided.

****************************************************/
int CUT_MimeEncode::GetDecodeListItem(int index, MIMEATTACHMENTLISTITEM **work){

	MIMEATTACHMENTLISTITEM    *item;

	int count = 0;

	item = m_ptrAttachList;

	while( item != NULL && count != index ){
		count++;
		item = item->next;
	}

	if ( count != index )
		return -1;

	*work = item;

	return UTE_SUCCESS;

}

/***************************************************
GetMessageHeader
Helper function. Reads header from the message data source.
The header can be split in several lines in this case the 
next line of the message starts with the whitespace (SP or TAB).
It also excludes comments enclosed in matching parentheses.
PARAM
ds         - message data source
lPosition  - filled with position in the data source where 
the header is finished
RETURN
Pointer to the header data.
Use delete [] to release the data.
****************************************************/
LPSTR CUT_MimeEncode::GetMessageHeader(CUT_DataSource *ds, long &lPosition)
{
	char    szBuffer[1024];
	char    *lpszData;
	int     i, nDataBuffSize = 2048, nBytesRead = 0;

	// Allocate data buffer
	lpszData = new char[nDataBuffSize];

	// Read the first line
	nBytesRead += ds->ReadLine(lpszData, nDataBuffSize-1);
	CUT_StrMethods::RemoveCRLF(lpszData);
	lPosition = ds->Seek(0, SEEK_CUR);

	// Keep reading lines while it begins with the whitespace
	do {
		i = ds->ReadLine(szBuffer, sizeof(szBuffer)-1);
		if(i > 0 && (*szBuffer == ' ' || *szBuffer == '\t')) {
			CUT_StrMethods::RemoveCRLF(szBuffer);
			lPosition = ds->Seek(0, SEEK_CUR);
			nBytesRead += i;

			// Check if the data buffer is big enough
			if(nBytesRead >= nDataBuffSize) {
				char    *lpszTmp;

				nDataBuffSize += 2048;                  // Increase the data buffer size
				lpszTmp = new char[nDataBuffSize];      // Allocate new buffer
				strcpy(lpszTmp, lpszData);              // Copy existing data
				delete[] lpszData;                      // Free previous buffer
				lpszData = lpszTmp;                     // Copy the buffer pointer
			}

			// Add line to the data buffer
			strcat(lpszData, (szBuffer + 1));
		}
	}while ((i > 0 && (*szBuffer == ' ' || *szBuffer == '\t')));

	// Remove comments enclosed in matching parentheses.
	int     nTo = 0, nFrom = 0;
	BOOL    bQuoted = FALSE;
	do {
		// Exclude quoted text
		if(lpszData[nFrom] == '\"' && (nFrom == 0 || lpszData[nFrom-1] != '\\'))
			bQuoted = !bQuoted;

		// If the parenthese was found outside the quoted text - it's a comment
		else if(lpszData[nFrom] == '(' && !bQuoted) {
			while(lpszData[nFrom] != ')' && lpszData[nFrom] != NULL)
				++nFrom;
			if(lpszData[nFrom] == ')') 
				++nFrom;
		}

		// Copy characters
		lpszData[nTo++] = lpszData[nFrom++];
	} while(lpszData[nFrom] != NULL);

	return lpszData;
}

/********************************************************** 
DecodeGetAttachmentInfo
Gets and parse attachment Content-Type header. Name, Charset
and type parameters are processed all others are ignored.
PARAM
workitem    - a temporary attachment item
filePos     - position of the Content-Type header in the data source
RETURN
VOID
**********************************************************/
void CUT_MimeEncode::DecodeGetAttachmentInfo(MIMEATTACHMENTLISTITEM *workitem, long filePos)
{

	char    *lpszData ;      // Header data pointer
	char    lpszSrcData [MAX_PATH];      // Header data pointer
	char    *lpszToken;     // Temp token pointer
	char    lpszFoundToken[MAX_PATH];     // Temp token pointer
	char    lpszPiece[MAX_PATH];     // Temp token pointer

	// Seek data source to the filePos
	m_ptrDecodeDataSource->Seek(filePos, SEEK_SET); 

	// Get the 'Content-Type:' header data
	lpszData =  GetMessageHeader(m_ptrDecodeDataSource, filePos);
	strcpy(lpszSrcData ,lpszData);

	// The header should start with the 'Content-Type:' text
	CUT_StrMethods::ParseString (lpszSrcData,":",0,lpszFoundToken, MAX_PATH-1 );
	// lpszFoundToken = strtok(lpszSrcData, ":");
	if(lpszFoundToken != NULL && ((_stricmp(lpszFoundToken, "Content-Type") == 0  || _stricmp(lpszFoundToken, "Content-Description") == 0  || _stricmp(lpszFoundToken, "Content-Disposition") == 0))) 
	{


		// Get the pointer to the rest of data
		CUT_StrMethods::ParseString (lpszSrcData,":",1,lpszFoundToken, MAX_PATH-1 );
		//         lpszFoundToken = strtok(NULL, ":");

		// if the content is a multipart then handel it differently
		// Get the pointer to the Type/SubType
		CUT_StrMethods::ParseString (lpszFoundToken, " \t;",0, lpszPiece , MAX_PATH-1 );
		
		// v4.2 added check for multipart/mixed
		if (_stricmp(lpszPiece, "Multipart/Related") == 0 || 
			_stricmp(lpszPiece, "Multipart/Alternative") == 0 ||
			_stricmp(lpszPiece, "Multipart/Mixed") == 0 )
		{
			char    lpszMultBoundString [MAX_PATH];      // Header data pointer

			// SO WE ARE MULTIPART 
			// SO LET FIRST FIND THE MULTIPART STRING  BOUNDRY
			// then we go an get the multipart entities

			GetMultiPartBoundryString (filePos, MAX_PATH,lpszMultBoundString );
			LocateMultiPartEntities (lpszMultBoundString, filePos);
			// call the get attachemnt info for a multipart nested entry
			if((lpszPiece) != NULL) {
				workitem->lpszContentType = new char[strlen(lpszPiece)+1];
				strcpy(workitem->lpszContentType, lpszPiece);
			}
			delete []lpszData;
			return ;		
		}

		if((lpszPiece) != NULL)
		{
			workitem->lpszContentType = new char[strlen(lpszPiece)+1];
			strcpy(workitem->lpszContentType, lpszPiece);
			// Loop through all optional parameters (parameter := attribute "=" value)
			int loop = 1;
			int count  = CUT_StrMethods::GetParseStringPieces (lpszFoundToken, "=\t;");
			do{
				// Get the parameter attribute name
				CUT_StrMethods::ParseString (lpszFoundToken, "=\t;",loop, lpszPiece , MAX_PATH-1 );

				// lpszFoundToken = strtok(NULL, "= \t;");
				if(lpszPiece != NULL) {

					CUT_StrMethods::RemoveSpaces (lpszPiece);

					// Get the attribute type. And initialize the pointer to the value pointer
					char **lplpszValue = NULL;
					if(_stricmp(lpszPiece, "NAME") == 0 || _stricmp(lpszPiece, "filename") == 0)     
						lplpszValue = &workitem->lpszName;
					if(_stricmp(lpszPiece, "CHARSET") == 0)  
						lplpszValue = &workitem->lpszCharSet;
					if(_stricmp(lpszPiece, "TYPE") == 0)     
						lplpszValue = &workitem->lpszAttachType;

					CUT_StrMethods::ParseString (lpszFoundToken, "=\t;",loop+1, lpszPiece , MAX_PATH-1, '"' );
					// Get attribute value
					// lpszFoundToken = strtok(NULL, ";");
					if(lpszPiece != NULL && lplpszValue != NULL) {
						*lplpszValue = new char[strlen(lpszPiece) + 1];
						// Copy the value
						strcpy(*lplpszValue, lpszPiece);

						// Remove extra spaces
						CUT_StrMethods::RemoveSpaces(*lplpszValue);

						// If the value is the quoted string - remove quotes
						if((*lplpszValue)[0] == '\"') {
							memmove(*lplpszValue, (*lplpszValue)+1, strlen(*lplpszValue)-1);
							int i = 0;
							while((*lplpszValue)[i] != 0 && (*lplpszValue)[i] != '\"')
								++i;

							(*lplpszValue)[i] = NULL;
						}
					}
					loop ++;
				}
				loop ++;
			}while (count > loop) ;
		}

	}  
	// now lets get the name even if above did not work
	if (workitem->lpszName == NULL)
	{
		m_ptrDecodeDataSource->Seek(filePos, SEEK_SET); 
		filePos = filePos-2;
		delete []lpszData;
		lpszData = GetMessageHeader(m_ptrDecodeDataSource, filePos);

		// The header should start with the 'Content-Type:' text
		lpszToken = strtok(lpszData, ":");
		if(lpszToken != NULL && ((_stricmp(lpszToken, "Content-Type") == 0  || _stricmp(lpszToken, "Content-Description") == 0  || _stricmp(lpszToken, "Content-Disposition") == 0))) 
		{
			// Get the pointer to the rest of data
			lpszToken = strtok(NULL, ":"); 

			// Get the pointer to the Type/SubType
			if((lpszToken = strtok(lpszToken, " \t;")) != NULL) {
				if (workitem->lpszContentType != NULL)
					delete [] workitem->lpszContentType;
				workitem->lpszContentType = new char[strlen(lpszToken)+1];
				strcpy(workitem->lpszContentType, lpszToken);

				// Loop throug all optioanl parameters (parameter := attribute "=" value)
				do{
					// Get the parameter attribute name
					lpszToken = strtok(NULL, "= \t;");
					if(lpszToken != NULL) {

						// Get the attribute type. And initialize the pointer to the value pointer
						char **lplpszValue = NULL;
						if(_stricmp(lpszToken, "NAME") == 0 || _stricmp(lpszToken, "filename") == 0)     
							lplpszValue = &workitem->lpszName;
						if(_stricmp(lpszToken, "CHARSET") == 0)  
							lplpszValue = &workitem->lpszCharSet;
						if(_stricmp(lpszToken, "TYPE") == 0)     
							lplpszValue = &workitem->lpszAttachType;

						// Get attribute value
						lpszToken = strtok(NULL, ";");
						if(lpszToken != NULL && lplpszValue != NULL) {
							*lplpszValue = new char[strlen(lpszToken) + 1];

							// Copy the value
							strcpy(*lplpszValue, lpszToken);

							// Remove extra spaces
							CUT_StrMethods::RemoveSpaces(*lplpszValue);

							// If the value is the quoted string - remove quotes
							if((*lplpszValue)[0] == '\"') {
								memmove(*lplpszValue, (*lplpszValue)+1, strlen(*lplpszValue)-1);
								int i = 0;
								while((*lplpszValue)[i] != 0 && (*lplpszValue)[i] != '\"')
									++i;

								(*lplpszValue)[i] = NULL;
							}
						}
					}
				}while (lpszToken != NULL) ;
			}

		}   

	}

	// Delete the data buffer
	delete [] lpszData;
}
/********************************
GetName
Gets the name of encoding/decoding
algorithm 
PARAM:
none
RETURN:
name "Mime"
*********************************/
LPCSTR	CUT_MimeEncode::GetName() const
{
	static LPCSTR lpName = "Mime";
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
int CUT_MimeEncode::GetAttachmentNumber() const
{
	int		count = 0;

	MIMEATTACHMENTLISTITEM	*item = m_ptrAttachList;

	while(item!=NULL) {
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
void CUT_MimeEncode::EmptyAttachmentList() {

	MIMEATTACHMENTLISTITEM	*item;

	while (m_ptrAttachList != NULL) {
		item = m_ptrAttachList->next;

		if(m_ptrAttachList->ptrDataSource)
			delete m_ptrAttachList->ptrDataSource;

		if (m_ptrAttachList->lpszName  != NULL)
			delete[] m_ptrAttachList->lpszName;

		if (m_ptrAttachList->lpszContentType  != NULL)
			delete[] m_ptrAttachList->lpszContentType;

		if (m_ptrAttachList->lpszCharSet  != NULL)
			delete[] m_ptrAttachList->lpszCharSet;

		if (m_ptrAttachList->lpszAttachType  != NULL)
			delete[] m_ptrAttachList->lpszAttachType;

		delete m_ptrAttachList;

		m_ptrAttachList = item;
	}
}

/********************************
GetMessageBodyText
Gets a meessage body text from the encoded message
PARAM:
dest - where to put the message text
[append]	- should we append it to the data source
RETURN:
UTE_SUCCESS			- success
UTE_ERROR			- error
UTE_DS_OPEN_FAILED	- failed to open data source
UTE_DS_WRITE_FAILED		- data source writing error
*********************************/
int CUT_MimeEncode::GetMessageBodyText(CUT_DataSource & dest, BOOL append) {

	// Try to find attachment with no name and Content-Type: "text/plain" or "text/html"
	MIMEATTACHMENTLISTITEM	*item = m_ptrAttachList, *prev_item = NULL;
	int						error = UTE_SUCCESS, index = 0;

	while( item != NULL) {
		if( (item->lpszName == NULL || item->lpszName[0] == 0) && 
			(
			(item->lpszContentType != NULL && _stricmp(item->lpszContentType, "text/plain") == 0) || 
			(item->lpszContentType != NULL && _stricmp(item->lpszContentType, "text/html") == 0) )
			) 
		{
			// Decode this attachment
			error = Decode(index, dest, append);

			if (item->lpszContentType != NULL && _stricmp(item->lpszContentType, "text/plain") == 0)
				m_bTextBody = TRUE;

			if (item->lpszContentType != NULL && _stricmp(item->lpszContentType, "text/html") == 0)
				m_bHTMLBody = TRUE;
			else 
				m_bHTMLBody = FALSE;

			
			// TD - v4.2 save last seen charset - message class will glean this after calling
			if(item->lpszCharSet != NULL && *item->lpszCharSet != '\0') {
				strcpy(m_lpszCharSet,item->lpszCharSet);
			}
			// Remove this attachment from the list
			if(item->ptrDataSource)
				delete item->ptrDataSource;
			if (item->lpszName  != NULL)
				delete[] item->lpszName;
			if (item->lpszContentType  != NULL)
				delete[] item->lpszContentType;
			if (item->lpszCharSet  != NULL)
				delete[] item->lpszCharSet;
			if (item->lpszAttachType  != NULL)
				delete[] item->lpszAttachType;

			if(item == m_ptrAttachList)
				m_ptrAttachList = item->next;
			else if(prev_item != NULL) 
				prev_item->next = item->next;

			delete item;

			break;
		}

		++ index;
		prev_item	= item;
		item		= item->next;
	}

	// In case of Base MIME
	if(m_gctMsgContenType.type != CUT_MIME_MULTIPART_CONTENT && error == UTE_ERROR) {
		error = UTE_SUCCESS;
	}

	return error;
}

/********************************
AddAttachment
Used in the process of encoding. Adds a new attachment 
to the attachment list. 
PARAM:
source	- datasource to be included
name	- name of the data source
[params]- pointer to the list of optional parameters
(0) - encoding type 7BIT, 8BIT, BINARY, BASE64, QUOTED_PRINTABLE, BINHEX.
(the value from the EncodeTypes enumeration converted to string)
(1)	- MIME compliant content type that the attachment will be identified by.
(2) - charset portion of the content type
(3)	- attachment type
bAddToTop - If true the attachment will be added to the top of the list. The default is FALSE.
RETURN:
UTE_SUCCESS			- success
UTE_DS_OPEN_FAILED	- failed to open attachment data source
*********************************/
int CUT_MimeEncode::AddAttachment(	CUT_DataSource & source, 
								  LPCSTR name,  
								  CUT_StringList *params,
								  BOOL bAddToTop) 
{
	LPCSTR		lpszStr;

	// verify that the supplied data source can be opened for reading
	if (source.Open(UTM_OM_READING) == -1)
		return UTE_DS_OPEN_FAILED;

	long	lSize = source.Seek(0, SEEK_END);

	source.Close();

	// Allocate new item
	MIMEATTACHMENTLISTITEM* item;

	if (bAddToTop)
	{
		item = m_ptrAttachList;
		m_ptrAttachList = new MIMEATTACHMENTLISTITEM; 
		m_ptrAttachList->next = item;
		item = m_ptrAttachList;
	}
	else // add to bottom
	{
		if (m_ptrAttachList == NULL)
		{
			m_ptrAttachList = new MIMEATTACHMENTLISTITEM;
			item = m_ptrAttachList;
			item->next = NULL;
		}
		else
		{
			item = m_ptrAttachList;
			while (item->next != NULL)
				item = item->next;
			item->next = new MIMEATTACHMENTLISTITEM;
			item = item->next;
			item->next = NULL;
		}
	}

	// Save pointer to the datasource in the list
	item->ptrDataSource = source.clone();

	// Save name of the data source
	if ( name != NULL ) {
		item->lpszName = new char[strlen(name) + 1];
		strcpy(item->lpszName, name);
	} 
	else
		item->lpszName = NULL;

	// Get encoding type from the parameters list
	item->nEncodeType = CUT_MIME_BASE64;
	if(params != NULL && (lpszStr = params->GetString(0L)) != NULL && strlen(lpszStr) > 0 ) {
		item->nEncodeType = atoi(lpszStr);
		if(item->nEncodeType < CUT_MIME_7BIT || item->nEncodeType >= CUT_MIME_MAX)
			item->nEncodeType = CUT_MIME_BASE64;
	} 

	// Get content type from the parameters list
	item->lpszContentType = NULL;
	if(params != NULL && (lpszStr = params->GetString(1)) != NULL && strlen(lpszStr) > 0) {
		item->lpszContentType = new char[strlen(lpszStr) + 1];
		strcpy(item->lpszContentType, lpszStr);
	} 

	// Get charset portion of the content type from the parameters list
	item->lpszCharSet = NULL;
	if(params != NULL && (lpszStr = params->GetString(2)) != NULL && strlen(lpszStr) > 0 ) {
		item->lpszCharSet = new char[strlen(lpszStr) + 1];
		strcpy(item->lpszCharSet, lpszStr);
	} 

	// Get attachment type from the parameters list
	item->lpszAttachType = NULL;
	if(params != NULL && (lpszStr = params->GetString(3)) != NULL && strlen(lpszStr) > 0 ) {
		item->lpszAttachType = new char[strlen(lpszStr) + 1];
		strcpy(item->lpszAttachType, lpszStr);
	} 

	item->lSize = lSize;

	item->ptrHeaders		= NULL;

	// used in the event that the user wants to 
	// add custom headers to individual attachments
	m_ptrCurrentAttach = item;

	m_nNumberOfAttachmentAdded++;

	return UTE_SUCCESS;
}


/********************************
AddFileHeaderTag
adds Custom Header information to The last added attachment.
After adding an attachment with the AddFile() member,
that attachment becomes the 'current' attachment.
Subseqent calls to AddHeaderTag() will add a list of 
'custom' headers to that's attachemnts list.  It is 
important to note that these 'custom' headers are added
for ONLY INDIVIDUAL attachments, and are not global to 
the entire message. 
By convention, custom headers are prefaced with 'X-".
PARAM:
tag			- custom header  to add
RETURN:
UTE_SUCCESS	- success
-1          - no current attachment.  AddHeaderTag()
was called before AddFile().            
*********************************/
int	CUT_MimeEncode::AddFileHeaderTag(LPCSTR tag)
{

	if (m_ptrCurrentAttach == NULL)
		return -1;

	if (m_ptrCurrentAttach->ptrHeaders == NULL) 
		m_ptrCurrentAttach->ptrHeaders = new CUT_StringList;

	m_ptrCurrentAttach->ptrHeaders->AddString(tag);

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
int CUT_MimeEncode::GetGlobalHeadersNumber() const
{
	return m_listGlobalHeader.GetCount();
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
LPCSTR CUT_MimeEncode::GetGlobalHeader(int index) const
{
	return m_listGlobalHeader.GetString(index);
}

/********************************
Encode
Encode list of attachments into the data source
PARAM:
dest		- destination data source
[append]	- should we append it to the data source
RETURN:
UTE_SUCCESS				- success
UTE_MSG_NO_ATTACHMENTS	- no attachments
UTE_DS_OPEN_FAILED		- failed to open data source
UTE_DS_WRITE_FAILED		- data source writing error
*********************************/
int CUT_MimeEncode::Encode(CUT_DataSource & msg_body, CUT_DataSource & dest, BOOL append) {

	MIMEATTACHMENTLISTITEM	*item = m_ptrAttachList;
	int						error = UTE_SUCCESS;

	char	header[100];

	// Open the datasource for writing
	if(dest.Open( (append) ? UTM_OM_APPEND : UTM_OM_WRITING) == -1)
		return UTE_DS_OPEN_FAILED;

	// Open message body datasource for reading
	if ( msg_body.Open(UTM_OM_READING) == -1 ) {
		dest.Close();
		return UTE_DS_OPEN_FAILED;				// problem opening data source
	}

	// If there is no attachments
	if ( item == NULL ) {

		// Write down the message body as is
		char	buffer[512];
		int		nCharsRead;
		while((nCharsRead=msg_body.Read(buffer, sizeof(buffer)-1)) > 0) 
			if(dest.Write(buffer, nCharsRead) != nCharsRead)
				error = UTE_DS_WRITE_FAILED;

		// Close destination source
		if (dest.Close() == -1)
			error = UTE_DS_CLOSE_FAILED;		// problem closing file
		// Close message source
		if (msg_body.Close() == -1)
			error = UTE_DS_CLOSE_FAILED;		// problem closing file

		return error;
	}

	// If there are attachments
	else
	{
		// Save message body as plain/text attachment without a name
		MIMEATTACHMENTLISTITEM* msg_body_item	= new MIMEATTACHMENTLISTITEM;
		m_ptrTextBodyAttachment = msg_body_item; // save the pointer to the text body attachment
		msg_body_item->next						= NULL;
		msg_body_item->ptrDataSource			= &msg_body;
		msg_body_item->lpszName					= new char[2];
		msg_body_item->lpszName[0]				= 0;
		msg_body_item->lpszContentType			= new char[15];
		if (m_bHTMLBody && !m_bTextBody)
		{
			msg_body_item->nEncodeType		= CUT_MIME_QUOTEDPRINTABLE;
			strcpy(msg_body_item->lpszContentType, "text/html");
		}
		else
		{
			msg_body_item->nEncodeType		= 	CUT_MIME_QUOTEDPRINTABLE;
			strcpy(msg_body_item->lpszContentType, "text/plain");
		}

		msg_body_item->lpszCharSet		= NULL;
		msg_body_item->lpszAttachType	= NULL;
		msg_body_item->ptrHeaders		= NULL;

		// Write MIME message body text
		char	msg[] = "This is a multi-part message in MIME format.\r\n";
		dest.Write(msg, strlen(msg));

		// Write boundary string 
		WriteBoundaryString(dest, m_szBoundaryString, BS_INTERMEDIATE);

		// If we have an html body part with no other attachments we need to separate the two
		if (m_ptrHtmlBodyAttachment != NULL && m_ptrTextBodyAttachment != NULL &&
			m_ptrHtmlBodyAttachment->next != NULL)
		{
			char szSubHeader[1024];
			_snprintf(szSubHeader, sizeof(szSubHeader) - 1, "Content-Type: Multipart/Alternative;\r\n\tboundary=\"%s\"\r\n",m_szSubBoundaryString);
			dest.Write(szSubHeader, strlen(szSubHeader));

			// Write the sub-boundary string
			WriteBoundaryString(dest, m_szSubBoundaryString, BS_INTERMEDIATE);
		}

		// Encode message body to the destination
		EncodeQuotedPrintable(msg_body_item, dest);

		// Clean up
		delete [] msg_body_item->lpszName;
		delete [] msg_body_item->lpszContentType;
		delete msg_body_item;
	}


	// loop through each of the attachments and construct each
	// message.  if there is more than one attachment, then we
	// need to construct a boundary marker.  Each file needs
	// to be serialized and added to the existing stream.
	while (item != NULL && error == UTE_SUCCESS)
	{
		// If this is the html attachment and there is a text attachment
		// put the sub-boundary string here
		if (item == m_ptrHtmlBodyAttachment && m_ptrTextBodyAttachment != NULL && m_ptrHtmlBodyAttachment->next != NULL)
			WriteBoundaryString(dest, m_szSubBoundaryString, BS_INTERMEDIATE);
		else
			WriteBoundaryString(dest, m_szBoundaryString, BS_INTERMEDIATE);


		// If the data source for the attachment is not set it means
		// that we are trying to encode already encoded attachment.
		if(item->ptrDataSource == NULL && m_ptrDecodeDataSource != NULL &&
			item->lContentLength > 0 && item->lOffset > 0 ) {

				if(m_ptrDecodeDataSource->Open(UTM_OM_READING) != -1) {

					// Find the begining of the attachment header
					long    nCurOffset = max(item->lOffset - 1000 - 4, 0);
					long    lReturn, lLastFoundPos = 0;
					for(int i = 1; lLastFoundPos == 0 && i <= 10; i++) {
						nCurOffset = max(item->lOffset - 1000*i - 4, 0);
						while((lReturn = m_ptrDecodeDataSource->Find("\r\n\r\n", nCurOffset, FALSE, 1000)) > 0) {
							if(lReturn >= item->lOffset - 4)
								break;
							lLastFoundPos = lReturn;
							nCurOffset = lReturn + 2;
						}
					}

					if(lLastFoundPos == 0)
						lLastFoundPos = item->lOffset;
					else
					{
						// Skip old boundary string
						int iOldStringLength = (int)::strlen(m_szMainBoundry);
						if (iOldStringLength > 0)
						{
							lReturn = m_ptrDecodeDataSource->Find(m_szMainBoundry, lLastFoundPos, FALSE);
							if (lReturn > 0)
								lLastFoundPos = lReturn + iOldStringLength;
						}

						// Skip old marker
						if((lReturn = m_ptrDecodeDataSource->Find("\r\n", lLastFoundPos, FALSE)) > 0) 
							lLastFoundPos = lReturn + 2;
					}

					if(m_ptrDecodeDataSource->Seek(lLastFoundPos, SEEK_SET) == lLastFoundPos) {

						// In this case just copy the encoded attachment to the destination
						char    szBuffer[MAX_LINE_LENGTH + 1];
						int     nBytesRead ,nBytesToRead = item->lContentLength + (item->lOffset-lLastFoundPos);

						// Copy encoded data to the destination
						while((nBytesRead=m_ptrDecodeDataSource->Read(szBuffer, MAX_LINE_LENGTH)) > 0) {
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
					m_ptrDecodeDataSource->Close();

					// Go to the next item
					item=item->next;
					continue;
				}
		}

		switch(item->nEncodeType) {
			case CUT_MIME_7BIT:         // 7 bit
				error = Encode7bit(item, dest);
				break;
			case CUT_MIME_BASE64:       // base 64
				error = EncodeBase64(item, dest);
				break;
			case CUT_MIME_QUOTEDPRINTABLE:      // quoted printable
				error = EncodeQuotedPrintable(item, dest);
				break;
			case CUT_MIME_8BIT:         // 8 bit
				error = Encode8bit(item, dest);
				break;
			case CUT_MIME_BINARY:       // binary
				error = EncodeBinary(item, dest);
				break;
			case CUT_MIME_BINHEX40:     // BinHex format (NOT SUPPORTED, Here to allow for future integration) 
				error = EncodeBinHex(item, dest);
				break;
		}

		// If this is the html attachment and there is a text attachment
		// put the sub-boundary string here
		if (item == m_ptrHtmlBodyAttachment && m_ptrTextBodyAttachment != NULL && m_ptrHtmlBodyAttachment->next != NULL)
			WriteBoundaryString(dest, m_szSubBoundaryString, BS_ENDOFFILE);

		item=item->next;
	}

	error = WriteBoundaryString(dest, m_szBoundaryString, BS_ENDOFFILE);

	if (dest.Close() == -1)
		return UTE_DS_CLOSE_FAILED;      // problem closing data source

	if (msg_body.Close() == -1)
		return UTE_DS_CLOSE_FAILED;      // problem closing data source



	// Fill in global headers
	EmptyGlobalHeaders();   
	_snprintf(header,sizeof(header)-1, "MIME-Version: 1.0\r\n");
	EncodeAddGlobalHeader(header);

	// By default the global content type is multipart/mixed. However if we have
	// plain text body and html body but no attachments we need to change it to
	// multipart/alternative
	if (GetAttachmentNumber() == 1 && m_ptrHtmlBodyAttachment != NULL)
	{
		_snprintf(header,sizeof(header)-1, "Content-Type: Multipart/Alternative;\r\n\tboundary=\"%s\"\r\n",m_szBoundaryString);
		EncodeAddGlobalHeader(header);
	}
	else
	{
		_snprintf(header,sizeof(header)-1, "Content-Type: Multipart/Mixed;\r\n\tboundary=\"%s\"\r\n", m_szBoundaryString);
		EncodeAddGlobalHeader(header);
	}

	return error;
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
int CUT_MimeEncode::SetDecodeSource(CUT_DataSource & source) 
{

	long	firstBlankLinePos;

	m_nNumberOfAttachmentFound = 0;
	BOOL    multiPart;

	MIMEATTACHMENTLISTITEM item;

	// Sets the data source
	if(m_ptrDecodeDataSource)
	{

		delete m_ptrDecodeDataSource;
		EmptyAttachmentList();
		EmptyGlobalHeaders();
	}
	m_ptrDecodeDataSource = source.clone();

	// Open data source
	if(m_ptrDecodeDataSource->Open(UTM_OM_READING) == -1)
		return UTE_DS_OPEN_FAILED;

	// Ensure this is a MIME Encoded File
	if (!CanDecode()) 
	{
		delete m_ptrDecodeDataSource;
		m_ptrDecodeDataSource = NULL;
		return UTE_ERROR;				// Not a Mime datasource
	}

	DecodeGetGlobalContentType(0, m_gctMsgContenType);

	if (m_gctMsgContenType.type == CUT_MIME_UNRECOGNIZED)
		return UTE_ERROR;// Malformed file, this could be caused by list mailer not allowing attachment so the 
	//header could have stayed there as MIME but the content have been modified

	// Now we have detremined that it is a mime encoded message lets find the boundries
	// and the content type
	// Find the content type of the whole message or entity

	// find position of first blank file
	firstBlankLinePos = m_ptrDecodeDataSource->Find("\r\n\r\n");
	if (firstBlankLinePos < 0)
		return UTE_ERROR;// Malformed file, no space between header and content found

	// search for "Multipart" in message header to determine if this is a multipart file
	// by using the "Multipart/" string, we trap "Multipart/Mixed", Multipart/Alternative, etc.

	if (m_gctMsgContenType.type != CUT_MIME_MULTIPART_CONTENT )
	{
		multiPart = FALSE;
	} 
	else 
	{ 
		multiPart = TRUE;
		// Get The Global boundary String
		// m_szMainBoundry
		if (GetMultiPartBoundryString (0,firstBlankLinePos,m_szMainBoundry) != UTE_SUCCESS)
			return UTE_ERROR;

	}

	// initialize the structure
	item.ptrDataSource	= NULL;
	item.lpszName		= NULL;
	item.lpszContentType	= NULL;
	item.lpszCharSet		= NULL;
	item.lpszAttachType		= NULL;
	item.lSize              = 0;
	item.next           = NULL;
	if (multiPart)
	{
		// find the Multipart entities if Multipart Mixed
		// IF MIXED 
		LocateMultiPartEntities(m_szMainBoundry);
		// ELSE ALTERNATIVE
		// decode each 

	} 
	else 
	{
		m_nNumberOfAttachmentFound = 1;
		item.lOffset = m_ptrDecodeDataSource->Find("\r\n\r\n",0);
		item.lContentLength = m_ptrDecodeDataSource->Seek(0, SEEK_END); 
		AddDecodeFileInfo(&item);
	}

	// find each attachment, and determine it content-type, content-transfer-encoding and 
	// other pertinent details.  If header information is not present in an attachment, 
	// we need to apply defaults as outlined in RFCs 2045-2049
	// CALL  GetEntityInformation(); 
	GetEntityInformation();

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
BOOL CUT_MimeEncode::CanDecode() {

	long	firstBlankLinePos, filePos;

	if (m_ptrDecodeDataSource == NULL)
		return FALSE;						// decode data source was not set

	// seek the beginning of the file to be decoded
	m_ptrDecodeDataSource->Seek(0, SEEK_SET);

	// find position of first blank file
	firstBlankLinePos = m_ptrDecodeDataSource->Find("\r\n\r\n");
	if (firstBlankLinePos < 0)
		return FALSE;						// Malformed file


	// Look for MIME 1.0 string to ensure that we're working
	// on the appropriate type of file 
	filePos = m_ptrDecodeDataSource->Find("MIME-Version: 1.0", 0, FALSE, firstBlankLinePos);

	if (filePos < 0)
		return FALSE;       // Not a MIME file

	return TRUE;
}

/********************************
GetAttachmentInfo
Gets attachment info
PARAM:
index		- index of the attachment
name 		- name of the attachment
params		- list of additional parameters
(0) - attachment encoding type
maxsize		- maxsize of buffers
RETURN:
UTE_SUCCESS	- success
UTE_ERROR	- fail
*********************************/
int CUT_MimeEncode::GetAttachmentInfo(	int index, 
									  LPSTR name, 
									  int namesize,
									  LPSTR type,
									  int typesize,
									  CUT_StringList *params) const
{
	int		count = 0;


	MIMEATTACHMENTLISTITEM	*item = m_ptrAttachList;

	while( (item != NULL) && (count != index) ) {
		count++;
		item = item->next;
	}

	// index out of range
	if (item == NULL)
		return UTE_ERROR;          

	// Get the attachment name
	if(name != NULL) {
		if ( item->lpszName != NULL )
			strncpy(name, item->lpszName, namesize);
		else
			strncpy(name, "", namesize);
	}

	// Get the attachment content type
	if(type != NULL) {
		if ( item->lpszContentType != NULL )
			strncpy(type, item->lpszContentType, typesize);
		else
			strncpy(type, "", typesize);
	}

	// Get the attachment encoding type
	if(params != NULL) {
		char	buffer[10];

		params->ClearList();
		_itoa(item->nEncodeType, buffer, 10);
		params->AddString(buffer);
	}	

	return UTE_SUCCESS;
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
int CUT_MimeEncode::Decode(int index, CUT_DataSource & dest, BOOL append) 
{
	int	count	= 0;
	int	retval	= UTE_SUCCESS;

	// Decode data source not set
	if(m_ptrDecodeDataSource == NULL)
		return UTE_DS_OPEN_FAILED;

	MIMEATTACHMENTLISTITEM	*item = m_ptrAttachList;
	while(item!=NULL && count != index) {
		count++;
		item=item->next;
	}

	if (item == NULL)
		return UTE_INDEX_OUTOFRANGE;           // index out of range

	// Open dest data source
	if(dest.Open( (append) ? UTM_OM_APPEND : UTM_OM_WRITING) == -1)
		return UTE_DS_OPEN_FAILED;

	switch(item->nEncodeType){
		case CUT_MIME_7BIT:           // 7 bit
			retval = Decode7bit(item, dest);
			break;
		case CUT_MIME_BASE64:     // base 64
			retval = DecodeBase64(item, dest);
			break;
		case CUT_MIME_QUOTEDPRINTABLE:        // quoted printable
			retval = DecodeQuotedPrintable(item, dest);
			break;
		case CUT_MIME_8BIT:           // 8 bit
			retval = Decode8bit(item, dest);
			break;
		case CUT_MIME_BINARY:     // binary
			retval = DecodeBinary(item, dest);
			break;
		case CUT_MIME_BINHEX40:       // BinHex format (NOT SUPPORTED, Here to allow for future integration)
			retval = DecodeBinHex(item, dest);
			break;
		case CUT_MIME_MULTIPART:  // nested Multipart attachment
			// sending this type of file to the binary section should work
			// properly.  The resulting file chunk can be simply fed back into
			// the decoding routine again to get the nested attachments.
			retval = DecodeBinary(item, dest); 
			break;
	}

	// Close data sources
	dest.Close();

	return retval;
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
long CUT_MimeEncode::GetAttachmentSize(int index) const
{
	int					count	= 0;
	long				size	= 0;
	MIMEATTACHMENTLISTITEM	*item = m_ptrAttachList;

	while( item != NULL ) {
		if(count == index) 
			return (item->lSize == 0 && item->ptrDataSource == NULL) ? (long)(item->lContentLength*0.75) : item->lSize;

		size += (item->lSize == 0 && item->ptrDataSource == NULL) ? (long)(item->lContentLength*0.75) : item->lSize;
		count ++;
		item = item->next;
	}

	return size;
}
/************************************************************
Function name	: CUT_MimeEncode::DecodeGetGlobalContentType
Description	    : 
Return type		: BOOL  
Argument         : long startingPos
Argument         : GlobalContentType &gctMsgContenType
Syntax of the Content-Type Header Field
In the Augmented BNF notation of RFC 822, a Content-Type header field
value is defined as follows:

content := "Content-Type" ":" type "/" subtype
*(";" parameter)
; Matching of media type and subtype
; is ALWAYS case-insensitive.

type := discrete-type / composite-type

discrete-type := "text" / "image" / "audio" / "video" /
"application" / extension-token

composite-type := "message" / "multipart" / extension-token

extension-token := ietf-token / x-token

ietf-token := <An extension token defined by a
standards-track RFC and registered
with IANA.>

x-token := <The two characters "X-" or "x-" followed, with
no intervening white space, by any token>

subtype := extension-token / iana-token

iana-token := <A publicly-defined extension token. Tokens
of this form must be registered with IANA
as specified in RFC 2048.>

parameter := attribute "=" value

attribute := token
; Matching of attributes
; is ALWAYS case-insensitive.

value := token / quoted-string

token := 1*<any (US-ASCII) CHAR except SPACE, CTLs,
or tspecials>

tspecials :=  "(" / ")" / "<" / ">" / "@" /
"," / ";" / ":" / "\" / <">
"/" / "[" / "]" / "?" / "="
; Must be in quoted-string,
; to use within parameter values

***************************************************************/
BOOL  CUT_MimeEncode::DecodeGetGlobalContentType(long startingPos,GlobalContentType &gctMsgContenType  )
{
	long	firstBlankLinePos, filePos;

	if (m_ptrDecodeDataSource == NULL)
		return FALSE;						// decode data source was not set

	// seek the beginning of the file to be decoded
	m_ptrDecodeDataSource->Seek(startingPos, SEEK_SET);

	// find position of first blank file
	firstBlankLinePos = m_ptrDecodeDataSource->Find("\r\n\r\n");
	if (firstBlankLinePos < 0)
		return FALSE;						// Malformed file


	// Look for MIME 1.0 string to ensure that we're working
	// on the appropriate type of file 
	filePos = m_ptrDecodeDataSource->Find("Content-Type: ", 0, FALSE, firstBlankLinePos);

	/*
	Mod by Nish - April 21, 2005
	Notes - Certain mail servers might insert a malformed Content-Type header, 
	and while we should actually be tagging this as an error and
	returning false, some mail clients (mostly from MS) ignore this error
	and continue to parse the mail normally. This fix is done to emulate
	that behavior - and may be removed in a future version.
	*/

	if(filePos == -1) //Search again without a space after the : (malformed - see RFC 2045)
		filePos = m_ptrDecodeDataSource->Find("Content-Type:", 0, FALSE, firstBlankLinePos);	

	if (filePos < 0)
		return FALSE;       // Not a MIME file
	m_ptrDecodeDataSource->Seek (filePos, SEEK_SET);
	char buf[MAX_PATH+1];
	char type[MAX_PATH+1];
	char sub[MAX_PATH+1];
	if (m_ptrDecodeDataSource->ReadLine (buf,MAX_PATH-1)>0)
	{
		if (strlen(buf) ==0)
			return FALSE;
		else{
			if (CUT_StrMethods::ParseString (buf,":/;",1,type,MAX_PATH-1) != UTE_SUCCESS)
			{
				// return the file pointer back to the begining
				m_ptrDecodeDataSource->Seek(startingPos, SEEK_SET);
				return FALSE;
			}

			if (CUT_StrMethods::ParseString (buf,":/;",2,sub,MAX_PATH-1)!= UTE_SUCCESS)
			{
				// return the file pointer back to the begining
				m_ptrDecodeDataSource->Seek(startingPos, SEEK_SET);
				return FALSE;
			}

			// Truncate spaces
			CUT_StrMethods::RemoveSpaces(type);
			CUT_StrMethods::RemoveSpaces(sub);

			if (_stricmp(type,"multipart") == 0)
			{
				gctMsgContenType.type = CUT_MIME_MULTIPART_CONTENT;
				if (_stricmp(sub,"alternative") == 0)
				{
					gctMsgContenType.sub = CUT_MIME_MULTIPART_ALTERNATIVE;
				}
				else if (_stricmp(sub,"parallel") == 0)
				{
					gctMsgContenType.sub = CUT_MIME_MULTIPART_PARALLEL;

				}
				else if (_stricmp(sub,"digest")== 0)
				{
					gctMsgContenType.sub = CUT_MIME_MULTIPART_DIGEST;
				}
				else if (_stricmp(sub,"related")== 0)
				{
					gctMsgContenType.sub = CUT_MIME_MULTIPART_RELATED;
				}				
				else // the default is mixed ...hmmm! lets check and make sure for later on 
				{
					gctMsgContenType.sub = CUT_MIME_MULTIPART_MIXED;
				}

			}
			else if (_stricmp(type,"message") == 0)
			{
				gctMsgContenType.type = CUT_MIME_MESSAGE ;
				if (_stricmp(sub,"digest") == 0)
				{
					gctMsgContenType.sub = CUT_MIME_MULTIPART_DIGEST;
				}
				else  if (_stricmp(sub,"partial") == 0)
				{
					gctMsgContenType.sub = CUT_MIME_MESSAGE_PARTIAL;

				}
				else //  (stricmp(sub,"rfc822")== 0)  this is the default
				{
					gctMsgContenType.sub = CUT_MIME_MESSAGE_RFC822;

				}		

			}
			else if (_stricmp(type,"text") == 0) //text
			{
				gctMsgContenType.type = CUT_MIME_TEXT ;
				/*

				// RFC 2046 did not define other subtypes for text (See section 4.1.3   of RFC 2046)
				if (stricmp(sub,"html")== 0)   
				{
				gctMsgContenType.sub = CUT_MIME_TEXT_HTML;
				}else
				*/
				gctMsgContenType.sub = CUT_MIME_TEXT_PLAIN;
			}
			else if (_stricmp(type,"application") == 0 ) 
			{  // application  we will recognize all image types as applications (see section 4.2 RFC 2046)
				gctMsgContenType.type = CUT_MIME_APPLICATION;
				gctMsgContenType.sub = CUT_MIME_NA;
			}
			else if (_stricmp(type,"image") == 0 ) 
			{  // application  we will recognize all image types as applications (see section 4.2 RFC 2046)
				gctMsgContenType.type = CUT_MIME_IMAGE;
				gctMsgContenType.sub = CUT_MIME_NA;
			}
			else if (_stricmp(type,"video") == 0)
			{
				gctMsgContenType.type = CUT_MIME_VIDEO;
				gctMsgContenType.sub = CUT_MIME_NA;  // initial should be MPEG
			}
			else if (_stricmp(type,"audio") == 0)
			{
				gctMsgContenType.type = CUT_MIME_AUDIO;
				gctMsgContenType.sub = CUT_MIME_NA;  // initial should be BASIC
			}
			else
			{
				gctMsgContenType.type = CUT_MIME_UNRECOGNIZED;
				gctMsgContenType.sub = CUT_MIME_NA;  // initial should be BASIC

			}

		}
	}

	// return the file pointer back to the begining
	m_ptrDecodeDataSource->Seek(startingPos, SEEK_SET);
	return TRUE;


}
/************************************************************************************************
// Function name	: 
// Description	    : 
// Return type		: int CUT_MimeEncode::GetMultiPartBoundryString 
// Argument         :  int startPos
// Argument         : int EndPosition
// Argument         : char *boundaryString
Find the boundry string within the specified block
************************************************************************************************/
int CUT_MimeEncode::GetMultiPartBoundryString ( int startPos,int EndPosition, char *boundaryString)
{
	int filePos  = 0;
	char    buf[200];
	char *token;
	filePos = m_ptrDecodeDataSource->Find("Multipart/", startPos, FALSE,EndPosition );    
	//Determine the boundaryString for the multipart file.
	//Bug-fix by Nish - Apr 21, 2005 : was sending a -ve number as search limit
	filePos = m_ptrDecodeDataSource->Find("boundary", filePos, FALSE, EndPosition-filePos);

	if (filePos < 0)
		return UTE_ERROR;   // malformed message.

	m_ptrDecodeDataSource->Seek(filePos, SEEK_SET);
	int length = min(startPos - filePos, (sizeof(buf)-1) );
	int reading;
	// Reading source data
	reading = m_ptrDecodeDataSource->Read(buf, length);
	buf[reading] = '\0';
	token = new char [reading];
	if (CUT_StrMethods::ParseString (buf,"\"\r",1,token,reading)!= UTE_SUCCESS)
	{
		delete token;
		return UTE_ERROR;
	}

	BOOL fSpaces = TRUE;

	// now lets see if we have spaces only 
	for (UINT nCounter = 0; nCounter < strlen(token) ; nCounter++)
	{
		// if not a space then we have good string
		if (!isspace(*token+nCounter))
		{
			fSpaces = FALSE;
			break;
		}

	}
	if (fSpaces)
	{
		if (CUT_StrMethods::ParseString (buf,"\"\r",2,token,reading)!= UTE_SUCCESS)
		{
			delete token;
			return UTE_ERROR;
		}
	}
	strcpy(boundaryString, "\r\n--");
	strcat(boundaryString,token);
	delete token;

	return UTE_SUCCESS;
}
/******************************************************************
Function name	: CUT_MimeEncode::LocateMultiPartEntities
Description	    :  
Locate each part if this is a multipart attachment and create 
a slot for it so we can populate it in the 
GetEntityInformation(int filePos , int Index)

Return type		: int 
Argument         : char *boundaryString	
the boundry string we are looking for 

*******************************************************************/
int CUT_MimeEncode::LocateMultiPartEntities( char *boundaryString, int startingPos){

	long top;   
	int boundaryStringLen=0;
	int filePos = startingPos;     
	int counter = 0;
	MIMEATTACHMENTLISTITEM item;

	// initialize the structure
	item.ptrDataSource	= NULL;
	item.lpszName		= NULL;
	item.lpszContentType	= NULL;
	item.lpszCharSet		= NULL;
	item.lpszAttachType		= NULL;
	item.lSize          = 0;



	filePos = m_ptrDecodeDataSource->Find(boundaryString, filePos);
	boundaryStringLen = (int)strlen(boundaryString);
	filePos += boundaryStringLen; // advance to the end of the boundary string
	top = filePos;      

	// loop through the input file and map out the boundary markers
	// add each entry into the m_decodeInfo linked list.
	do
	{     
		filePos = m_ptrDecodeDataSource->Find( boundaryString, filePos );

		if (filePos < 0)      // we're at the end of the file
			break;

		item.lOffset = top;

		item.lContentLength = filePos - top;

		AddDecodeFileInfo( &item );

		m_nNumberOfAttachmentFound++;

		filePos += boundaryStringLen;   
		top = filePos; 

		counter++;
	}while (filePos > 0 );
	return counter;

}
/*******************************************************

Function name	: CUT_MimeEncode::GetEntityInformation
Description	    : 
find each attachment, and determine it content-type,
content-transfer-encoding and 
other pertinent details.  If header information is 
not present in an attachment, we need to apply 
defaults as outlined in RFCs 2045-2049
For each part of the attachment(s) lets loop through
them	and find where are they exactly in the 
DataSource and if posible 	lets get the encoding type
for each part of the attachments.
If the message 	contains only one attachment this 
function still works

Return type		: int 
Argument         : int filePos
Argument         : int Index


*****************************************************************/
int CUT_MimeEncode::GetEntityInformation(int filePos , int Index){


	char	buf[200],*token;
	MIMEATTACHMENTLISTITEM *workitem, *previtem = NULL;
	int boundaryStringLen = (int)strlen(m_szMainBoundry);
	BOOL includesAlternatives = FALSE ;
	int		alternativeStart = 0;
	int		alternativeEnd = 0;
	int		alterIndex = 0;
	int     reductionInContentLength = 0;

	for ( int xy= Index; xy < m_nNumberOfAttachmentFound; xy++ ) 
	{

		GetDecodeListItem(xy, &workitem);

		// with the addition of the next lines we don't have to assume 
		// the message is encoded in Multi-part MIME 
		// Now we should be able to encode Basic MIME messages too

		//  Parse Content-type: header field if present
		// find 'content type' in this attachment       
		if ( m_gctMsgContenType.type != CUT_MIME_MULTIPART_CONTENT ) 
		{
			//workitem->lOffset = 0;
			//workitem->lContentLength = m_ptrAttachList->lOffset;
			filePos = m_ptrDecodeDataSource->Find("Content-type:", 0, FALSE, 0);
			DecodeGetAttachmentInfo(workitem,filePos);
		}		
		else 
		{
			filePos = m_ptrDecodeDataSource->Find("Content-", workitem->lOffset, 
				FALSE, workitem->lContentLength);

			if (filePos<0 || filePos > workitem->lOffset + workitem->lContentLength)
			{
				// there is no header for this attachment
				workitem->lpszContentType = new char[12];
				workitem->lpszCharSet = new char[12];
				strcpy(workitem->lpszContentType, "Text/Plain");
				strcpy(workitem->lpszCharSet, "US-ASCII");
			}
			else
				DecodeGetAttachmentInfo(workitem,filePos);
		}
		// v4.2 added check for multipart/mixed
		if((workitem->lpszContentType && _stricmp(workitem->lpszContentType, "multipart/alternative") == 0) ||
			(workitem->lpszContentType && _stricmp(workitem->lpszContentType, "multipart/related") == 0) ||
			(workitem->lpszContentType && _stricmp(workitem->lpszContentType, "multipart/mixed") == 0))
		{
			//  RFC 2046
			//	5.1.4.  Alternative Subtype
			//		The "multipart/alternative" type is syntactically identical to
			//		"multipart/mixed", but the semantics are different.  In particular,
			//		each of the body parts is an "alternative" version of the same
			//		information.
			// we will deal with it at the end  we just need to keep track of where did we find it and we did we add
			// it to the list
			includesAlternatives = TRUE;
			filePos = filePos+1 ; // increment the file position pointer
			alternativeStart = workitem->lOffset ;
			alternativeEnd   = workitem->lOffset + workitem->lContentLength;
			filePos = alternativeEnd;
			// lets keep in mind which item is the one that contained the 
			// alternative part
			alterIndex = xy;

			continue ;

		}
		else // 
		{
			//  Parse Content-transfer-encoding 
			//  header field if present
			// find 'content-transfer-encoding'
			if ( m_gctMsgContenType.type != CUT_MIME_MULTIPART_CONTENT ) 
			{
				filePos = m_ptrDecodeDataSource->Find("Content-transfer-encoding:", 0, 
					FALSE, 0);
			}		
			else
			{
				filePos = m_ptrDecodeDataSource->Find("Content-transfer-encoding:", workitem->lOffset, 
					FALSE, workitem->lContentLength);
			}

			// if not found, assume 7Bit encoding as per RFC2045 section 6.1
			if (filePos < 0 || (m_gctMsgContenType.type == CUT_MIME_MULTIPART_CONTENT && 
				filePos > workitem->lOffset + workitem->lContentLength)) 
			{
				workitem->nEncodeType = CUT_MIME_7BIT;
			} 
			else if (filePos >= 0 )
			{

				m_ptrDecodeDataSource->Seek(filePos, SEEK_SET);
				m_ptrDecodeDataSource->Read(buf, 100);

				token = strtok(buf, " /\r\n:");      // this should be the content line
				token = strtok(NULL, " /\r\n");     // this should be the encodeType

				if(token != NULL) 
				{

					_strupr(token);

					int bMatchedYet = FALSE;

					if (!bMatchedYet && strcmp(token, "8BIT") == 0) 
					{
						workitem->nEncodeType = CUT_MIME_8BIT;
						bMatchedYet = TRUE;
					}

					else if (!bMatchedYet && strcmp(token, "BINARY") == 0) 
					{
						workitem->nEncodeType = CUT_MIME_BINARY;
						bMatchedYet = TRUE;
					}

					else if (!bMatchedYet && strcmp(token, "BASE64") == 0) 
					{
						workitem->nEncodeType = CUT_MIME_BASE64;
						bMatchedYet = TRUE;
					}

					else if (!bMatchedYet && strcmp(token, "QUOTED-PRINTABLE") == 0) 
					{
						workitem->nEncodeType = CUT_MIME_QUOTEDPRINTABLE;
						bMatchedYet = TRUE;
					}

					else if (!bMatchedYet && strcmp(token, "BINHEX40") == 0) 
					{
						workitem->nEncodeType = CUT_MIME_BINHEX40;
						bMatchedYet = TRUE;
					}

					else if (!bMatchedYet && strcmp(token, "MULTIPART") == 0) 
					{
						// this is included to handle nested multipart attachments
						workitem->nEncodeType = CUT_MIME_MULTIPART;
						bMatchedYet = TRUE;
					}
					// default operation
					else // (!bMatchedYet)
						workitem->nEncodeType = CUT_MIME_7BIT;
				}            
			}

			//  Complete entries into m_ptrAttachList
			//  linked list.

			filePos = m_ptrDecodeDataSource->Find("\r\n\r\n", 
				workitem->lOffset, FALSE, 
				workitem->lContentLength);

			// if a blank line is present in the attachment, it will mark the separation
			// between the attachment header, and the attachment body.  If a blank line
			// is not present (which is technically not possible under the RFC, then
			// we will assume that the body starts immediately after the boundary marker.

			if (filePos > 0)
			{
				reductionInContentLength = ((filePos+4) - workitem->lOffset);
				workitem->lContentLength = workitem->lContentLength - reductionInContentLength;

				workitem->lOffset = filePos+4;
			} 
			else 
			{
				// this branch is not possible under the terms of the RFC, but is included for robustness.
				workitem->lContentLength = workitem->lContentLength - boundaryStringLen;
				workitem->lOffset = workitem->lOffset + boundaryStringLen;
			}
		}

		previtem = workitem;
	}
	// If we have alternative as part of this attachment then lets go back and fix evry thing
	// the item indexed by GetDecodeListItem(xy, &workitem);
	// is not yet populated as part of the attachemnts
	// so to do this 
	if (includesAlternatives)
	{
		DecodeAlternatives(alternativeStart, alternativeEnd,alterIndex);
	}

	return -1;
}


/*********************************************************

Function name	: CUT_MimeEncode::DecodeAlternatives
Description	    : 
Return type		: int 
Argument         : int alternativeStart 
The starting point of the alternative part block
Argument         : int alternativeEnd
The end point of the alternative part block
Argument         : int alterIndex 
the index of of the item we have added when we discovred that
the message contains alternative parts so we can modify it

(FRC 2046)
" 5.1.4.  Alternative Subtype

The "multipart/alternative" type is syntactically identical to
"multipart/mixed", but the semantics are different.  In particular,
each of the body parts is an "alternative" version of the same
information.


NOTE: From an implementor's perspective, it might seem more sensible
to reverse this ordering, and have the plainest alternative last.
However, placing the plainest alternative first is the friendliest
possible option when "multipart/alternative" entities are viewed
using a non-MIME-conformant viewer.  While this approach does impose
some burden on conformant MIME viewers, interoperability with older
mail readers was deemed to be more important in this case.

It may be the case that some user agents, if they can recognize more
than one of the formats, will prefer to offer the user the choice of
which format to view.  This makes sense, for example, if a message
includes both a nicely- formatted image version and an easily-edited
text version.  What is most critical, however, is that the user not
automatically be shown multiple versions of the same data.  Either
the user should be shown the last recognized version or should be
given the choice.

THE SEMANTICS OF CONTENT-ID IN MULTIPART/ALTERNATIVE:  Each part of a
"multipart/alternative" entity represents the same data, but the
mappings between the two are not necessarily without information
loss.  For example, information is lost when translating ODA to
PostScript or plain text.  It is recommended that each part should
have a different Content-ID value in the case where the information
content of the two parts is not identical.  And when the information
content is identical -- for example, where several parts of type
"message/external-body" specify alternate ways to access the
identical data -- the same Content-ID field value should be used, to
optimize any caching mechanisms that might be present on the
recipient's end.  However, the Content-ID values used by the parts
should NOT be the same Content-ID value that describes the
"multipart/alternative" as a whole, if there is any such Content-ID
field.  That is, one Content-ID value will refer to the
"multipart/alternative" entity, while one or more other Content-ID
values will refer to the parts inside it.
"
According to this section we will be decoding the simplest and most user friendly.
if we decode the first version of the alternative parts. 

*********************************************************/
int CUT_MimeEncode::DecodeAlternatives(int alternativeStart,int alternativeEnd, int alterIndex)
{
	CUT_ComplexMimeNode node;
	CUT_MimeEncode *_this = this;
	node.DecodeComplex (*_this,alternativeStart, alternativeEnd ,  alterIndex);
	return 1;
}

CUT_ComplexMimeNode::CUT_ComplexMimeNode()
{

}

CUT_ComplexMimeNode::~CUT_ComplexMimeNode()
{

}


int CUT_ComplexMimeNode::DecodeComplex( CUT_MimeEncode &mime,int alternativeStart,int alternativeEnd, int alterIndex)
{

	char	buf[200],*token;
	char m_szAltrnvBoundry[200];
	//int reductionInContentLength = 0;
	int boundaryStringLen = 0;

	MIMEATTACHMENTLISTITEM	*item = mime.m_ptrAttachList, *prev_item = NULL, *workitem;


	int numberAttachBeforeAlterNative = mime.m_nNumberOfAttachmentFound ;
	mime.GetMultiPartBoundryString (alternativeStart,alternativeEnd,m_szAltrnvBoundry);
	boundaryStringLen = (int)strlen(m_szAltrnvBoundry);
	alternativeStart = alternativeStart + boundaryStringLen ;

	// get the inner boundry string for the alternative parts
	// at the point we have found the alternative type
	mime.LocateMultiPartEntities(m_szAltrnvBoundry,alternativeStart);

	//	int numberNewAttachAdded = mime.m_nNumberOfAttachmentFound - numberAttachBeforeAlterNative;

	for (int ii = numberAttachBeforeAlterNative ; ii < mime.m_nNumberOfAttachmentFound; ii++)
	{
		mime.GetDecodeListItem(ii, &workitem);

		//  Parse Content-type: header field if present
		// find 'content type' in this attachment       

		alternativeStart = mime.m_ptrDecodeDataSource->Find("Content-type:", alternativeStart, FALSE, workitem->lContentLength);

		if (alternativeStart<0 || alternativeStart > workitem->lOffset + workitem->lContentLength)
		{
			// there is no header for this attachment
			// then we will use the default as stated by RFC 2045
			workitem->lpszContentType = new char[12];
			workitem->lpszCharSet = new char[12];
			strcpy(workitem->lpszContentType, "Text/Plain");
			strcpy(workitem->lpszCharSet, "US-ASCII");
		}
		else  // otherwise lets get the information from the file at that position 
			mime.DecodeGetAttachmentInfo(workitem,alternativeStart);



		//  Parse Content-transfer-encoding 
		//  header field if present
		// find 'content-transfer-encoding'
		int posiTion =  mime.m_ptrDecodeDataSource->Find("Content-transfer-encoding:", workitem->lOffset, 
			FALSE, workitem->lContentLength);
		if (posiTion > 0  && alternativeEnd > posiTion )
			alternativeStart = posiTion;


		// if not found, assume 7Bit encoding as per RFC2045 section 6.1
		if (posiTion<0 || posiTion > workitem->lOffset + workitem->lContentLength) 
		{
			workitem->nEncodeType = CUT_MIME_7BIT;
		} 
		else
		{  
			mime.m_ptrDecodeDataSource->Seek(alternativeStart, SEEK_SET);
			mime.m_ptrDecodeDataSource->Read(buf, 100);

			token = strtok(buf, " /\r\n");      // this should be the content line
			token = strtok(NULL, " /\r\n");     // this should be the encodeType

			// lets put an Id on the encoding type
			if(token != NULL) 
			{

				_strupr(token);

				int bMatchedYet = FALSE;

				if (!bMatchedYet && strcmp(token, "8BIT") == 0) 
				{
					workitem->nEncodeType = CUT_MIME_8BIT;
					bMatchedYet = TRUE;
				}

				else if (!bMatchedYet && strcmp(token, "BINARY") == 0) 
				{
					workitem->nEncodeType = CUT_MIME_BINARY;
					bMatchedYet = TRUE;
				}

				else if (!bMatchedYet && strcmp(token, "BASE64") == 0) 
				{
					workitem->nEncodeType = CUT_MIME_BASE64;
					bMatchedYet = TRUE;
				}

				else if (!bMatchedYet && strcmp(token, "QUOTED-PRINTABLE") == 0) 
				{
					workitem->nEncodeType = CUT_MIME_QUOTEDPRINTABLE;
					bMatchedYet = TRUE;
				}

				else if (!bMatchedYet && strcmp(token, "BINHEX40") == 0) 
				{
					workitem->nEncodeType = CUT_MIME_BINHEX40;
					bMatchedYet = TRUE;
				}

				else if (!bMatchedYet && strcmp(token, "MULTIPART") == 0) 
				{
					// this is included to handle nested multipart attachments
					workitem->nEncodeType = CUT_MIME_MULTIPART;
					bMatchedYet = TRUE;
				}
				// default operation
				else // (!bMatchedYet)
					workitem->nEncodeType = CUT_MIME_7BIT;
			}            
		}			
		alternativeStart = mime.m_ptrDecodeDataSource->Find("\r\n\r\n", alternativeStart, FALSE, workitem->lContentLength);
		// if a blank line is present in the attachment, it will mark the separation
		// between the attachment header, and the attachment body.  If a blank line
		// is not present (which is technically not possible under the RFC, then
		// we will assume that the body starts immediately after the boundary marker.
		if (alternativeStart > 0)
		{
			// The content length is the difrence between the position of the empty line and the first accurence of the next 
			// boundry string 
			workitem->lContentLength=  mime.m_ptrDecodeDataSource->Find(m_szAltrnvBoundry, alternativeStart, FALSE, workitem->lContentLength);// - (workitem->lOffset + reductionInContentLength);
			workitem->lOffset = alternativeStart + 4;

			// lets not forget the lines we have read
			workitem->lContentLength = workitem->lContentLength - (workitem->lOffset  );
			// lets increment it to the next one 
			alternativeStart = workitem->lOffset + workitem->lContentLength;
		} 
		else 
		{
			// ******* THIS BRANCH SHOULD NEVER BE EXCUTED ***************************
			// this branch is not possible under the terms of the RFC, but is included for robustness.
			workitem->lContentLength = workitem->lContentLength - boundaryStringLen;
			workitem->lOffset = workitem->lOffset + boundaryStringLen;
		}
		// For decoding the other alternatives we need to check if there is an other occurance of 
		// the boundry string ofthis portion of this type then we will do as above 
		// but first we need to ad a new item to the attachment list
		// if we are finding an other one then lets decode it too .gw
	}


	// now that we have found the multipart alternative in the indexed attachment 
	// lets remove it from the Attachment list
	// Try to find attachment with no name and Content-Type: "text/plain"

	mime.GetDecodeListItem(alterIndex, &item);

	MIMEATTACHMENTLISTITEM	*LastItem;

	// get the item prior to it
	// i we failed then this is the first one so lets make the one after it the main one
	if (mime.GetDecodeListItem(alterIndex-1, &LastItem) == -1)
		mime.m_ptrAttachList = item->next ; 
	else
		LastItem->next = item->next ;


	// Remove this attachment from the list
	if(item->ptrDataSource)
		delete item->ptrDataSource;
	if (item->lpszName  != NULL)
		delete[] item->lpszName;
	if (item->lpszContentType  != NULL)
		delete[] item->lpszContentType;
	if (item->lpszCharSet  != NULL)
		delete[] item->lpszCharSet;
	if (item->lpszAttachType  != NULL)
		delete[] item->lpszAttachType;
	if(item == mime.m_ptrAttachList)
		mime.m_ptrAttachList = item->next;
	else if(prev_item != NULL) 
		prev_item = item->next;

	delete item;


	return 1;
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
int CUT_MimeEncode::EncodeAttachmentFileName(int iIndex, enumEncodingType EncodingType, LPCSTR lpszCharSet)
{
	// Loop though all the attachment items and find the one we want
	MIMEATTACHMENTLISTITEM* pItem = m_ptrAttachList;
	int iCount = 0;
	while (pItem != NULL)
	{
		if (iIndex == iCount)
		{
			// Encode this filename
			CUT_HeaderEncoding HE;
			LPCSTR lpszEncoded = HE.Encode(pItem->lpszName, lpszCharSet, EncodingType);
			delete [] pItem->lpszName;
			pItem->lpszName = new char[::strlen(lpszEncoded)];
			::strcpy(pItem->lpszName, lpszEncoded);
		}
		pItem = pItem->next;
		iCount++;
	}

	return UTE_SUCCESS;
}

/***********************************************
DecodeAttachmentFileName
Decodes the filename of the attachment
PARAM:
iIndex		- index of the attachment
lpszCharSet	- (out) character set of the decoded text
RETURN:
UTE_SUCCESS	- success
UTE_ERROR	- error
************************************************/
int CUT_MimeEncode::DecodeAttachmentFileName(int iIndex, LPSTR lpszCharSet)
{
	// Loop though all the attachment items and find the one we want
	MIMEATTACHMENTLISTITEM* pItem = m_ptrAttachList;
	int iCount = 0;
	while (pItem != NULL)
	{
		if (iIndex == iCount)
		{
			CUT_HeaderEncoding HE;
			if (HE.IsEncoded(pItem->lpszName))
			{
				// Decode this filename
				LPCSTR lpszDecoded = HE.Decode(pItem->lpszName, lpszCharSet);
				delete [] pItem->lpszName;
				pItem->lpszName = new char[::strlen(lpszDecoded)];
				::strcpy(pItem->lpszName, lpszDecoded);
			}
		}
		pItem = pItem->next;
		iCount++;
	}

	return UTE_SUCCESS;
}

/***********************************************
AddHtmlBody
Adds an html body to the message as a nameless attachment
PARAM:
lpszHtmlBody - pointer to the html body
RETURN:
UTE_SUCCESS	- success
UTE_ERROR	- error
************************************************/
int CUT_MimeEncode::AddHtmlBody(LPCSTR lpszHtmlBody)
{
	RemoveHtmlBody(); // in order to avoid duplicates

	CUT_BufferDataSource bdsHtmlText((LPSTR) lpszHtmlBody, strlen(lpszHtmlBody));

	CUT_StringList listParams;
	listParams.AddString("2"); // CUT_MIME_QUOTEDPRINTABLE
	listParams.AddString("text/html");
	listParams.AddString("US-ASCII");
	AddAttachment(bdsHtmlText, NULL, &listParams, TRUE);

	m_ptrHtmlBodyAttachment = m_ptrCurrentAttach;

	return UTE_SUCCESS;
}

int CUT_MimeEncode::RemoveHtmlBody()
{
	// Try to find an attachment with no name and Content-Type: "text/html"
	MIMEATTACHMENTLISTITEM* item = m_ptrAttachList;
	MIMEATTACHMENTLISTITEM* prev_item = NULL;
	int	error = UTE_SUCCESS, index = 0;

	while (item != NULL)
	{
		if((item->lpszName == NULL || item->lpszName[0] == 0) && 
			((item->lpszContentType != NULL && _stricmp(item->lpszContentType, "text/html") == 0))) 
		{
			// Found the attachment, so remove it from the list
			if (item->ptrDataSource)
				delete item->ptrDataSource;
			if (item->lpszName  != NULL)
				delete[] item->lpszName;
			if (item->lpszContentType  != NULL)
				delete[] item->lpszContentType;
			if (item->lpszCharSet  != NULL)
				delete[] item->lpszCharSet;
			if (item->lpszAttachType  != NULL)
				delete[] item->lpszAttachType;

			if (item == m_ptrAttachList)
				m_ptrAttachList = item->next;
			else if(prev_item != NULL) 
				prev_item->next = item->next;

			delete item;

			break;
		}

		++ index;
		prev_item	= item;
		item		= item->next;
	}

	// v4.2 - note retval is not set if html body not found
	return error;
}

#pragma warning ( pop )