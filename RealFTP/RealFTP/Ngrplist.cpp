// =================================================================
//  class: CUT_NNTPGroupList
//  File:  ngrplist.cpp
//
//  Purpose:
//
//  Declaration of linked list class CUT_NNTPGroupList
//  - helper class that is  used internally by CUT_NNTPClient class
//  SEE ALSO nntpc.cpp
//
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

#pragma warning (push, 3)
#include <fstream>
#pragma warning (pop)

#include "nntpc.h"

// Suppress warnings for non-safe str fns. Transitional, for VC6 support.
#pragma warning (push)
#pragma warning (disable : 4996)


/****************************************************************
****************************************************************/
CUT_NNTPGroupList::CUT_NNTPGroupList() : 
        m_groupList(NULL)       // Initialize group list
{
}

/****************************************************************
****************************************************************/
CUT_NNTPGroupList::~CUT_NNTPGroupList()
{
    Empty();                    // Clear group list
}

/****************************************************************
Load()
  Read strings from the files and load them to the group list.
    {"newsGroupName postingAllowed startId endId  postingAllowed"} 
    This function was added to allow for caching News Groups and their information to allow 
    connecting and refrencing news group names without having to query the server for them
    each time you connect
PARAM :
    source - the data source to read the list from
RETURN:
    UTE_SUCCESS         - success
    UTE_DS_OPEN_FAILED  - failed to open data source
    UTE_DS_CLOSE_FAILED - failed to close data source
****************************************************************/
int CUT_NNTPGroupList::Load(CUT_DataSource &source) 
{
    char    buf[MAX_PATH + 1];

    // Empty the group list first
    Empty();

    // Open the data source
    if(source.Open(UTM_OM_READING) == -1)
        return UTE_DS_OPEN_FAILED;

    // Read in and save the group list informations
    while(source.ReadLine(buf, MAX_PATH) != 0 ) 
        AddGroup(buf);
 
    // Close data source
    if(source.Close() == -1)
        return UTE_DS_CLOSE_FAILED;
    
    return UTE_SUCCESS;
}

/****************************************************************
Save()
  Loop through the list of available news groups available 
  and save the summary information for each group to a disk file
  in the following format
    {"newsGroupName postingAllowed startId endId  postingAllowed"} 
PARAM :
    dest - the data source to save the list to
RETURN:
    UTE_SUCCESS         - success
    UTE_ERROR           - the Group list is empty
    UTE_DS_OPEN_FAILED  - failed to open data source
    UTE_DS_CLOSE_FAILED - failed to close data source
****************************************************************/
int CUT_NNTPGroupList::Save(CUT_DataSource &dest)
{
    newsGroupList   *work = m_groupList;
    int             nStrLen, error = UTE_SUCCESS;
    char            buf[MAX_PATH + 1];

    // If the group list is empty
    if (work == NULL)
        return UTE_ERROR;  

    // Open the data source
    if(dest.Open(UTM_OM_WRITING) == -1)
        return UTE_DS_OPEN_FAILED;

    // Read in and save the group list informations
    while (work != NULL && error == UTE_SUCCESS) {

        // Prepare formatted string
        nStrLen = _snprintf(buf,sizeof(buf)-1, "%s %d %d %d\r\n",   work->lpszNewsGroupName,
                                                    work->nStartId,
                                                    work->nEndId, 
                                                    work->nPostingAllowed);

		// if the length of string to copy exceeds buf length, 
		// then _snprintf will return negative value
		if(0 > nStrLen)
		{
			nStrLen = (int)strlen(buf);
		}

        // Write to the data source
        if(dest.Write(buf, nStrLen) != nStrLen)
            error = UTE_DS_WRITE_FAILED;

        // Go to the next group item
        work = work->next;
        }

    // Close data source
    if(dest.Close() == -1)
        error = UTE_DS_CLOSE_FAILED;

    return error;
}

/****************************************************************
AddGroup()  (internal method)

  Add a new NewsGroupList node to the list.
  Parameter "newsGroup" is a string that contains the 
  summary information for the newsgroup.
  the summary is in the following format
    {"newsGroupName postingAllowed startId endId  postingAllowed"} 
PARAM   
    LPSTR newsGroup  - a string containing the News Group summary to be parsed
RETURN
    TRUE
****************************************************************/
int CUT_NNTPGroupList::AddGroup(LPCSTR newsGroup)
{
  	char			szNewsGroupName[MAX_PATH]; 

    newsGroupList   *item;

    // test to see if this is the first entry in the list
    if (m_groupList == NULL) {
        m_groupList = new newsGroupList;
        item = m_groupList;
        } 
    else {
        // iterate through until we find the end of the list.
        // not terribly efficient, but lists should be small < 20K
        item = m_groupList;

        while (item->next != NULL)
            item=item->next;
        
        item->next = new newsGroupList;
        item = item->next;
    }
	CUT_StrMethods::ParseString (newsGroup, " \t" ,0, szNewsGroupName,sizeof(szNewsGroupName)-1);
	szNewsGroupName[MAX_PATH-1] = 0;


    item->lpszNewsGroupName     = new char[strlen(szNewsGroupName) + 1];
    item->lpszNewsGroupName[0]  = 0;
    item->next = NULL;
    strcpy(item->lpszNewsGroupName, szNewsGroupName);

    // add the ID of the last article in the group
	long lTemp = 0;
	CUT_StrMethods::ParseString (newsGroup, " \t" ,1,&lTemp);
	item->nStartId =lTemp;

	CUT_StrMethods::ParseString (newsGroup, " \t" ,2,&lTemp);
	item->nEndId =lTemp;

 
    // calculate the ESTIMATED nubmer of articles in the 
    // group by finding the difference between the startId
    // and the endId.  It is important to note that article
    // postings may not be contiguous.
    item->nArticlesNumber = item->nEndId - item->nStartId;

    // store whether posting is allowed to this news group.
    CUT_StrMethods::ParseString (newsGroup, " \t" ,3,&lTemp);
	
	switch(lTemp)
	{
            case 'y':
                item->nPostingAllowed = CUT_POST_YES;
                break;
            case 'n':
                item->nPostingAllowed = CUT_POST_NO;
                break;
            case 'm':
                item->nPostingAllowed = CUT_POST_MODERATED;
                break;
	}

    return TRUE;
}

/****************************************************************
DeleteGroup
    Remove a news group from the list
    This function can be used when you are saving the news group list to a file
    SEE ALSO Save() & Load functions
PARAM:
    long index - the index of the news group to be deleted from the list
RETURN
    UTE_SUCCESS             - success
    UTE_INDEX_OUTOFRANGE    - index is out of range
****************************************************************/
int CUT_NNTPGroupList::DeleteGroup(long index)
{
    // If the group list is empty
    if (m_groupList == NULL)
        return UTE_INDEX_OUTOFRANGE;

    long            count   = 0;
    newsGroupList   *item   = m_groupList;
    newsGroupList   *prev	= m_groupList;

    if (index == 0) {
        m_groupList = item->next;
        delete[] item->lpszNewsGroupName;
        delete item;
		return UTE_SUCCESS; 
    }

    while (item != NULL) {          // test for accidental end-of-list
        if (count == index) {        
            prev->next = item->next;
            delete[] item->lpszNewsGroupName;
            delete item;
            return UTE_SUCCESS;
            }

        count++;
        prev = item;
        item = item->next;
    }

    return UTE_INDEX_OUTOFRANGE;
}

/****************************************************************
FindFirst()
    Find the first news group that has a substring which matches the string passed
PARAM
    LPSTR match - The sub string to look for
RETURN
    -1 NOT found
    the index of the first news group in the list that matches the passed substring
****************************************************************/
int CUT_NNTPGroupList::FindFirst(LPSTR match)
{
    long            count = 0;
    newsGroupList   *work = m_groupList;

    // If the group list is empty
    if(work == NULL)
        return -1;

    while(strstr(work->lpszNewsGroupName, match) == NULL) {
        count++;
        work = work->next;
        if (work == NULL)
            return -1;
    }

    return count;
}

/****************************************************************
FindNext()
      Find a group the next avilable news group that has a sub string that 
      matches a specific string
PARAM:
    LPSTR match
    long index
RETURN
    -1      the list is empty or the index is aout of range or substring was not found
    count   The index of the news group that matched the specified string
****************************************************************/
int CUT_NNTPGroupList::FindNext(LPSTR match, long index)
{
    long            count = 0;
    newsGroupList   *work = m_groupList;

    // If the group list is empty
    if(work == NULL)
        return -1;

    while(count != index) {
        if (work == NULL)
            return -1;
        count ++;
        work = work->next;
        }

    while(strstr(work->lpszNewsGroupName,match) == NULL) {
        count++;
        work= work->next;
        if (work == NULL)
            return -1;
    }

    return count;
}

/****************************************************************
GetCount()
    Get the number of available news group in the linked list
PARAM:
    NONE
RETURN
    long    number of groups available
****************************************************************/
long CUT_NNTPGroupList::GetCount() const
{
    // If the group list is empty
    if (m_groupList == NULL)
        return 0L;

    long            count = 0;
    newsGroupList   *next = m_groupList;

    while (next != NULL) {
        count ++;
        next= next->next;
    }

    return count;
}

/****************************************************************
GetGroupTitle()
  Get the title of the news group specified bby the index
PARAM
    long index      - index of the news group 
    LPSTR newsGroup - a buffer to hold the news group title
RETURN
    NULL    - error
    news group title
****************************************************************/
LPCSTR CUT_NNTPGroupList::GetGroupTitle(long index) const
{
    // If the group list is empty
    if (m_groupList == NULL)
        return NULL;

    long            count = 0;
    newsGroupList   *next = m_groupList;

    while (next != NULL) {
        if (count == index) 
            return next->lpszNewsGroupName;

        count ++;
        next = next->next;
    }

    return NULL;  // index out of range
}

/****************************************************************
Empty()
    Remove all entries in the Group list linked list and reclaim
    the memory allocated
PARAM
    NONE
RETURN
    TRUE
****************************************************************/
int CUT_NNTPGroupList::Empty()
{
    // If the group list is empty
    if (m_groupList == NULL)
        return TRUE;

    newsGroupList   *next;

    while (m_groupList != NULL) {
        next = m_groupList->next;
        delete[] m_groupList->lpszNewsGroupName;
        delete m_groupList;
        m_groupList = next;
    }

    return TRUE;
}

/****************************************************************
SetGroupInfo()
    Update the Group information   
PARAM
    LPCSTR newsGroup - the name of the news group to update the information for
    int number       - number of articles
    int first        - ID of the firts article available
    int last         - ID of the last article available
RETURN
    -1      - the news group was not allocated 
    count   - the index of the news group that we have just updated
****************************************************************/
long CUT_NNTPGroupList::SetGroupInfo(LPCSTR newsGroup, int number, int first, int last)
{
    long            count = 0;
    newsGroupList   *work = m_groupList;

    // If the group list is empty
    if(work == NULL)
        return -1;

    while(strcmp(work->lpszNewsGroupName, newsGroup) != 0) {
        count++;
        work= work->next;
        if (work == NULL)
            return -1;
    }
    
    work->nStartId          = first;
    work->nEndId            = last;
    work->nArticlesNumber   = number;
    
    return count;
}

#pragma warning ( pop )