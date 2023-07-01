#include "stdafx.h"
#include "CTriangulator.h"
#include <math.h>
#include <algorithm>
#include <ISystem.h>


#if defined(WIN32) && defined(_DEBUG) 
#include <crtdbg.h> 
#define DEBUG_NEW_NORMAL_CLIENTBLOCK(file, line) new(_NORMAL_BLOCK, file, line) 
#define new DEBUG_NEW_NORMAL_CLIENTBLOCK( __FILE__, __LINE__) 
#endif



CTriangulator::CTriangulator()
{
	m_vtxBBoxMin.x = 3000;
	m_vtxBBoxMin.y = 3000;
	m_vtxBBoxMin.z = 0;

	m_vtxBBoxMax.x = 0;
	m_vtxBBoxMax.y = 0;
	m_vtxBBoxMax.z = 0;

}

CTriangulator::~CTriangulator()
{
	if (!m_vVertices.empty())
		m_vVertices.clear();

/*	if (!m_vTriangles.empty())
	{
		std::list<Tri *>::iterator i;		
		for (i=m_vTriangles.begin();i!=m_vTriangles.end();i++)
		{
			Tri* tri= (*i);
			delete tri;
		}

		m_vTriangles.clear();
			
	}
	*/

	if (!m_lstUnique.empty())
		m_lstUnique.clear();

	if (!m_vProcessed.empty())
		m_vTriangles.clear();
	
}


int CTriangulator::AddVertex(float x, float y, float z, void *pForeign, bool bImportant)
{
	Vtx newvertex; 
	newvertex.x  = x;
	newvertex.y  = y;
	newvertex.z  = z;
	newvertex.pForeign = pForeign;
	newvertex.bImportant = bImportant;

	if (newvertex.x < m_vtxBBoxMin.x)
		m_vtxBBoxMin.x = newvertex.x;
	if (newvertex.x > m_vtxBBoxMax.x)
		m_vtxBBoxMax.x = newvertex.x;

	if (newvertex.y < m_vtxBBoxMin.y)
		m_vtxBBoxMin.y = newvertex.y;
	if (newvertex.y > m_vtxBBoxMax.y)
		m_vtxBBoxMax.y = newvertex.y ;


	VARRAY::iterator i;
	i = std::find(m_vVertices.begin(),m_vVertices.end(),newvertex);

	if (i!=m_vVertices.end())
		return -1;
	
	m_vVertices.push_back(newvertex);

	return (m_vVertices.size()-1);
}


bool CTriangulator::Triangulate()
{

	if (m_vVertices.empty()) return true;

	// init supertriangle and structures
	if (!PrepForTriangulation())
		return false;

	// perform triangulation on any new vertices
	if (!TriangulateNew())
		return false;

	// cleanup supertriangle
	FinalizeTriangulation();
	return true;
}


VARRAY CTriangulator::GetVertices()
{
	return m_vProcessed;
}

TARRAY CTriangulator::GetTriangles()
{
	return m_vTriangles;
}




void CTriangulator::PushUnique(int a, int b)
{

	MYPOINT newpoint,oldpoint;
	newpoint.x = a;
	newpoint.y = b;
	oldpoint.x = b;
	oldpoint.y = a;
//	bool found = false;
	std::list<MYPOINT>::iterator i;

	if ((i=std::find(m_lstUnique.begin(),m_lstUnique.end(),oldpoint)) == m_lstUnique.end())
		m_lstUnique.push_back(newpoint);
	else
	{
		m_lstUnique.erase(i);
	}

	//m_lstUnique.sort();
	
/*
	if (m_lstUnique.empty())
	{
		m_lstUnique.push_back(newpoint);
		return;
	}
	else
	{
		std::list<POINT>::iterator i;

		i = m_lstUnique.begin();

		while (i!=m_lstUnique.end())
		{
			POINT curr = (*i);
			if ((curr.x == b) && (curr.y==a))
			{
				i=m_lstUnique.erase(i);
				found = true;
				break;
			}
		  	i++;
		}


		if (!found) m_lstUnique.push_back(newpoint);
	}
*/
}

bool CTriangulator::IsAntiClockwise(Tri *who)
{
	Vtx v1,v2,v3;

	v1 = m_vProcessed[who->v[0]];
	v2 = m_vProcessed[who->v[1]];
	v3 = m_vProcessed[who->v[2]];

	Vtx vec1,vec2;

	vec1.x = v1.x - v2.x;
	vec1.y = v1.y - v2.y;

	vec2.x = v3.x - v2.x;
	vec2.y = v3.y - v2.y;
	
	float f = vec1.x * vec2.y - vec2.x * vec1.y;	

	if (f>0) return true;

	return false;
}









bool CTriangulator::Calculate(Tri *pTri)
{
	Vtx v1,v2,v3;

	v1 = m_vProcessed[pTri->v[0]];
	v2 = m_vProcessed[pTri->v[1]];
	v3 = m_vProcessed[pTri->v[2]];

	if (!IsPerpendicular(v1, v2, v3) )				
		CalcCircle(v1, v2, v3, pTri);	
	else if (!IsPerpendicular(v1, v3, v2))		
		CalcCircle(v1, v3, v2, pTri);	
	else if (!IsPerpendicular(v2, v1, v3))		
		CalcCircle(v2, v1, v3, pTri);	
	else if (!IsPerpendicular(v2, v3, v1))		
		CalcCircle(v2, v3, v1, pTri);	
	else if (!IsPerpendicular(v3, v2, v1))		
		CalcCircle(v3, v2, v1, pTri);	
	else if (!IsPerpendicular(v3, v1, v2))		
		CalcCircle(v3, v1, v2, pTri);	
	else { 
		// should not get here
	
	//	char str[1024];
	//	sprintf(str,"These points are collinear: (%.2f,%.2f,%.2f) - (%.2f,%.2f,%.2f) - (%.2f,%.2f,%.2f)\n",v1.x,v1.y,v1.z,v2.x,v2.y,v2.z,v3.x,v3.y,v3.z);
	//	OutputDebugString(str);
		AIWarning("These points are collinear: (%.2f,%.2f,%.2f) - (%.2f,%.2f,%.2f) - (%.2f,%.2f,%.2f)\n",v1.x,v1.y,v1.z,v2.x,v2.y,v2.z,v3.x,v3.y,v3.z);
		pTri->radius=0;
		return false;
	}
	return true;
}

bool CTriangulator::IsPerpendicular(const Vtx &v1, const Vtx &v2, const Vtx &v3)
{

	double yDelta_a= v2.y - v1.y;
	double xDelta_a= v2.x - v1.x;
	double yDelta_b= v3.y - v2.y;
	double xDelta_b= v3.x - v2.x;
	
	// checking whether the line of the two pts are vertical
	if (fabs(xDelta_a) <= 0.000000001 && fabs(yDelta_b) <= 0.000000001)
		return false;

	if (fabs(yDelta_a) <= 0.0000001)
		return true;
	else if (fabs(yDelta_b) <= 0.0000001)
		return true;
	else if (fabs(xDelta_a)<= 0.000000001)
		return true;
	else if (fabs(xDelta_b)<= 0.000000001)
		return true;
	else 
		return false;

}

void CTriangulator::CalcCircle(const Vtx &v1, const Vtx &v2, const Vtx &v3, Tri *pTri)
{
	double yDelta_a= v2.y - v1.y;
	double xDelta_a= v2.x - v1.x;
	double yDelta_b= v3.y - v2.y;
	double xDelta_b= v3.x - v2.x;
	
	if (fabs(xDelta_a) <= 0.000000001 && fabs(yDelta_b) <= 0.000000001)
	{

		pTri->center.x = 0.5f*(v2.x + v3.x);
		pTri->center.y = 0.5f*(v1.y + v2.y);
		pTri->center.z = v1.z;
		pTri->radius = (pTri->center.x - v1.x)*(pTri->center.x - v1.x) + (pTri->center.y - v1.y)*(pTri->center.y - v1.y);
		return;
	}
	
	// IsPerpendicular() assure that xDelta(s) are not zero
	double aSlope=yDelta_a/xDelta_a; // 
	double bSlope=yDelta_b/xDelta_b;
	if (fabs(aSlope-bSlope) <= 0.000000001)
	{	
		// checking whether the given points are colinear. 
/*		char which[1024];
		sprintf(which,"vertices (%.2f,%.2f,%.2f),(%.2f,%.2f,%.2f),(%.2f,%.2f,%.2f) caused assert \n",v1.x,v1.y,v1.z,v2.x,v2.y,v2.z,v3.x,v3.y,v3.z);
		sprintf(which,"ouside data is %d\n",(int)v1.pForeign);
		OutputDebugString(&which[0]);
		DEBUG_BREAK; // we should never get here!!! 
*/
		CryError("vertices (%.2f,%.2f,%.2f),(%.2f,%.2f,%.2f),(%.2f,%.2f,%.2f) caused assert \n",v1.x,v1.y,v1.z,v2.x,v2.y,v2.z,v3.x,v3.y,v3.z);
		return;
	}

	// calc center
	pTri->center.x =(float) ( (aSlope*bSlope*(v1.y - v3.y) + bSlope*(v1.x + v2.x) - aSlope*(v2.x+v3.x) )/(2.f * (bSlope-aSlope) ) );
	pTri->center.y =(float) ( -(pTri->center.x - (v1.x + v2.x)/2.f ) / aSlope +  (v1.y+v2.y)/2.f );
	pTri->center.z = v1.z;

	pTri->radius =  (pTri->center.x - v1.x)*(pTri->center.x - v1.x) + (pTri->center.y - v1.y)*(pTri->center.y - v1.y) ;
}



bool CTriangulator::PrepForTriangulation(void)
{
	m_vProcessed.reserve(m_vVertices.size());

	m_vProcessed.clear();
	// calculate super-triangle
	VARRAY::iterator i;
	Vtx min,max;
	min.x = 100000.f;
	min.y = 100000.f;
	max.x = 0.f;
	max.y = 0.f;

// bounding rectangle
	for (i=m_vVertices.begin();i!=m_vVertices.end();i++)
	{
		Vtx current = (*i);
		if (current.x < min.x) min.x = current.x;
		if (current.y < min.y) min.y = current.y;
		if (current.x > max.x) max.x = current.x;
		if (current.y > max.y) max.y = current.y;
	}

	Vtx sv1,sv2,sv3;
	/*
	sv1.x = (max.x - min.x) + max.x;
	sv1.y = max.y + (max.y - min.y);

	sv3.y = max.y + (max.y - min.y);

	sv2.x = min.x + (max.x - min.x) / 2;
	sv2.y = min.y - (max.x - min.x) *2;

	sv3.x = min.x - (max.x - min.x);
	*/

	float xsize = max.x - min.x;
	float ysize = max.y - min.y;

	sv1.x = min.x + xsize/2.f;
	sv1.y = min.y - xsize/2.f;		
	sv1.y -= xsize*0.5f;

	sv2.x = min.x - (ysize+(ysize*0.5f));
	sv2.y = max.y + (ysize*0.5f);

	sv3.x = max.x + (ysize+(ysize*0.5f));
	sv3.y = max.y + (ysize*0.5f);


	m_vProcessed.push_back(sv1);
	m_vProcessed.push_back(sv2);
	m_vProcessed.push_back(sv3);


	Tri *newtri = new Tri;
	newtri->v[0] = 0;
	newtri->v[1] = 1;
	newtri->v[2] = 2;

	if (!Calculate(newtri))
		return false;

	m_vTriangles.push_back(newtri);
	return true;
}

void CTriangulator::FinalizeTriangulation(void)
{
	// delete triangles that use super vertices
	TARRAY::iterator dr;

	for (dr=m_vTriangles.begin();dr!=m_vTriangles.end();)
	{
		Tri *now = (*dr);

		if ((now->v[0] < 3) || (now->v[1] < 3) || (now->v[2] < 3))
		{
			dr = m_vTriangles.erase(dr);
			delete now;
		}
		else
			dr++;
	}

}

bool CTriangulator::TriangulateNew(void)
{
	VARRAY::iterator i;

	int la=0;	
	// for every vertex
	for (i=m_vVertices.begin();i!=m_vVertices.end();i++)
	{
		Vtx current = (*i);

//		char str[255];
//		sprintf(str,"Now on vertex %d of %d [%.3f,%.3f,%.3f]\n",la++,m_vVertices.size(),current.x,current.y, current.z);
//		OutputDebugString(str);
		m_lstUnique.clear();
		// find enclosing circles
		TARRAY::iterator ti,tinext;


		for (ti=m_vTriangles.begin();ti!=m_vTriangles.end();)
		{
			Tri *triangle = (*ti);

			float dist = (current.x - triangle->center.x)*(current.x - triangle->center.x) +(current.y - triangle->center.y)*(current.y - triangle->center.y);

			if (dist <= triangle->radius)
			{
				PushUnique(triangle->v[0],triangle->v[1]);
				PushUnique(triangle->v[1],triangle->v[2]);
				PushUnique(triangle->v[2],triangle->v[0]);
				tinext = ti;
				tinext++;
				m_vTriangles.erase(ti);
				delete triangle;
				ti = tinext;
			}
			else 
				ti++;

		}


		// add new triangles	
		int pos = m_vProcessed.size();
		m_vProcessed.push_back(current);


		std::list<MYPOINT>::iterator ui;

		for (ui=m_lstUnique.begin();ui!=m_lstUnique.end();ui++)
		{
			MYPOINT curr = (*ui);

			Tri *newone = new Tri;
			newone->v[0]=curr.x;
			newone->v[1]=curr.y;
			newone->v[2]=pos;

			if (!IsAntiClockwise(newone))
			{
				newone->v[0]=curr.y;
				newone->v[1]=curr.x;
			}

			if (!Calculate(newone))
				return false;

			m_vTriangles.push_back(newone);
		}




	}
	m_vVertices.clear();

	return true;
}
