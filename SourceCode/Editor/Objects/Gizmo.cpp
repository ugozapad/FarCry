////////////////////////////////////////////////////////////////////////////
//
//  Crytek Engine Source File.
//  Copyright (C), Crytek Studios, 2002.
// -------------------------------------------------------------------------
//  File name:   gizmo.cpp
//  Version:     v1.00
//  Created:     2/7/2002 by Timur.
//  Compilers:   Visual Studio.NET
//  Description: 
// -------------------------------------------------------------------------
//  History:
//
////////////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Gizmo.h"

#include "ObjectManager.h"

//////////////////////////////////////////////////////////////////////////
// CGizmo implementation.
//////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////
CGizmo::CGizmo()
{
	m_bDelete = false;
	m_matrix.SetIdentity();
	m_flags = 0;
}

//////////////////////////////////////////////////////////////////////////
CGizmo::~CGizmo()
{
}

//////////////////////////////////////////////////////////////////////////
void CGizmo::SetMatrix( const Matrix44 &tm )
{
	m_matrix = tm;
}

//////////////////////////////////////////////////////////////////////////
IGizmoManager* CGizmo::GetGizmoManager() const
{
	return GetIEditor()->GetObjectManager()->GetGizmoManager();
}

//////////////////////////////////////////////////////////////////////////
void CGizmo::DeleteThis()
{
	m_bDelete = true;
};