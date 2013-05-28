// =================================================================
//  class: CUT_NNTPArticleList
//  File:  nartlist.cpp
//
//  Purpose:
//
//  Declaration of linked list class CUT_NNTPArticleList
//  This class is a helper class that is used internally 
//  by the CUT_NNTPClient class
//
//  SEE ALSO nntpc.cpp
//  REMARK:
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

#ifdef _WINSOCK_2_0_
    #define _WINSOCKAPI_    /* Prevent inclusion of winsock.h in windows.h   */
                            /* Remove this line if you are using WINSOCK 1.1 */
#endif


//MFC Users - include the following
#include "stdafx.h"
//OWL Users - include the following
//#include <owl\owlpch.h>
//SDK Users - include the following
//#include <windows.h>

#include <stdio.h>
#include "nntpc.h"

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)

/****************************************************************
****************************************************************/
CUT_NNTPArticleList::CUT_NNTPArticleList() :
        m_listArticles(NULL)        // Initialize article list
{
}

/****************************************************************
****************************************************************/
CUT_NNTPArticleList::~CUT_NNTPArticleList()
{
    Empty();                        // Clear article list
}

/****************************************************************
Load()
    Load the current list of articles headers from a disk file
    The articles headers will be retrived in the following format -
    this format is the same format as XOVER command would be answered
    "nArticleId[TAB]lpszSubject[TAB]Author[TAB]Date[TAB]Refrences[TAB]BytesCount[TAB]NumberOfLines[CRLF]"
    We will then call the AddXOVERArticle() to parse the result
PARAM
    source  - data source to load the articles from
RETURN
    UTE_SUCCESS         - success
    UTE_DS_OPEN_FAILED  - failed to open data source
    UTE_DS_CLOSE_FAILED - failed to close data source
*****************************************************************/
int CUT_NNTPArticleList::Load(CUT_DataSource & source)
{
    char    buf[500];

    // Empty the article list first
    Empty();

    // Open the data source
    if(source.Open(UTM_OM_READING) == -1)
        return UTE_DS_OPEN_FAILED;

    // Read in and save the article list informations
    while(source.ReadLine(buf, MAX_PATH) != 0 ) 
        AddArticleXOVER(buf);

    // Close data source
    if(source.Close() == -1)
        return UTE_DS_CLOSE_FAILED;

    return UTE_SUCCESS;
}

/****************************************************************
Save()
    Save the current list of articles headers to a disk file
    The articles headers will be stored in the following format, this format is
    the same format an XOVER command would be answered
    "nArticleId[TAB]lpszSubject[TAB]Author[TAB]Date[TAB]Refrences[TAB]BytesCount[TAB]NumberOfLines[CRLF]"
    We will save the articles list in the same format. Note that the optional ThreadTopic is not saved.
PARAM
    dest - the data source to save the list to
RETURN
    UTE_SUCCESS         - success
    UTE_ERROR           - the Group list is empty
    UTE_DS_OPEN_FAILED  - failed to open data source
    UTE_DS_CLOSE_FAILED - failed to close data source
****************************************************************/
int CUT_NNTPArticleList::Save(CUT_DataSource & dest)
{
    UT_ARTICLEINFOA          *work = m_listArticles;
    int             nStrLen, error = UTE_SUCCESS;
    char            buf[500];

    // If the article list is empty
    if (work == NULL)
        return UTE_ERROR;  

    // Open the data source
    if(dest.Open(UTM_OM_WRITING) == -1)
        return UTE_DS_OPEN_FAILED;
    
    // Read in and save the article list informations
    while (work != NULL && error == UTE_SUCCESS) {

        // Prepare formatted string
        nStrLen = _snprintf(buf,sizeof(buf)-1, "%d\t%s\t%s\t%s\t%s\t%d\t%d\r\n",    
                                work->nArticleId,
                                work->lpszSubject, 
                                work->lpszAuthor, 
                                work->lpszDate,
                                work->lpszReferences,
                                work->nByteCount,
                                work->nLineCount);

		// if the length of string to copy exceeds buf length, 
		// then _snprintf will return negative value
		if(0 > nStrLen)
		{
			nStrLen = (int)strlen(buf);
		}

        // Write to the data source
        if(dest.Write(buf, nStrLen) != nStrLen)
            error = UTE_DS_WRITE_FAILED;

        // Go to the next article item
        work = work->next;
        }

    // Close data source
    if(dest.Close() == -1)
        error = UTE_DS_CLOSE_FAILED;

    return error;
}

/****************************************************************
AddArticle()
  Insert a new article in the linked list of Articles
PARAM:
    UT_ARTICLEINFOA *articleInfo - apointer to the Article info structure
                see the header file for structure memebers
RETURN:
    UTE_SUCCESS - success
****************************************************************/
int CUT_NNTPArticleList::AddArticle(UT_ARTICLEINFOA & articleInfo)
{
    UT_ARTICLEINFOA      *item; 

    // Find end of linked list or create new list 
    // if it doesn't already exist.
    if (m_listArticles == NULL) {           // create the list
        m_listArticles = new UT_ARTICLEINFOA;
        item = m_listArticles;
        } 
    else {                                  // find the end of the existing list
        item = m_listArticles;

        while (item->next != NULL)
            item = item->next;
        
        item->next = new UT_ARTICLEINFOA;
        item = item->next;
       }
    
    // Insert new element into linked list
    item->nArticleId        = articleInfo.nArticleId;
    item->lpszSubject       = articleInfo.lpszSubject;
    item->lpszAuthor        = articleInfo.lpszAuthor;
    item->lpszDate          = articleInfo.lpszDate;
    item->lpszMessageId     = articleInfo.lpszMessageId;
    item->lpszReferences    = articleInfo.lpszReferences;
    item->nByteCount        = articleInfo.nByteCount;
    item->nLineCount        = articleInfo.nLineCount;
    item->nStatus           = articleInfo.nStatus;

    item->next = NULL;

    return UTE_SUCCESS;
}
void	CUT_NNTPArticleList::SetOverViewFormat(CUT_StringList &overViewList)
{
	m_OverviewFormat.ClearList ();
	int count  = overViewList.GetCount ();
	for (int loop = 0; loop < count; loop++ )
		m_OverviewFormat.AddString( overViewList.GetString (loop));

	return;

}
/****************************************************************
AddArticleXOVER()
      This function accepts article header information
      formatted in the standard NNTP XOVER format.  
param
  LPSTR articleHeader -  An XOVER formated string that hold the raw data 
        to be parsed as the header information
RETURN
    UTE_SUCCESS                 - success    
    UTE_PARAMETER_INVALID_VALUE - invalid pointer parameter
****************************************************************/
int CUT_NNTPArticleList::AddArticleXOVER(LPSTR articleHeader)
{
    char             temp[1024];
    UT_ARTICLEINFOA  workArticle;

    if(articleHeader == NULL)
        return UTE_PARAMETER_INVALID_VALUE;

    int fieldCounter = 0; // Counter for the number of fields in the header tokened by a tab Or NULL

    // scan string for two tabs together and insert a space between
    // them.  This is necessary to ensure that the strtok function
    // will not get gummed up.  strtok will skip leading tokens in
    // a string, so if two tokens are side by side, the function,
    // will skip one of the parameters rather than returning a null
    // parameter.

    if(strstr(articleHeader, "\t\t") != NULL) {
        char buf[2500];
        int i = 0, j = 0;
        do {
            if (articleHeader[i] == '\t' && articleHeader[i+1] == '\t') {
                buf[j++]=articleHeader[i++];
                buf[j++]=' ';
                buf[j++]=articleHeader[i++];
                } 
            else 
                buf[j++]=articleHeader[i++];        
        } while (articleHeader[i] != '\0');
            
        buf[j] = '\0';
        strcpy(articleHeader, buf);
    }

	int nId = 0;  // by default
	int nSubject  = 0;
	int nXref = 0;
	int nFrom = 0;
	int nDate = 0;
	int nByte = 0;
	int nMsgId = 0;
	int nNumberOfLines = 0;
	int nRefrence = 0;
	int nTopic = 0;

	/* Updated Version 4.x 
	Each line of output will be formatted with the article number,
	followed by each of the headers in the overview database or the
	article itself (when the data is not available in the overview
	database) for that article separated by a tab character.  The
	sequence of fields must be in this order: subject, author, date,
	message-id, references, byte count, and line count.  Other optional
	fields may follow line count.  Other optional fields may follow line
	count.  These fields are specified by examining the response to the
	LIST OVERVIEW.FMT command.  Where no data exists, a null field must
	be provided (i.e. the output will have two tab characters adjacent to
	each other).  Servers should not output fields for articles that have
	been removed since the XOVER database was created.
								GW
	*/

	for (int loop = 0; loop < m_OverviewFormat.GetCount (); loop++ )
	{
		if (nSubject == 0 && stricmp(m_OverviewFormat.GetString (loop),"Subject") == 0)
		{
			nSubject = loop+1;

		}
		else if (nFrom == 0 && stricmp(m_OverviewFormat.GetString (loop),"From") == 0)
		{
			nFrom = loop+1;

		}
		else if (nDate == 0 && stricmp(m_OverviewFormat.GetString (loop),"Date") == 0)
		{
			nDate = loop+1;

		}
		else if (nXref == 0 && stricmp(m_OverviewFormat.GetString (loop),"Xref") == 0)
		{
			nXref = loop+1;

		}
		else if (nMsgId == 0 && stricmp(m_OverviewFormat.GetString (loop),"Message-Id") == 0)
		{
			nMsgId = loop+1;

		}
		else if (nByte == 0 && stricmp(m_OverviewFormat.GetString (loop),"Bytes") == 0)
		{
			nByte = loop+1;

		}
		else if (nNumberOfLines == 0 && stricmp(m_OverviewFormat.GetString (loop),"Lines") == 0)
		{
			nNumberOfLines = loop+1;

		}
		else if (nRefrence == 0 && stricmp(m_OverviewFormat.GetString (loop),"References") == 0)
		{
			nRefrence = loop+1;
		}			
		else if (nTopic == 0 && stricmp(m_OverviewFormat.GetString (loop),"Thread-Topic") == 0)
		{
			nTopic = loop+1;
		}			
	}
	// string operation is way too consuming 
    // Get the number of peices
    fieldCounter = CUT_StrMethods::GetParseStringPieces(articleHeader, "\t");

	CUT_StrMethods::ParseString (articleHeader,"\t",nId, &(workArticle.nArticleId));
	if (nSubject > 0)
	{
		CUT_StrMethods::ParseString (articleHeader,"\t",nSubject,temp,sizeof(temp)-1);
		workArticle.lpszSubject = new char[strlen(temp)+1];
        strcpy(workArticle.lpszSubject, temp);
	}     
	if (nFrom > 0)
	{
		CUT_StrMethods::ParseString (articleHeader,"\t",nFrom,temp,sizeof(temp)-1);
	    workArticle.lpszAuthor = new char[strlen(temp)+1];
		strcpy(workArticle.lpszAuthor, temp);   
	}
    // Date
		if (nDate > 0)
	{
		CUT_StrMethods::ParseString (articleHeader,"\t",nDate,temp,sizeof(temp)-1);
	    workArticle.lpszDate = new char[strlen(temp)+1];
		strcpy(workArticle.lpszDate, temp);   
	}

	if (nMsgId > 0)
	{
		CUT_StrMethods::ParseString (articleHeader,"\t",nMsgId,temp,sizeof(temp)-1);
	    workArticle.lpszMessageId = new char[strlen(temp)+1];
		strcpy(workArticle.lpszMessageId, temp);   
	}

	if (nRefrence > 0)
	{
		CUT_StrMethods::ParseString (articleHeader,"\t",nRefrence,temp,sizeof(temp)-1);
	    workArticle.lpszReferences = new char[strlen(temp)+1];
		strcpy(workArticle.lpszReferences, temp);   
	}
	// bytecount
	if (nByte > 0)
	{
		long numbByte = 0;
		CUT_StrMethods::ParseString (articleHeader,"\t",nByte, &numbByte );

		workArticle.nByteCount = (int )numbByte ;
	}	

	if (nNumberOfLines > 0)
	{
		long numbByte = 0;
		CUT_StrMethods::ParseString (articleHeader,"\t",nNumberOfLines, &numbByte );
		workArticle.nLineCount  = (int )numbByte ;
	}

    AddArticle(workArticle);
    
    return UTE_SUCCESS;
}

/****************************************************************
DeleteArticle()
    Remove an article from the linked list
PARAM
    long index - index of the article to be deleted 
RETURN
    UTE_SUCCESS             - success
    UTE_INDEX_OUTOFRANGE    - index is out of range
****************************************************************/
int CUT_NNTPArticleList::DeleteArticle(long index)
{
    // If the article list is empty
    if (m_listArticles == NULL)
        return UTE_INDEX_OUTOFRANGE;

    long        count   = 0;
    UT_ARTICLEINFOA *item    = m_listArticles;
    UT_ARTICLEINFOA *prev    = m_listArticles;

    if (index == 0) {                   // deleting the first element
        m_listArticles = item->next;
        } 
    else {
        while (count != index) {        // search for element to delete
            if (item->next == NULL)     // if we've hit the end of the list then exit.
                return 2;
            count++;
            prev = item;
            item = item->next;
            }
        prev->next = item->next;
        }
	// v4.2 added NULL checks along with ctor init - not critical
    if(NULL != item->lpszSubject) delete[] item->lpszSubject;
    if(NULL != item->lpszAuthor) delete[] item->lpszAuthor;
    if(NULL != item->lpszMessageId) delete[] item->lpszMessageId;
    if(NULL != item->lpszDate) delete[] item->lpszDate;
    if(NULL != item->lpszReferences) delete[] item->lpszReferences;
    delete item;
    return UTE_SUCCESS;
}

/****************************************************************
FindFirst()
    Find the first Article that has a substring lpszSubject which matches
    the string passed
PARAM
    LPSTR match - The sub string to look for
RETURN
    -1  - NOT found
    int - the index of the first news group in the list that
            matches the passed substring
****************************************************************/
int CUT_NNTPArticleList::FindFirst(LPSTR match) 
{
    long        count = 0;
    UT_ARTICLEINFOA *item = m_listArticles;

    // If the article list is empty
    if (item == NULL)
        return -1;

    while (strstr(item->lpszSubject, match) == NULL) {
        count++;
        item = item->next;
        if (item == NULL)
            return -1;
        }

    return count;
}

/****************************************************************
FindNext()
      Find the next avilable Article that has a sub string in the lpszSubject that 
    matches a specific string
PARAM:
    match   - string to find
    index   - index to start with
RETURN
    -1      - the list is empty or the index is aout of range or substring was not found
    count   - The index of the news group that matched the specified string
****************************************************************/
int CUT_NNTPArticleList::FindNext(LPSTR match, long index)
{
    long        count = 0;
    UT_ARTICLEINFOA *item = m_listArticles;

    while (count != index) {
        count ++;
        item = item->next;
        if (item == NULL)
            return -1;
        }

    while (strstr(item->lpszSubject,match)==NULL) {
        count++;
        item = item->next;
        if (item == NULL)
            return -1;
        }

    return count;
}

/****************************************************************
GetCount()
  Get the number of available articles in the list
param 
    NONE
return 
    long - number of articles available
****************************************************************/
long CUT_NNTPArticleList::GetCount() const
{
    long        count = 0;
    UT_ARTICLEINFOA *item = m_listArticles;

    while (item != NULL) {
        count ++;
        item = item->next;
        }

    return count;
}

/****************************************************************
GetArticleInfo
    traverse the linked list of articles and return an
    UT_ARTICLEINFOA structure containing the article summary data
PARAM
    index       - position in article list
    articleInfo - Pointer to an UT_ARTICLEINFOA structure
RETURN
    NULL    - error
    pointer to UT_ARTICLEINFOA structure
****************************************************************/
UT_ARTICLEINFOA *CUT_NNTPArticleList::GetArticleInfo(long index) const
{
    long        count = 0;
    UT_ARTICLEINFOA *item = m_listArticles;
    
    // Since the we have added The new article features
    // some of these articles may not be available 
    while (item != NULL) {
        if (count == index) 
            return item;

        count ++;
        item = item->next;
    }

    return NULL;   // reached the end of the list without finding a match
}

/****************************************************************
Empty()
  Remove all articles headers in the list and 
  reclaim all memory occupied
PARAM
    none
RETURN
    UTE_SUCCESS
****************************************************************/
int CUT_NNTPArticleList::Empty(){

    UT_ARTICLEINFOA *item = m_listArticles;
    UT_ARTICLEINFOA *work;

    while (item != NULL) {
        work = item->next;

        if(NULL != item->lpszSubject) delete[] item->lpszSubject;
        if(NULL != item->lpszAuthor) delete[] item->lpszAuthor;
        if(NULL != item->lpszMessageId) delete[] item->lpszMessageId;
        if(NULL != item->lpszDate) delete[] item->lpszDate;
        if(NULL != item->lpszReferences) delete[] item->lpszReferences;
        delete item;

        item = work;
        }

    m_listArticles = NULL;

    return UTE_SUCCESS;
}


/************************
/* (GW) March 19 1998
    Now test if we have all the articles of the specified group
    IF we do then 
        LOOP through the articles 
            IF the Message Id of each article matches to a one we just found  
                THEN
                    keep it
            ELSE 
                remove it form the linked list
        END LOOP
    ELSE
        Get All articles in the group
            THEN
                LOOP through the articles 
                    IF the Message Id of each article matches to a one we just found  
                        then
                            keep it
                    ELSE
                        remove it form the linked list
                END LOOP
    END IF 
************************/
int CUT_NNTPArticleList::ExtractNewArticles(CUT_StringList &m_newArticles)
{
    int             index;
    UT_ARTICLEINFOA      *prev = NULL;  // Our anchor 
    UT_ARTICLEINFOA      *temp;
    UT_ARTICLEINFOA      *dummy;

    temp            = m_listArticles;

    if (temp != NULL ) {
        while (temp != NULL) {

            if ( prev == NULL) {                            // It is the first one 
                for(index = 0; index < m_newArticles.GetCount(); index++ ) 
                    if ( strcmp(m_newArticles.GetString(index), temp->lpszMessageId) == 0 )
                        break;                              // This article is new 

                if (index >= m_newArticles.GetCount()) {    // we broke from the loop without finding the article
                    m_listArticles = temp->next;                // since we are at the start so far
                    delete temp;
                    temp = m_listArticles;                  // so far we are at the begining  so 
                                                            // we don't reset prev
                    } 

                else {                                      // we broke from the loop after finding the article
                    prev = m_listArticles;
                    temp = m_listArticles;
                    }

                }                                           // End of testing for the first

            else {                                          // Not the first one
                for(index = 0; index < m_newArticles.GetCount(); index++ ) 
                    if ( strcmp(m_newArticles.GetString(index), temp->lpszMessageId) == 0 )
                        break;                              // This article is new 

                if (index >= m_newArticles.GetCount())  {   // we broke from the loop without finding the article
                    dummy = temp->next ; 
                    delete temp;
                    temp = dummy; 
                    } 
                else
                    temp = temp->next;
                }
            }
        }

    if (temp != NULL)                                       // the loop has finished 
        return CUT_ERROR;

    return UTE_SUCCESS;
}   

#pragma warning ( pop )