//=================================================================
//  class: CUT_NNTPClient
//  class: CUT_NNTPGroupList
//  class: CUT_NNTPArticleList  
//  File:  nntpc.h
//
//  Purpose:
// 
//  Declaration of NNTP client class and its helper linked 
//  list classes CUT_NNTPGroupList & CUT_NNTPArticleList.
//
//  RFC 977
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

#ifndef NNTP_CLIENT_H
#define NNTP_CLIENT_H

#define LINE_BUFFER_SIZE            256

#define CUT_SORT_NAME_ASCENDING     1000
#define CUT_SORT_NAME_DESCENDING    1001

// base TCP/IP class 
#include "ut_clnt.h" 
#include "UTDataSource.h" 
#include "UTErr.h" 
#include "UTMessage.h"
#include "ut_strop.h"

    // Posting attributes enumeration
    enum PostingAttrib {
        CUT_POST_YES,
        CUT_POST_NO,
        CUT_POST_MODERATED
        };

//===================================
//  class: CUT_NNTPGroupList
//===================================
class CUT_NNTPGroupList {

    typedef struct tagNewsGroupList {
        LPSTR       lpszNewsGroupName;  // Title of the news group
        int         nStartId;           // ID of the start Article
        int         nEndId;             // ID of the end article
        int         nArticlesNumber;    // number of articles available
        int         nPostingAllowed;    // Posting attribute YES NO or MODERATED
        
        tagNewsGroupList *next;         // next group in the linked list
    } newsGroupList;
 
protected:      
    newsGroupList   *m_groupList;       // the work structure for the linked list of News groups

public:
    
    CUT_NNTPGroupList();                // constructor
    virtual ~CUT_NNTPGroupList();       // destructor

    // Read strings from the data soutce or file and load them to the group list.
    virtual int     Load(CUT_DataSource &source);
    int     Load(LPCTSTR filename) { CUT_FileDataSource ds(filename); return Load(ds); }

    // Loop through the list of available news groups available 
    // and save the summary information for each group to the data source 
    // or file 
    virtual int     Save(CUT_DataSource &dest);
    int     Save(LPCTSTR filename) { CUT_FileDataSource ds(filename); return Save(ds); }
    
    // Add a new NewsGroupList node to the list
    virtual int     AddGroup(LPCSTR newsGroup);

    // Remove a news group from the list
    virtual int     DeleteGroup(long index);

    //  Find the first news group that has a substring which matches the string passed
    virtual int     FindFirst(LPSTR match);

    //  Find a group the next avilable news group that has a substring that 
    //  matches a specific string
    virtual int     FindNext(LPSTR match, long index);

    //  Get the number of available news group in the linked list
    long    GetCount() const;

    //   Get the title of the news group specified by the index
    LPCSTR  GetGroupTitle(long index) const;

    //  Update the Group information   
    long    SetGroupInfo(LPCSTR newsGroup, int number, int first, int last);

    //  Remove all entries in the Group list linked list and reclaim
    //  the memory allocated
    int     Empty();


};  // end of class CUT_NNTPGroupList
//====================================================================


//====================================================================
// UT_ARTICLEINFOA STRUCTURE - used internally (ascii)
//====================================================================
	// v4.2 change - added constructor - probably not necessary
    typedef struct UT_ArticleInfoATag {  
		UT_ArticleInfoATag() : lpszSubject(NULL), lpszAuthor(NULL), 
							  lpszDate(NULL), lpszMessageId(NULL), 
							  lpszReferences(NULL){}
        long    nArticleId;         // Article ID
        LPSTR   lpszSubject;        // Article lpszSubject
        LPSTR   lpszAuthor;         // the from field in the Article message

        LPSTR   lpszDate;           // date of posting
        LPSTR   lpszMessageId;      // the RFC822 Message ID of the Article
        LPSTR   lpszReferences;     // Any refrence for reply 
        int     nByteCount;         // Number of bytes
        int     nLineCount;         // Number of lines
        int     nStatus;            // read, new, etc.
        
        UT_ArticleInfoATag *next;    // pointer to the next item
    } UT_ARTICLEINFOA;

	//====================================================================
// UT_ARTICLEINFO STRUCTURE - Holds _TCHAR - for user interface
//====================================================================
	// v4.2 change - added constructor - probably not necessary
    typedef struct UT_ArticleInfoTag {  
		UT_ArticleInfoTag() : lpszSubject(NULL), lpszSubjectCharset(NULL), lpszAuthor(NULL), 
							  lpszAuthorCharset(NULL), lpszDate(NULL), lpszMessageId(NULL), 
							  lpszReferences(NULL){}
        long    nArticleId;         // Article ID
        LPTSTR   lpszSubject;        // Article lpszSubject
		LPTSTR	 lpszSubjectCharset;  // charset if decoded
        LPTSTR   lpszAuthor;         // the from field in the Article message
		LPTSTR	 lpszAuthorCharset;   // charset if decoded
        LPTSTR   lpszDate;           // date of posting
        LPTSTR   lpszMessageId;      // the RFC822 Message ID of the Article
        LPTSTR   lpszReferences;     // Any refrence for reply 
        int     nByteCount;         // Number of bytes
        int     nLineCount;         // Number of lines
        int     nStatus;            // read, new, etc.
        
        UT_ArticleInfoTag *next;    // pointer to the next item
    } UT_ARTICLEINFO;

//====================================================================
// class: CUT_NNTPArticleList   
//====================================================================

class CUT_NNTPArticleList {

protected: 
    UT_ARTICLEINFOA    *m_listArticles;    // pointer to artticle s linked list

	CUT_StringList      m_OverviewFormat;  // The format of the over view the articles database on the server

public:
    
    CUT_NNTPArticleList();          // constuctor
    virtual ~CUT_NNTPArticleList(); // destructor
	void	SetOverViewFormat(CUT_StringList &overViewList);

    // Read articles from the data soutce or file 
    virtual int     Load(CUT_DataSource &source);
    int     Load(LPCTSTR filename) { CUT_FileDataSource ds(filename); return Load(ds); }

    // Save articles to the data soutce or file 
    virtual int     Save(CUT_DataSource &dest);
    int     Save(LPCTSTR filename) { CUT_FileDataSource ds(filename); return Save(ds); }

    // extraact new articles
    virtual int     ExtractNewArticles(CUT_StringList &m_listNewArticles);

    // add new article to the article list
    virtual int     AddArticle(UT_ARTICLEINFOA & articleInfo);

    // add an XOVER formated string as a header
    virtual int     AddArticleXOVER(LPSTR newsArticle);

    // remove an article from the list
    virtual int     DeleteArticle(long index);

    // Remove all articles from the linked list
    int     Empty();

    // find the first match of the string in the article lpszSubject
    virtual int     FindFirst(LPSTR match);

    // find the next match of the string in the article lpszSubject
    virtual int     FindNext(LPSTR match, long index);

    // get the number of articles in the list
    long    GetCount() const;

    // populate an article structure information with the indexed article
    UT_ARTICLEINFOA  *GetArticleInfo(long index) const;

};



//=================================================================
//  class: CUT_NNTPClient
//=================================================================
class CUT_NNTPClient:public CUT_WSClient{

public:
    CUT_NNTPClient();                       // constructor
    virtual ~CUT_NNTPClient();              // destructor

protected:

    CUT_StringList      m_listNewArticles;  // list of new articles
	CUT_StringList      m_OverviewFormat;  // The format of the over view the articles database on the server
    CUT_NNTPGroupList   m_listGroups;       // news Groups linked list class
    CUT_NNTPArticleList m_listArticles;     // news articles linked list class

	UT_ARTICLEINFO		m_artW;				// wide char articleinfo for NGetArticleInfo interface


    int         m_nConnectPort;             // port number to connect to
    int         m_nConnectTimeout;          // wait for connect time out 
    int         m_nReceiveTimeout;          // wait for receive time out 
    int         m_nPostingAllowed;          // is posting allowed in the server
    BOOL        m_bConnected;               // are we connected 
    char        *m_szSelectedNewsGroup;     // the currently selected group name
    int         m_nSelectedEstNum;          // number of articles available on the server upon establishing connection 
    int         m_nSelectedFirst;           // id of the first article availabl on the srver 
    int         m_nSelectedLast;            // id of the last article availabl on the srver 



protected:

    //////////////////////////////////////////////////////////
    //  Helper functions
    //////////////////////////////////////////////////////////
	int		GetOverviewFormat();


    // Receives a line of data
    int     ReceiveLineUntil(LPSTR data,int maxDataLen,int timeOut);

    // Receives data from the current connection directly to the data source
    int     ReceiveToDSUntil(   CUT_DataSource &dest, 
                                int fileType,
                                long *bytesReceived,
                                int timeOut, 
                                LPCSTR untilString);
    
    // Provides us with a an RFC 822 year 2000 complient date stamp
    int     NGetNNTPDateStamp(LPSTR buf, int bufLen);

    // Empty the new article list
    void    EmptyNewArticlesList();

public:
  
    //////////////////////////////////////////////////////////
    //  Generic server communication functions
    //////////////////////////////////////////////////////////

    // Connect to the NNTP server
    virtual int     NConnect(LPCSTR HostName, LPSTR User = NULL, LPSTR Pass = NULL);
#if defined _UNICODE
    virtual int     NConnect(LPCWSTR HostName, LPWSTR User = NULL, LPWSTR Pass = NULL);
#endif

    // Close the conection with the NNTP server
    virtual int     NClose();

    // Attempt to authorize this client with the server
    virtual int     NAuthenticateUserSimple(LPSTR User, LPSTR Pass);
#if defined _UNICODE
    virtual int     NAuthenticateUserSimple(LPWSTR User, LPWSTR Pass);
#endif

    // Returns the status of the m_canPost memory variable
    virtual int     NCanPost();

    // Contact the server and verify our ability to post.  May
    // be neccessary if posting is restricted in some news groups.
    virtual int     NForceCheckCanPost();

    // Retreives date from nntp server
    virtual int     NGetServerDate(LPSTR date, int nChars);
#if defined _UNICODE
    virtual int     NGetServerDate(LPWSTR date, int nChars);
#endif

    //////////////////////////////////////////////////////////
    //  News group relative functions
    //////////////////////////////////////////////////////////

    // All groups or new groups only.
    virtual int     NGetNewsGroupList(int type);

    // Select a news group
    virtual int     NSelectNewsGroup(LPCSTR newsGroup);
#if defined _UNICODE
    virtual int     NSelectNewsGroup(LPCWSTR newsGroup);
#endif 

    // Save the group list to a data source or file
    virtual int     NSaveNewsGroupList(CUT_DataSource &dest);
    int     NSaveNewsGroupList(LPCTSTR file) { CUT_FileDataSource ds(file); return NSaveNewsGroupList(ds); }

    // Read group list from a data source or file
    virtual int     NLoadNewsGroupList(CUT_DataSource &dest);
    int     NLoadNewsGroupList(LPCTSTR file) { CUT_FileDataSource ds(file); return NLoadNewsGroupList(ds); }

    // Get the number of groups available
    long    NGetNewsGroupCount() const;

    // Get the title of the indexed group
    LPCSTR  NGetNewsGroupTitle(int index) const;

	// v4.2 Refactored - added interface with LPSTR and LPWSTR params
	int NGetNewsGroupTitle(LPSTR title, size_t maxSize, int index, size_t *size);
#if defined _UNICODE
	int NGetNewsGroupTitle(LPWSTR title, size_t maxSize, int index, size_t *size);
#endif

	//////////////////////////////////////////////////////////
    //  Article relative functions
    //////////////////////////////////////////////////////////

    // Get the number of available articles in the selected group
    long    NGetNewArticlesCount() const;  

    // Save the articles list to a data source or file 
    virtual int     NSaveArticleHeaderList(CUT_DataSource &dest);
    int     NSaveArticleHeaderList(LPCTSTR file) { CUT_FileDataSource ds(file); return NSaveArticleHeaderList(ds); }

    // Load the articles list from a data source or file
    virtual int     NLoadArticleHeaderList(CUT_DataSource &source);
    int     NLoadArticleHeaderList(LPCTSTR file) { CUT_FileDataSource ds(file); return NLoadArticleHeaderList(ds); }
	    
    // Add to existing list, replace existing list, merge, etc.
    virtual int     NGetArticleList(int type);

   
    // Get an article by message id into the data source or file
    virtual int     NGetArticle(LPCSTR nArticleId, CUT_DataSource &dest);
#if defined _UNICODE
    virtual int     NGetArticle(LPCWSTR nArticleId, CUT_DataSource &dest);
#endif
    int     NGetArticle(LPCSTR nArticleId, LPCTSTR filename) { CUT_FileDataSource ds(filename); return NGetArticle(nArticleId, ds); }
#if defined _UNICODE
    int     NGetArticle(LPCWSTR nArticleId, LPCTSTR filename) { CUT_FileDataSource ds(filename); return NGetArticle(AC(nArticleId), ds); }
#endif

    // Get the number of articles available 
    long    NGetArticleHeaderCount() const;

    // Get the article headers information 
    UT_ARTICLEINFO *NGetArticleInfo(int index);

    // Post article as a whole message where the user have already taken care of the RFC 822 format
    virtual int     NPostArticle(CUT_Msg & message);

    // Format the article into an RFC 822 and post it
    virtual int     NPostArticle(LPCSTR PostGroup, LPCSTR PostFrom, LPCSTR PostlpszSubject, LPCSTR PostArticle);
#if defined _UNICODE
    virtual int     NPostArticle(LPCWSTR PostGroup, LPCWSTR PostFrom, LPCWSTR PostlpszSubject, LPCWSTR PostArticle);
#endif

    //////////////////////////////////////////////////////////
    //  Property Set / Get functions
    //////////////////////////////////////////////////////////

    // Set/Get connection port number
    int     SetConnectPort(int port);
    int     GetConnectPort() const;

    // Set/Get connect time out
    int     SetConnectTimeout(int secs);
    int     GetConnectTimeout() const;

    // Set/Get receive time out
    int     SetReceiveTimeout(int secs);
    int     GetReceiveTimeout() const;

	


    //////////////////////////////////////////////////////////
    //  Virtual notification functions
    //////////////////////////////////////////////////////////
protected:
    
    virtual int     OnNGetNewsGroupList(LPCSTR newGroupTitle);

    virtual int     OnNGetArticleList(LPCSTR artId, LPCSTR subject,LPCSTR author,LPCSTR date, LPCSTR refrence,LPCSTR BytesCount, LPCSTR NumberOfLines, LPCSTR xrefs );

    virtual BOOL    OnSendArticleProgress(long bytesSent,long totalBytes);
	long				GetResponseCode(LPCSTR data);
};

#endif
