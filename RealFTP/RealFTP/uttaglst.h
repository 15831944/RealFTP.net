// =================================================================
//  class: CUT_TagList
//  File:  uttaglst.h
//
//  Purpose:
//
//  Internal HyperText tags manipulation for use with Ultimate TCP/IP  
//  Hypertext Transfer Protocol client class
//
//  Revised Feb 11 99
// =================================================================
// Ultimate TCP/IP v4.2
// This software along with its related components, documentation and files ("The Libraries")
// is © 1994-2007 The Code Project (1612916 Ontario Limited) and use of The Libraries is
// governed by a software license agreement ("Agreement").  Copies of the Agreement are
// available at The Code Project (www.codeproject.com), as part of the package you downloaded
// to obtain this file, or directly from our office.  For a copy of the license governing
// this software, you may contact us at legalaffairs@codeproject.com, or by calling 416-849-8900.
// =================================================================
#ifndef UTTAGLST_INCLUDE_H
#define UTTAGLST_INCLUDE_H

class CUT_TagList{

private:
    
    typedef struct UTTagListTag{
        long line;
        int  pos;
        UTTagListTag * next;
    }UTTagList;

    UTTagList *m_list;

public:
    CUT_TagList();
    virtual ~CUT_TagList();

    int AddTagPos(long line,int pos);
    int ClearList();
    long GetCount();

    int GetTagPos(int index,long *line,int *pos);
};
#endif
