#include "StdAfx.h"
#include "UIRingUpgrade.h"
#include "GameProcMain.h"
#include "UIManager.h"

CUIRingUpgrade::CUIRingUpgrade()
{
    m_pBtn_Close = nullptr;
    m_iNpcID = 0;
}

CUIRingUpgrade::~CUIRingUpgrade()
{
}

bool CUIRingUpgrade::Load(HANDLE hFile)
{
    if (CN3UIBase::Load(hFile) == false) return false;

    m_pBtn_Close = (CN3UIButton*)GetChildByID("btn_close");
    __ASSERT(m_pBtn_Close, "NULL UI Component!!");

    return true;
}

bool CUIRingUpgrade::ReceiveMessage(CN3UIBase* pSender, uint32_t dwMsg)
{
    if (pSender->GetID() == m_pBtn_Close->GetID())
    {
        if (dwMsg == UIMSG_BUTTON_CLICK)
        {
            SetVisible(false);
            return true;
        }
    }

    return false;
}

void CUIRingUpgrade::SetVisible(bool bVisible)
{
    CN3UIBase::SetVisible(bVisible);
    if (!bVisible)
    {
        m_iNpcID = 0;
    }
} 