// CUIItemUpgrade.h: interface for the CUIItemUpgrade class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_UIItemUpgrade_H__fd9f5093_0ed3_4c08_9e31_19c40773b24d__INCLUDED_)
#define AFX_UIUIItemUpgrade_H__fd9f5093_0ed3_4c08_9e31_19c40773b24d__INCLUDED_

#include <N3Base/N3UIArea.h>
#include <N3Base/N3UIString.h>
#include <N3Base/N3UIImage.h>
#include <N3Base/N3UIButton.h>
#include <N3Base/N3UIBase.h>
#include "N3UIIcon.h"
#include "IconItemSkill.h"
#include "UIImageTooltipDlg.h"

const int MAX_ITEM_UPGRADE_SLOT = 9;	// Max items in the item upgrade UI.
const int MAX_UPGRADE_MATERIAL = 10;	// Max items in the item upgrade UI.
const int FLIPFLOP_MAX_FRAMES = 20;

struct __SelectedSkillInfo {
	__IconItemSkill* pSelectedItem;
	int iSourceOrder;
};

enum e_UI_DISTRICT {
	UIWND_DISTRICT_UPGRADE_SLOT,		// Slot district of Result Item.
	UIWND_DISTRICT_UPGRADE_INV,			// Inv district of Item Upgrade Wnd.
	UIWND_DISTRICT_UPGRADE_RESULT_SLOT,	// Upgrade  Result Slot district.
	UIWND_DISTRICT_UPGRADE_UNKNOWN
};

//////////////////////////////////////////////////////////////////////

class CUIItemUpgrade : public CN3UIBase
{

private:
	__SelectedSkillInfo	m_sSelectedIconInfo;
	__IconItemSkill* m_pMyUpgradeInv[MAX_ITEM_INVENTORY];
	__IconItemSkill* m_pUpgradeMaterialSlots[MAX_ITEM_UPGRADE_SLOT];
	int8_t m_iUpgradeScrollSlotInvPos[MAX_ITEM_UPGRADE_SLOT];
	int8_t m_iUpgradeItemSlotInvPos;
	CN3UIString* m_pStrMyGold;
	
	// Animation state management
	enum AnimationState {
		ANIM_NONE = 0,
		ANIM_START,
		ANIM_FLIPFLOP,
		ANIM_RESULT,
		ANIM_COVER_OPENING,
		ANIM_DONE
	};
	AnimationState m_eAnimationState;
	
	enum ItemBasic {
		UpgradeMaterial = 255,
	};

	float m_fAnimationTimer;
	int m_iCurrentFrame;
	bool m_bUpgradeSucceeded;
	bool m_bUpgradeInProgress;
	int m_iNpcID;

	RECT m_rcCover1Original;
	RECT m_rcCover2Original;

	CUIImageTooltipDlg* m_pUITooltipDlg;
	CN3UIButton* m_pBtnClose;
	CN3UIButton* m_pBtnOk;
	CN3UIButton* m_pBtnCancel;
	CN3UIButton* m_pBtnConversation;
	CN3UIArea* m_pAreaUpgrade;
	CN3UIArea* m_pAreaResult;
	CN3UIArea* m_pInvArea[MAX_ITEM_INVENTORY];
	CN3UIString* m_pInvString[MAX_ITEM_INVENTORY];
	CN3UIArea* m_pSlotArea[MAX_ITEM_UPGRADE_SLOT];
	CN3UIImage* m_pImageCover1;
	CN3UIImage* m_pImageCover2;

public:
	CUIItemUpgrade();
	~CUIItemUpgrade() override;
	void				Release() override;
	void				Close();
	void				Open();
	void				SetVisibleWithNoSound(bool bVisible, bool bWork = false, bool bReFocus = false) override;
	void				InitIconWnd();
	void				MsgRecv_ItemUpgrade(Packet& pkt);
	void				SetNpcID(int iNpcID);
private:

	RECT				GetSampleRect();
	e_UI_DISTRICT		GetWndDistrict(__IconItemSkill* spItem) const;
	bool				HandleInventoryIconRightClick(__IconItemSkill* spItem);
	void				ShowItemCount(__IconItemSkill* spItem, int iOrder);
	bool				OnKeyPress(int iKey) override;
	bool				Load(HANDLE hFile) override;
	
	uint32_t			MouseProc(uint32_t dwFlags, const POINT& ptCur, const POINT& ptOld) override;
	bool				ReceiveMessage(CN3UIBase* pSender, uint32_t dwMsg) override;
	void				Render() override;

	void				SetSelectedIconInfo(CN3UIIcon* pUIIcon);
	void				CancelIconDrop(__IconItemSkill* spItem);
	bool				ReceiveIconDrop(__IconItemSkill* spItem, POINT ptCur);
	void				GetItemFromInv();
	void                ResetUpgradeInventory();
	void				GoldUpdate();
	void				InitIconUpdate(){};
	bool				IsTrina(uint32_t dwID) const;
	bool				IsAllowedUpgradeItem(__IconItemSkill* spItem) const;
	void				SendToServerUpgradeMsg();

	void				FlipFlopAnim();
	void				AnimClose();
	void				StartUpgradeAnim();
	void				UpdateCoverAnimation();
	void				UpdateFlipFlopAnimation();
	void				HideAllAnimationFrames();
	void				CreateUIIconForItem(__IconItemSkill* spItem);
	void				SetupIconArea(__IconItemSkill* spItem, CN3UIArea* pArea);
	bool				MaterialSlotDrop(__IconItemSkill* spItem,int iOrder);
	bool				IsMaterialSlotCompatible(__IconItemSkill* pSrc) const;
	void				Tick() override;
};

#endif // !defined(AFX_UIItemUpgrade_H__fd9f5093_0ed3_4c08_9e31_19c40773b24d__INCLUDED_)
