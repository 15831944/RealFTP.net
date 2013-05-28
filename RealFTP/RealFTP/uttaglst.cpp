//=================================================================
//  class: CUT_TagList
//  File:  uttaglst.cpp
//
//  Purpose:
//
//  Internal HyperText tags manipulation for use with Ultimate TCP/IP  
//  Hypertext Transfer Protocol client class
//
//  Revised: Feb 11 99:
//=================================================================
// Ultimate TCP/IP v4.2
// This software along with its related components, documentation and files ("The Libraries")
// is © 1994-2007 The Code Project (1612916 Ontario Limited) and use of The Libraries is
// governed by a software license agreement ("Agreement").  Copies of the Agreement are
// available at The Code Project (www.codeproject.com), as part of the package you downloaded
// to obtain this file, or directly from our office.  For a copy of the license governing
// this software, you may contact us at legalaffairs@codeproject.com, or by calling 416-849-8900.
//=================================================================/

#include "stdafx.h"
#include "uttaglst.h"


/****************************************
*****************************************/
CUT_TagList::CUT_TagList(){
    m_list = NULL;
}

/****************************************
*****************************************/
CUT_TagList::~CUT_TagList(){
    ClearList();
}

/****************************************
*****************************************/
int CUT_TagList::AddTagPos(long line,int pos){

    if(m_list == NULL){
        m_list = new UTTagList;
        m_list->line = line;
        m_list->pos = pos;
        m_list->next = NULL;
        return TRUE;
    }

    UTTagList* next = m_list;
    while(next->next != NULL){
        next = next->next;
    }
    next->next = new UTTagList;
    next = next->next;
    
    next->line = line;
    next->pos = pos;
    next->next = NULL;

    return TRUE;
}
/****************************************
*****************************************/
int CUT_TagList::ClearList(){
    
    if(m_list == NULL)
        return TRUE;

    UTTagList* next;
    
    while(m_list != NULL){

        next = m_list->next;
        delete m_list;
        m_list = next;
    }

    return TRUE;
}

/****************************************
*****************************************/
long CUT_TagList::GetCount(){
    
    if(m_list == NULL)
        return 0;

    long count = 0;
    UTTagList* next = m_list;
    
    while(next != NULL){
        count ++;
        next = next->next;
    }

    return count;
}

/****************************************
*****************************************/
int CUT_TagList::GetTagPos(int index,long *line,int *pos){

    if(m_list == NULL)
        return FALSE;

    long count = 0;
    UTTagList* next = m_list;
    
    while(next != NULL){
        if(count == index){
            *line = next->line;
            *pos = next->pos;
            return TRUE;
        }
        count ++;
        next = next->next;
    }

    return FALSE;
}
