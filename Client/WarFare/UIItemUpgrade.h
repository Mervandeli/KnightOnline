#pragma once

#include <N3BASE/N3UIBase.h>
#include <N3BASE/N3UIButton.h>

class CUIItemUpgrade : public CN3UIBase
{
public:
    CUIItemUpgrade();
    virtual ~CUIItemUpgrade();

    bool Load(HANDLE hFile);
    bool ReceiveMessage(CN3UIBase* pSender, uint32_t dwMsg);
    void SetVisible(bool bVisible);
    void SetNpcID(int iNpcID) { m_iNpcID = iNpcID; }

protected:
    CN3UIButton*    m_pBtn_Close;
    int             m_iNpcID;
}; 