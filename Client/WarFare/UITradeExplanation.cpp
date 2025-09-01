﻿// UITradeExplanation.cpp: implementation of the CUITradeExplanation class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "UITradeExplanation.h"
#include "UITradeSellBBS.h"

#include <N3Base/N3UIButton.h>
#include <N3Base/N3UIString.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUITradeExplanation::CUITradeExplanation()
{
	m_pBtn_PageUp		= nullptr;
	m_pBtn_PageDown		= nullptr;	
	m_pBtn_Close		= nullptr;
	m_pText_Explanation = nullptr;
	
	m_szExplanation		= "";
	m_iCurSel			= 0;
}

CUITradeExplanation::~CUITradeExplanation()
{

}

void CUITradeExplanation::SetExplanation(int iCurSel, std::string szExplanation)
{
	m_szExplanation = szExplanation;
	m_iCurSel		= iCurSel;
	if(m_pText_Explanation)
	{
		m_pText_Explanation->SetString(m_szExplanation);
	}
}

bool CUITradeExplanation::ReceiveMessage(CN3UIBase *pSender, uint32_t dwMsg)
{
	if( dwMsg == UIMSG_BUTTON_CLICK )
	{
		if(pSender == m_pBtn_PageUp)
		{
			if(m_pParentUI)
			{
				m_pParentUI->CallBackProc(m_iChildID,1);
			}
		}
		else if(pSender == m_pBtn_PageDown)
		{
			if(m_pParentUI)
			{
				m_pParentUI->CallBackProc(m_iChildID,2);
			}
		}
		else if(pSender == m_pBtn_Close)
		{
			SetVisible(false);
		}
	}

	return true;
}

bool CUITradeExplanation::Load(HANDLE hFile)
{
	if(CN3UIBase::Load(hFile)==false) return false;

	N3_VERIFY_UI_COMPONENT(m_pBtn_PageUp, GetChildByID<CN3UIButton>("btn_pageup"));
	N3_VERIFY_UI_COMPONENT(m_pBtn_PageDown, GetChildByID<CN3UIButton>("btn_pagedown"));
	N3_VERIFY_UI_COMPONENT(m_pBtn_Close, GetChildByID<CN3UIButton>("btn_close"));
	N3_VERIFY_UI_COMPONENT(m_pText_Explanation, GetChildByID<CN3UIString>("Text_Title"));

	return true;
}

bool CUITradeExplanation::OnKeyPress(int iKey)
{
	switch(iKey)
	{
	case DIK_PRIOR:
		ReceiveMessage(m_pBtn_PageUp, UIMSG_BUTTON_CLICK);
		return true;
	case DIK_NEXT:
		ReceiveMessage(m_pBtn_PageDown, UIMSG_BUTTON_CLICK);
		return true;
	case DIK_ESCAPE:
		ReceiveMessage(m_pBtn_Close, UIMSG_BUTTON_CLICK);
		return true;
	}

	return CN3UIBase::OnKeyPress(iKey);
}
