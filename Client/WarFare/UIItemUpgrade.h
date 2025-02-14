﻿#pragma once

#include <N3BASE/N3UIBase.h>
#include <N3BASE/N3UIButton.h>
#include <N3BASE/N3UIString.h>
#include "UIInventory.h"
#include "GameProcMain.h"
#include "GameDef.h"
#include "N3UIWndBase.h"


class CUIItemUpgrade : public CN3UIBase
{
public:
    CUIItemUpgrade();
    virtual ~CUIItemUpgrade();

    bool Load(HANDLE hFile);
    bool ReceiveMessage(CN3UIBase* pSender, uint32_t dwMsg);
    void SetVisible(bool bVisible);
    void SetNpcID(int iNpcID) { m_iNpcID = iNpcID; }
    void Tick();
    void UpdateGold();
    virtual void Release();
    __IconItemSkill* m_pMyInvWnd[MAX_ITEM_INVENTORY];


protected:
    CN3UIButton*    m_pBtn_Close;
    CN3UIButton*    m_pBtn_Cancel;
    CN3UIButton*    m_pBtn_Ok;
    CN3UIString*    m_pText_Gold;
    int             m_iNpcID;
    
}; 