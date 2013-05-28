#include "stdafx.h"
#include "ConnectionManager.h"


CConnectionManager::CConnectionManager(void)
{
	
}

CConnectionManager::~CConnectionManager(void)
{
}


void CConnectionManager::Load()
{
	CFile fleCars;
    CFileFind fndFile;
    CString strFilename = _T("Connections.data");

    // Look for the Cars.bcr file
    BOOL exists = fndFile.FindFile(strFilename);

    if( exists == TRUE )
    {
		// If the file exists, open it and fill the controls with it
		fleCars.Open(strFilename, CFile::modeRead);
		// Create a CArchive object to receive the collection
		CArchive arc(&fleCars, CArchive::load);
		// Pass the CArchive object to the Serialize() function of this application
		Serialize(arc);

		// Close the archive
		arc.Close();
		// Close the file stream
		fleCars.Close();

		// Behave as if the user had just clicked the First button
		//OnBnClickedFirst();
    }

	//TRACE1("size %d \n", m_listOfConnections.GetSize() );

	//int i = 0;
	//for(i = 0; i <  m_listOfConnections.GetSize(); i++){
	//	CConnection * con = (CConnection *)m_listOfConnections.GetAt(i);
		//TRACE1("  el: %s \n", con->name );
	//}
}

void CConnectionManager::Store()
{
	CFile fleCars;
	CString strFilename = _T("Connections.data");

	fleCars.Open(strFilename, CFile::modeCreate | CFile::modeWrite);
	CArchive arc(&fleCars, CArchive::store);
	Serialize(arc);

	arc.Close();
	fleCars.Close();
}

void CConnectionManager::Add(CConnection &con)
{
	m_listOfConnections.Add(&con);
}

void CConnectionManager::RemoveAt(int index)
{
	if(index >= 0 && index < m_listOfConnections.GetSize()){
		m_listOfConnections.RemoveAt(index);
	}
}

void CConnectionManager::Update(int index, CConnection &con)
{
	if(index >= 0 && index < m_listOfConnections.GetSize()){
		m_listOfConnections.SetAt(index, &con);
	}
}

CConnection* CConnectionManager::GetAt(int index)
{
	CConnection * con;
	if(index >= 0 && index < m_listOfConnections.GetSize()){
		con = (CConnection *)m_listOfConnections.GetAt(index);
	}
	return con;
}

void CConnectionManager::Swap(int index, int with_index)
{
	if(index >= 0 && index < m_listOfConnections.GetSize()){
		if(with_index >= 0 && with_index < m_listOfConnections.GetSize()){
			CConnection * con_a;
			CConnection * con_b;
			con_a = (CConnection *)m_listOfConnections.GetAt(index);
			con_b = (CConnection *)m_listOfConnections.GetAt(with_index);
			m_listOfConnections.SetAt(index, con_b);
			m_listOfConnections.SetAt(with_index, con_a);
		}
	}
}

CObArray * CConnectionManager::GetConnections()
{
	return &m_listOfConnections;
}


void CConnectionManager::Serialize(CArchive& ar)
{
    if (ar.IsStoring())
    {	// storing code
    }
    else
    {	// loading code
    }

    m_listOfConnections.Serialize(ar);
}