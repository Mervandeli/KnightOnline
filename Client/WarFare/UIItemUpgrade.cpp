#include "StdAfx.h"
#include "UIItemUpgrade.h"
#include "GameProcMain.h"
#include "UIManager.h"
#include "UIInventory.h"
#include "LocalInput.h"
#include "GameBase.h"
#include "PlayerMySelf.h"

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

void CUIItemUpgrade::Tick()
{
    UpdateGold();


}



void CUIItemUpgrade::UpdateGold()
{
    
	CN3UIString* pStatic = (CN3UIString* )GetChildByID("text_gold"); __ASSERT(pStatic, "NULL UI Component!!");
	if(pStatic)
	{
		char szBuff[32] = "";
		sprintf(szBuff, "%d", CGameBase::s_pPlayer->m_InfoExt.iGold);
		int buffLength = strlen(szBuff);
		int goldLength = buffLength + (buffLength / 3) - (buffLength % 3 == 0 ? 1 : 0);

		char szGold[42] = "";
		szGold[goldLength] = '\0';

		for (int i = buffLength - 1, k = goldLength - 1; i >= 0; i--, k--)
		{
			if ((buffLength - 1 - i) % 3 == 0 && (buffLength - 1 != i))
				szGold[k--] = ',';

			szGold[k] = szBuff[i];
		}


		pStatic->SetString(szGold);
	}
    
}

