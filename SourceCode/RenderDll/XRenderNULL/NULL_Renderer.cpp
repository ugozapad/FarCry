//////////////////////////////////////////////////////////////////////
//
//  Crytek CryENGINE Source code
//  
//  File:NULL_Renderer.cpp
//  Description: Implementation of the NULL renderer API
//
//  History:
//  -Jan 31,2001:Originally created by Marco Corbetta
//	-: taken over by Andrey Khonich
//
//////////////////////////////////////////////////////////////////////

#include "RenderPCH.h"
#include "NULL_Renderer.h"

// init memory pool usage
#ifndef PS2
#ifndef _XBOX
//#if !defined(LINUX)
//_ACCESS_POOL;
//#endif
#endif
#endif

#include "limits.h"

CNULLRenderer *gcpNULL = NULL;

//////////////////////////////////////////////////////////////////////
CNULLRenderer::CNULLRenderer()
{
  gcpNULL = this;

#ifdef DEBUGALLOC
#undef new
#endif
  m_TexMan = new CNULLTexMan;
#ifdef DEBUGALLOC
#define new DEBUG_CLIENTBLOCK
#endif
  
}


#include <stdio.h>
//////////////////////////////////////////////////////////////////////
CNULLRenderer::~CNULLRenderer()
{ 
  ShutDown(); 
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::EnableTMU(bool enable)
{ 
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::CheckError(const char *comment)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::BeginFrame()
{
}

//////////////////////////////////////////////////////////////////////
bool CNULLRenderer::ChangeDisplay(unsigned int width,unsigned int height,unsigned int bpp)
{
  return false;
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::ChangeViewport(unsigned int x,unsigned int y,unsigned int width,unsigned int height)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::Update()
{
  m_TexMan->Update();    
}

void CNULLRenderer::GetMemoryUsage(ICrySizer* Sizer)
{
}

WIN_HWND CNULLRenderer::GetHWND()
{
  return NULL;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
//IMAGES DRAWING
////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::Draw2dImage(float xpos,float ypos,float w,float h,int texture_id,float s0,float t0,float s1,float t1,float angle,float r,float g,float b,float a, float z)
{ 
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::DrawImage(float xpos,float ypos,float w,float h,int texture_id,float s0,float t0,float s1,float t1,float r,float g,float b,float a)
{ 
}

///////////////////////////////////////////
void CNULLRenderer::SetCullMode(int mode)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//ENVI/BLEND MODES
////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////
//FOG
////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////
void CNULLRenderer::SetFog(float density,float fogstart,float fogend,const float *color,int fogmode)
{
}

///////////////////////////////////////////
bool CNULLRenderer::EnableFog(bool enable)
{
  return false;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//TEXGEN 
////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////
void CNULLRenderer::EnableTexGen(bool enable)
{
}

///////////////////////////////////////////
void CNULLRenderer::SetTexgen(float scaleX,float scaleY,float translateX,float translateY)
{        
}

void CNULLRenderer::SetTexgen3D(float x1, float y1, float z1, float x2, float y2, float z2)
{        
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//MISC EXTENSIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////
void CNULLRenderer::SetLodBias(float value)
{  
}

///////////////////////////////////////////
void CNULLRenderer::EnableVSync(bool enable)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::SelectTMU(int tnum)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//MATRIX FUNCTIONS
////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////
void CNULLRenderer::PushMatrix()
{
}

///////////////////////////////////////////
void CNULLRenderer::RotateMatrix(float a,float x,float y,float z)
{
}

void CNULLRenderer::RotateMatrix(const Vec3d & angles)
{
}

///////////////////////////////////////////
void CNULLRenderer::TranslateMatrix(float x,float y,float z)
{
}

void CNULLRenderer::MultMatrix(float * mat)
{
}

void CNULLRenderer::TranslateMatrix(const Vec3d &pos)
{
}

///////////////////////////////////////////
void CNULLRenderer::ScaleMatrix(float x,float y,float z)
{
}

///////////////////////////////////////////
void CNULLRenderer::PopMatrix()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
void CNULLRenderer::LoadMatrix(const Matrix44 *src)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//MISC
////////////////////////////////////////////////////////////////////////////////////////////////////////

void CNULLRenderer::Flush3dBBox(const Vec3d &mins,const Vec3d &maxs,const bool bSolid)
{
}

///////////////////////////////////////////
void CNULLRenderer::Draw3dBBox(const Vec3d &mins,const Vec3d &maxs, int nPrimType)
{
}


///////////////////////////////////////////
int CNULLRenderer::SetPolygonMode(int mode)
{
  return 0;
}


///////////////////////////////////////////
void CNULLRenderer::SetPerspective(const CCamera &cam)
{
}

///////////////////////////////////////////
void CNULLRenderer::SetCamera(const CCamera &cam)
{
  m_cam=cam;
}

void CNULLRenderer::SetViewport(int x, int y, int width, int height)
{
}

void CNULLRenderer::SetScissor(int x, int y, int width, int height)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::GetModelViewMatrix(float * mat)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::GetModelViewMatrix(double *mat)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::GetProjectionMatrix(double *mat)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::GetProjectionMatrix(float *mat)
{
}

//////////////////////////////////////////////////////////////////////
Vec3d CNULLRenderer::GetUnProject(const Vec3d &WindowCoords,const CCamera &cam)
{
  return (Vec3d(0,0,0));
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::DrawQuad(const Vec3d &right, const Vec3d &up, const Vec3d &origin,int nFlipmode/*=0*/)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::ProjectToScreen( float ptx, float pty, float ptz, float *sx, float *sy, float *sz )
{
}

int CNULLRenderer::UnProject(float sx, float sy, float sz, 
              float *px, float *py, float *pz,
              const float modelMatrix[16], 
              const float projMatrix[16], 
              const int    viewport[4])
{
  return 0;
}

//////////////////////////////////////////////////////////////////////
int CNULLRenderer::UnProjectFromScreen( float  sx, float  sy, float  sz, 
                                      float *px, float *py, float *pz)
{
  return 0;
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::Draw2dLine	(float x1,float y1,float x2,float y2)
{
}

void CNULLRenderer::DrawLine(const Vec3d & vPos1, const Vec3d & vPos2)
{
}

void CNULLRenderer::DrawLineColor(const Vec3d & vPos1, const CFColor & vColor1, const Vec3d & vPos2, const CFColor & vColor2)
{
}


//////////////////////////////////////////////////////////////////////
void CNULLRenderer::ScreenShot(const char *filename)
{
}

int CNULLRenderer::ScreenToTexture()
{ // for death effects
  return 0;
}

void CNULLRenderer::ResetToDefault()
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//FONT RENDERING
////////////////////////////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////
void CNULLRenderer::WriteXY(CXFont *currfont,int x, int y, float xscale,float yscale,float r,float g,float b,float a,const char *message, ...)
{	
}

void CNULLRenderer::SetMaterialColor(float r, float g, float b, float a)
{
}

char * CNULLRenderer::GetStatusText(ERendStats type)
{
  return NULL;
}

void CNULLRenderer::DrawBall(float x, float y, float z, float radius )
{
}

void CNULLRenderer::DrawBall(const Vec3d & pos, float radius )
{
}

void CNULLRenderer::DrawPoint(float x, float y, float z, float fSize)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::ClearDepthBuffer()
{
}

void CNULLRenderer::ClearColorBuffer(const Vec3d vColor)
{
}

void CNULLRenderer::ReadFrameBuffer(unsigned char * pRGB, int nSizeX, int nSizeY, bool bBackBuffer, bool bRGBA, int nScaledX, int nScaledY)
{
}

void CNULLRenderer::SetFogColor(float * color)
{
}

void CNULLRenderer::TransformTextureMatrix(float x, float y, float angle, float scale)
{
}

void CNULLRenderer::ResetTextureMatrix()
{
}


void CNULLRenderer::SetClipPlane( int id, float * params )
{
}

void CNULLRenderer::DrawQuad(float dy,float dx, float dz, float x, float y, float z)
{
}

void CNULLRenderer::EnableAALines(bool bEnable)
{
}

//////////////////////////////////////////////////////////////////////
void CNULLRenderer::Set2DMode(bool enable, int ortox, int ortoy)
{ 
}

// ps2 to create matrix
void CNULLRenderer::MakeMatrix(const Vec3d & pos, const Vec3d & angles,const Vec3d & scale, Matrix44 * mat)
{
}

namespace ATL
{
    int __cdecl _AtlInitializeCriticalSectionEx(struct _RTL_CRITICAL_SECTION*, unsigned long, unsigned long)
    {
        return 0;
    }
}

//=========================================================================================

