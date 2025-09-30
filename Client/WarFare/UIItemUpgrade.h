#pragma once

#include <N3Base/N3UIBase.h>

#include "GameDef.h"

enum e_UIWND_DISTRICT;
struct __IconItemSkill;
class CN3UIIcon;
class CUIImageTooltipDlg;
class CUIItemUpgrade : public CN3UIBase
{
private:
	static constexpr int MAX_ITEM_UPGRADE_SLOT	= 9;	// Max items in the item upgrade UI.
	static constexpr int MAX_UPGRADE_MATERIAL	= 10;	// Max items in the item upgrade UI.
	static constexpr int FLIPFLOP_MAX_FRAMES	= 19;

	enum class AnimationState
	{
		None,
		Start,
		FlipFlop,
		Result,
		CoverOpening,
		Done
	};

	AnimationState m_eAnimationState;
	
	enum ItemBasic {
		UpgradeMaterial = 255,
	};

	float				m_fAnimationTimer;
	int					m_iCurrentFrame;
	bool				m_bUpgradeSucceeded;
	bool				m_bUpgradeInProgress;
	int					m_iNpcID;

	RECT				m_rcCover1Original;
	RECT				m_rcCover2Original;

	CN3UIButton*		m_pBtnClose;
	CN3UIButton*		m_pBtnOk;
	CN3UIButton*		m_pBtnCancel;
	CN3UIButton*		m_pBtnConversation;
	CN3UIString*		m_pStrMyGold;
	CN3UIArea*			m_pAreaUpgrade;
	CN3UIArea*			m_pAreaResult;
	CN3UIArea*			m_pInvArea[MAX_ITEM_INVENTORY];
	CN3UIString*		m_pInvString[MAX_ITEM_INVENTORY];
	CN3UIArea*			m_pSlotArea[MAX_ITEM_UPGRADE_SLOT];
	CN3UIImage*			m_pImgFail[FLIPFLOP_MAX_FRAMES];
	CN3UIImage*			m_pImgSuccess[FLIPFLOP_MAX_FRAMES];
	CN3UIImage*			m_pImageCover1;
	CN3UIImage*			m_pImageCover2;

	CUIImageTooltipDlg* m_pUITooltipDlg;

	__IconItemSkill*	m_pSelectedItem;
	int					m_iSelectedItemSourcePos;

	__IconItemSkill*	m_pMyUpgradeInv[MAX_ITEM_INVENTORY];
	int8_t				m_iUpgradeScrollSlotInvPos[MAX_ITEM_UPGRADE_SLOT];
	int8_t				m_iUpgradeItemSlotInvPos;

public:
	CUIItemUpgrade();
	~CUIItemUpgrade() override;
	void Release() override;
	void Close();
	void Open();
	void SetVisibleWithNoSound(bool bVisible, bool bWork = false, bool bReFocus = false) override;
	void MsgRecv_ItemUpgrade(Packet& pkt);
	void SetNpcID(int iNpcID);

private:
	RECT GetSampleRect();
	e_UIWND_DISTRICT GetWndDistrict() const;
	bool HandleInventoryIconRightClick(__IconItemSkill* spItem);
	void ShowItemCount(__IconItemSkill* spItem, int iOrder);
	bool OnKeyPress(int iKey) override;
	bool Load(HANDLE hFile) override;

	uint32_t MouseProc(uint32_t dwFlags, const POINT& ptCur, const POINT& ptOld) override;
	bool ReceiveMessage(CN3UIBase* pSender, uint32_t dwMsg) override;
	void Render() override;

	void SetSelectedIconInfo(CN3UIIcon* pUIIcon);
	void CancelIconDrop(__IconItemSkill* spItem);
	bool ReceiveIconDrop(__IconItemSkill* spItem);
	void GetItemFromInv();
	void ResetUpgradeInventory();
	void GoldUpdate();
	bool IsTrina(uint32_t dwID) const;
	bool IsAllowedUpgradeItem(const __IconItemSkill* spItem) const;
	void SendToServerUpgradeMsg();

	void FlipFlopAnim();
	void AnimClose();
	void StartUpgradeAnim();
	void UpdateCoverAnimation();
	void UpdateFlipFlopAnimation();
	void HideAllAnimationFrames();
	void CreateUIIconForItem(__IconItemSkill* spItem);
	void SetupIconArea(__IconItemSkill* spItem, CN3UIArea* pArea);
	bool MaterialSlotDrop(__IconItemSkill* spItem, int iOrder);
	void CleanAreaSlot(CN3UIArea* pArea);
	bool IsMaterialSlotCompatible(__IconItemSkill* pSrc) const;
	void Tick() override;
};
