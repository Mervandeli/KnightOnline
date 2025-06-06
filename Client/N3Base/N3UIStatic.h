﻿// N3UIStatic.h: interface for the CN3UIStatic class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_N3UISTATIC_H__CF0050A8_2813_40CB_B696_1CB689FDCC41__INCLUDED_)
#define AFX_N3UISTATIC_H__CF0050A8_2813_40CB_B696_1CB689FDCC41__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "N3UIBase.h"

class CN3UIStatic : public CN3UIBase  
{
public:
	CN3UIStatic();
	virtual ~CN3UIStatic();
// Attributes
protected:
	CN3UIString*	m_pBuffOutRef;		// CN3UIString 참조 포인터 (실제는 child리스트에서 관리)
	CN3UIImage*		m_pImageBkGnd;		// 배경 이미지 참조 포인터 (실제는 child리스트에서 관리)
	CN3SndObj*		m_pSnd_Click;	// static이 눌리는 순간 내는 소리
	
// Operations
public:
	virtual const std::string& GetString();
	virtual void	SetString(const std::string& szString);
	virtual void	Release();
//	virtual void	Render();
	virtual bool	Load(HANDLE);
	virtual void	SetRegion(const RECT& Rect);
	virtual uint32_t	MouseProc(uint32_t dwFlags, const POINT& ptCur, const POINT& ptOld);
protected:

#ifdef _N3TOOL
// 툴에서만 사용하는 함수
public:
	virtual	void	operator = (const CN3UIStatic& other);
	virtual bool	Save(HANDLE hFile);
	void			CreateImageAndString();		// 배경 이미지 및 string 생성하기
	CN3UIImage*		GetImageBkGnd() const {return m_pImageBkGnd;}
	CN3UIString*	GetUIString() const {return m_pBuffOutRef;}
	void			DeleteImage();		// 이미지를 사용하지 않는 static일 경우 호출하면 지워진다.
	void			SetSndClick(const std::string& strFileName);
	std::string		GetSndFName_Click() const;
#endif
};	
#endif // !defined(AFX_N3UISTATIC_H__CF0050A8_2813_40CB_B696_1CB689FDCC41__INCLUDED_)
