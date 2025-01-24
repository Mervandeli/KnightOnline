#include "StdAfx.h"
#include "UIItemUpgrade.h"
#include "GameProcMain.h"
#include "UIManager.h"

CUIItemUpgrade::CUIItemUpgrade()
{
    m_pBtn_Close = nullptr;
    m_iNpcID = 0;
}

CUIItemUpgrade::~CUIItemUpgrade()
{
}

bool CUIItemUpgrade::Load(HANDLE hFile)
{
    if (CN3UIBase::Load(hFile) == false) return false;

    m_pBtn_Close = (CN3UIButton*)GetChildByID("btn_close");__ASSERT(m_pBtn_Close, "NULL UI Component!!");
    m_pBtn_Cancel = (CN3UIButton*)GetChildByID("btn_cancel");__ASSERT(m_pBtn_Cancel, "NULL UI Component!!");
    m_pBtn_Ok = (CN3UIButton*)GetChildByID("btn_ok");__ASSERT(m_pBtn_Ok, "NULL UI Component!!");

    m_pText_Gold = (CN3UIString*)GetChildByID("text_gold");__ASSERT(m_pText_Gold, "NULL UI Component!!");
    m_pText_ItemName = (CN3UIString*)GetChildByID("text_itemname");__ASSERT(m_pText_ItemName, "NULL UI Component!!");
    m_pText_ItemLevel = (CN3UIString*)GetChildByID("text_itemlevel");__ASSERT(m_pText_ItemLevel, "NULL UI Component!!");
    m_pText_ItemPrice = (CN3UIString*)GetChildByID("text_itemprice");__ASSERT(m_pText_ItemPrice, "NULL UI Component!!");
    

    return true;
}

bool CUIItemUpgrade::ReceiveMessage(CN3UIBase* pSender, uint32_t dwMsg)
{
    if (pSender->GetID() == m_pBtn_Close->GetID())
    {
        if (dwMsg == UIMSG_BUTTON_CLICK)
        {
            SetVisible(false);
            return true;
        }
    }
    else if (pSender->GetID() == m_pBtn_Cancel->GetID())
    {
        if (dwMsg == UIMSG_BUTTON_CLICK)
        {
            SetVisible(false);
            return true;
        }
    }
    else if (pSender->GetID() == m_pBtn_Ok->GetID())
    {
        if (dwMsg == UIMSG_BUTTON_CLICK)
        {
            SetVisible(false);
            return true;
        }
    }

    return false;
}

void CUIItemUpgrade::SetVisible(bool bVisible)
{
    CN3UIBase::SetVisible(bVisible);
    if (!bVisible)
    {
        m_iNpcID = 0;
    }
} 