﻿// CUIItemUpgrade.h: interface for the CUIItemUpgrade class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_UIItemUpgrade_H__fd9f5093_0ed3_4c08_9e31_19c40773b24d__INCLUDED_)
#define AFX_UITRANSACTIONDLG_H__fd9f5093_0ed3_4c08_9e31_19c40773b24d__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <N3Base/N3UIBase.h>
#include <N3Base/N3UIArea.h>
#include <N3Base/N3UIString.h>
#include <N3Base/N3UIImage.h>
#include <N3Base/N3UIButton.h>
#include "GameProcedure.h"

#include "GameDef.h"
#include "N3UIWndBase.h"



//////////////////////////////////////////////////////////////////////

class CUIItemUpgrade : public CN3UIWndBase
{
	friend class CUIInventory;

public:
	// 직접 접근해야 할 객체 참조 포인터
	__IconItemSkill* m_pMyUpgradeSLot[MAX_ITEM_UPGRADE_SLOT]; // Upgrade and Trina Scroll Slot
	__IconItemSkill* m_pMyUpgradeInv[MAX_ITEM_INVENTORY];
	__IconItemSkill* m_pBackupUpgradeInv[MAX_ITEM_INVENTORY];
	__IconItemSkill* m_pUpgradeItemSlot;	// Which item to upgrade
	__IconItemSkill* m_pUpgradeResultSlot;
	int8_t m_iUpgradeSlotInvPos[MAX_ITEM_UPGRADE_SLOT+1];
	CN3UIString* m_pStrMyGold;
	
	// Animation state management
	enum AnimationState {
		ANIM_NONE = 0,
		ANIM_START,
		ANIM_FLIPFLOP,
		ANIM_RESULT,
		ANIM_COVER_OPENING,
		ANIM_DONE
	} m_eAnimationState = ANIM_NONE;
	
	float m_fAnimationTimer = 0.0f;
	int m_iCurrentFrame = 0;
	bool m_bUpgradeSuccesfull = false;
	bool m_bReceivedResultFromServer = false;
	
	// Cover animation data
	int m_iCoverShift = 0;
	RECT m_rcCover1Original;
	RECT m_rcCover2Original;


	CUIImageTooltipDlg* m_pUITooltipDlg;


	//this_ui_add_start
	CN3UIButton* m_pBtnClose;
	CN3UIButton* m_pBtnOk;
	CN3UIButton* m_pBtnCancel;
	CN3UIButton* m_pBtnConversation;
	CN3UIArea* m_pAreaUpgrade;
	CN3UIArea* m_pAreaResult;
	CN3UIImage* m_pImageCover1;
	CN3UIImage* m_pImageCover2;
	//this_ui_add_end


protected:
	int					GetItemiOrder(__IconItemSkill* spItem, e_UIWND_DISTRICT eWndDist);
	RECT GetSampleRect();
	e_UIWND_DISTRICT	GetWndDistrict(__IconItemSkill* spItem);
	void				SendToServerUpgradeMsg(int itemID, byte pos, int iCount)
	{};

public:
	CUIItemUpgrade();
	~CUIItemUpgrade() override;
	void				Release() override;

	//this_ui_add_start
	bool				OnKeyPress(int iKey) override;
	void				UpdateBackupUpgradeInv();
	bool				Load(HANDLE hFile) override;
	void				SetVisibleWithNoSound(bool bVisible, bool bWork = false, bool bReFocus = false) override;
	void				SetVisible(bool bVisible) override;
	//this_ui_add_end

	uint32_t			MouseProc(uint32_t dwFlags, const POINT& ptCur, const POINT& ptOld) override;
	bool				ReceiveMessage(CN3UIBase* pSender, uint32_t dwMsg) override;
	void				Render() override;
	void				Close();
	void				Open();

	void				InitIconWnd(e_UIWND eWnd) override;

	__IconItemSkill* GetHighlightIconItem(CN3UIIcon* pUIIcon) override;

	void				IconRestore();

	bool				ReceiveIconDrop(__IconItemSkill* spItem, POINT ptCur) override;
	void				CancelIconDrop(__IconItemSkill* spItem) override;
	void				AcceptIconDrop(__IconItemSkill* spItem) override;
	void				ItemMoveFromInvToThis();
	void				ItemMoveFromThisToInv();
	void                RestoreInventoryFromBackup();
	void				GoldUpdate();
	void				InitIconUpdate()
	{};
	bool				IsUpgradeScroll(uint32_t dwID);
	bool IsTrina(uint32_t dwID);
	bool				IsAllowedUpgradeItem(__IconItemSkill* spItem);
	void				DeleteIconItemSkill(__IconItemSkill*& pItem);
	void				SendToServerUpgradeMsg();
	void				MsgRecv_ItemUpgrade(Packet& pkt);
	void				FlipFlopAnim();
	void				AnimClose();
	void				ShowResultUpgrade();
	void				StartUpgradeAnim();
	void				UpdateCoverAnimation();
	void				UpdateFlipFlopAnimation();
	void				HideAllAnimationFrames();
	void				CreateUIIconForItem(__IconItemSkill* pItem, const std::string& szIconFN = "");
	__IconItemSkill*	CreateIconFromSource(__IconItemSkill* pSrc, int count);
	void				SetupIconArea(__IconItemSkill* pItem, CN3UIArea* pArea);
	bool				HandleUpgradeAreaDrop(__IconItemSkill* spItem, POINT ptCur);
	bool				HandleSlotDrop(__IconItemSkill* spItem, int iDestiOrder);
	bool				IsSlotCompatible(__IconItemSkill* pSrc, int iDestiOrder);
	void				Tick() override;
};

#endif // !defined(AFX_UIItemUpgrade_H__fd9f5093_0ed3_4c08_9e31_19c40773b24d__INCLUDED_)
