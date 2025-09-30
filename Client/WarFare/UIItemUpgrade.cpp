#include "stdafx.h"
#include "UIItemUpgrade.h"
#include "APISocket.h"
#include "GameProcMain.h"
#include "IconItemSkill.h"
#include "LocalInput.h"
#include "N3UIIcon.h"
#include "N3UIWndBase.h"
#include "PlayerMySelf.h"
#include "UIImageTooltipDlg.h"
#include "UIInventory.h"

#include "text_resources.h"

#include <N3Base/N3UIArea.h>
#include <N3Base/N3UIString.h>
#include <N3Base/N3UIImage.h>
#include <N3Base/N3UIButton.h>

#include <unordered_set>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

CUIItemUpgrade::CUIItemUpgrade()
{
	m_eAnimationState = AnimationState::None;
	m_fAnimationTimer = 0.0f;
	m_iCurrentFrame = 0;
	m_bUpgradeSucceeded = false;
	m_bUpgradeInProgress = false;
	m_iNpcID = 0;

	m_rcCover1Original = {};
	m_rcCover2Original = {};

	m_pBtnClose = nullptr;
	m_pBtnOk = nullptr;
	m_pBtnCancel = nullptr;
	m_pBtnConversation = nullptr;
	m_pStrMyGold = nullptr;
	m_pAreaUpgrade = nullptr;
	m_pAreaResult = nullptr;

	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		m_pInvArea[i] = nullptr;
		m_pInvString[i] = nullptr;
		m_pMyUpgradeInv[i] = nullptr;
	}

	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		m_pSlotArea[i] = nullptr;
		m_iUpgradeScrollSlotInvPos[i] = -1;
	}

	for (int i = 0; i < FLIPFLOP_MAX_FRAMES; i++)
	{
		m_pImgFail[i] = nullptr;
		m_pImgSuccess[i] = nullptr;
	}

	m_pImageCover1 = nullptr;
	m_pImageCover2 = nullptr;

	m_pUITooltipDlg = nullptr;

	m_pSelectedItem = nullptr;
	m_iSelectedItemSourcePos = -1;

	m_iUpgradeItemSlotInvPos = -1;
}

CUIItemUpgrade::~CUIItemUpgrade()
{
	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		if (m_pMyUpgradeInv[i] != nullptr)
		{
			m_pMyUpgradeInv[i]->pUIIcon = nullptr;
			delete m_pMyUpgradeInv[i];
			m_pMyUpgradeInv[i] = nullptr;
		}
	}
}

void CUIItemUpgrade::Release()
{
	m_eAnimationState = AnimationState::None;
	m_fAnimationTimer = 0.0f;
	m_iCurrentFrame = 0;
	m_bUpgradeSucceeded = false;
	m_bUpgradeInProgress = false;
	m_iNpcID = 0;

	m_rcCover1Original = {};
	m_rcCover2Original = {};

	m_pBtnClose = nullptr;
	m_pBtnOk = nullptr;
	m_pBtnCancel = nullptr;
	m_pBtnConversation = nullptr;
	m_pStrMyGold = nullptr;
	m_pAreaUpgrade = nullptr;
	m_pAreaResult = nullptr;

	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		m_pInvArea[i] = nullptr;
		m_pInvString[i] = nullptr;

		if (m_pMyUpgradeInv[i] != nullptr)
		{
			m_pMyUpgradeInv[i]->pUIIcon = nullptr;
			delete m_pMyUpgradeInv[i]->pUIIcon;
			m_pMyUpgradeInv[i] = nullptr;
		}
	}

	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		m_pSlotArea[i] = nullptr;
		m_iUpgradeScrollSlotInvPos[i] = -1;
	}

	for (int i = 0; i < FLIPFLOP_MAX_FRAMES; i++)
	{
		m_pImgFail[i] = nullptr;
		m_pImgSuccess[i] = nullptr;
	}

	m_pImageCover1 = nullptr;
	m_pImageCover2 = nullptr;

	m_pUITooltipDlg = nullptr;

	m_pSelectedItem = nullptr;
	m_iSelectedItemSourcePos = -1;

	m_iUpgradeItemSlotInvPos = -1;

	CN3UIBase::Release();
}

void CUIItemUpgrade::Tick()
{
	if (m_pImageCover1 != nullptr && m_pImageCover2 != nullptr)
	{
		switch (m_eAnimationState)
		{
			case AnimationState::Start:
				StartUpgradeAnim();
				break;

			case AnimationState::FlipFlop:
				UpdateFlipFlopAnimation();
				break;

			case AnimationState::Result:
				if (m_bUpgradeSucceeded)
					SetupIconArea(m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos], m_pAreaResult);

				m_eAnimationState = AnimationState::CoverOpening;
				break;

			case AnimationState::CoverOpening:
				UpdateCoverAnimation();
				break;

			case AnimationState::None:
				m_fAnimationTimer = 0.0f;
				m_iCurrentFrame = 0;
				break;
		}
	}

	CN3UIBase::Tick();
}

void CUIItemUpgrade::Render()
{
	if (!m_bVisible)
		return;

	POINT ptCur = CGameProcedure::s_pLocalInput->MouseGetPos();

	if (m_pUITooltipDlg != nullptr)
		m_pUITooltipDlg->DisplayTooltipsDisable();

	bool bTooltipRender = false;
	__IconItemSkill* spItem = nullptr;

	for (auto itor = m_Children.rbegin(); m_Children.rend() != itor; ++itor)
	{
		CN3UIBase* pChild = *itor;
		if (GetState() == UI_STATE_ICON_MOVING
			&& pChild->UIType() == UI_TYPE_ICON
			&& m_pSelectedItem != nullptr
			&& pChild == m_pSelectedItem->pUIIcon)
			continue;

		pChild->Render();

		if (GetState() == UI_STATE_COMMON_NONE
			&& pChild->UIType() == UI_TYPE_ICON
			&& pChild->GetStyle() & UISTYLE_ICON_HIGHLIGHT)
		{
			bTooltipRender = true;
			SetSelectedIconInfo((CN3UIIcon*) pChild);
		}
	}

	if (GetState() == UI_STATE_ICON_MOVING
		&& m_pSelectedItem != nullptr
		&& m_pSelectedItem->pUIIcon != nullptr)
		m_pSelectedItem->pUIIcon->Render();

	if (bTooltipRender
		&& m_pSelectedItem != nullptr)
		m_pUITooltipDlg->DisplayTooltipsEnable(ptCur.x, ptCur.y, m_pSelectedItem, false, false);
}

void CUIItemUpgrade::SetSelectedIconInfo(CN3UIIcon* pUIIcon)
{
	POINT ptCur = CGameProcedure::s_pLocalInput->MouseGetPos();
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		int iOrder = m_iUpgradeScrollSlotInvPos[i];
		if (iOrder < 0
			|| iOrder >= MAX_ITEM_INVENTORY)
			continue;
		
		if (!m_pSlotArea[i]->IsIn(ptCur.x, ptCur.y))
			continue;

		if (m_pMyUpgradeInv[iOrder] != nullptr
			&& m_pMyUpgradeInv[iOrder]->pUIIcon != nullptr)
		{
			m_iSelectedItemSourcePos = -1; // Can not move
			m_pSelectedItem = m_pMyUpgradeInv[iOrder];
			return;
		}
	}

	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		if (m_pMyUpgradeInv[i] != nullptr
			&& m_pMyUpgradeInv[i]->pUIIcon != nullptr
			&& m_pMyUpgradeInv[i]->pUIIcon == pUIIcon)
		{
			m_iSelectedItemSourcePos = i;
			m_pSelectedItem = m_pMyUpgradeInv[i];
			return;
		}
	}
}

void CUIItemUpgrade::Open()
{
	SetVisibleWithNoSound(true, false, false);
}

void CUIItemUpgrade::SetNpcID(int iNpcID)
{
	m_iNpcID = iNpcID;
}

void CUIItemUpgrade::GoldUpdate()
{
	if (m_pStrMyGold == nullptr)
		return;

	std::string formattedGold = CGameBase::FormatNumber(CGameBase::s_pPlayer->m_InfoExt.iGold);
	m_pStrMyGold->SetString(formattedGold);
}

void CUIItemUpgrade::GetItemFromInv()
{
	CUIInventory* pInven = CGameProcedure::s_pProcMain->m_pUIInventory;
	if (pInven == nullptr)
		return;

	m_pSelectedItem = nullptr;
	m_iSelectedItemSourcePos = -1;
	m_iUpgradeItemSlotInvPos = -1;

	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
		m_iUpgradeScrollSlotInvPos[i] = -1;

	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		if (m_pMyUpgradeInv[i] != nullptr)
		{
			if (m_pMyUpgradeInv[i]->pUIIcon != nullptr)
			{
				delete m_pMyUpgradeInv[i]->pUIIcon;
				m_pMyUpgradeInv[i]->pUIIcon = nullptr;
			}

			delete m_pMyUpgradeInv[i];
			m_pMyUpgradeInv[i] = nullptr;
		}

		if (pInven->m_pMyInvWnd[i] != nullptr)
		{
			__IconItemSkill* spItem = new __IconItemSkill(*pInven->m_pMyInvWnd[i]);
			CreateUIIconForItem(spItem);
			SetupIconArea(spItem, m_pInvArea[i]);
			ShowItemCount(spItem, i);
			m_pMyUpgradeInv[i] = spItem;
		}
	}
}

void CUIItemUpgrade::Close()
{
	bool bWork = IsVisible();
	SetVisibleWithNoSound(false, bWork, false);
}

bool CUIItemUpgrade::ReceiveIconDrop(__IconItemSkill* spItem)
{
	if (spItem == nullptr)
		return false;

	POINT ptCur = CGameProcedure::s_pLocalInput->MouseGetPos();
	if (IsAllowedUpgradeItem(spItem))
	{
		if (m_iUpgradeItemSlotInvPos == -1
			&& m_pAreaUpgrade->IsIn(ptCur.x, ptCur.y))
		{
			if (m_iUpgradeItemSlotInvPos != -1)
				SetupIconArea(m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos], m_pInvArea[m_iUpgradeItemSlotInvPos]);

			SetupIconArea(spItem, m_pAreaUpgrade);
			m_iUpgradeItemSlotInvPos = m_iSelectedItemSourcePos;
			return true;
		}
	}
	else if (IsMaterialSlotCompatible(spItem))
	{
		for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
		{
			CN3UIArea* pArea = m_pSlotArea[i];
			if (pArea != nullptr
				&& pArea->IsIn(ptCur.x, ptCur.y))
			{
				if (MaterialSlotDrop(spItem, i))
					return true;
			}
		}
	}
	return false;
}

// Restores the icon's position to its original inventory area.
void CUIItemUpgrade::CancelIconDrop(__IconItemSkill* spItem)
{
	if (spItem == nullptr)
		return;

	int iOrder = m_iSelectedItemSourcePos;
	if (iOrder < 0)
		return;

	SetupIconArea(spItem, m_pInvArea[iOrder]);
	ShowItemCount(spItem, iOrder);
	m_pSelectedItem = nullptr;
	m_iSelectedItemSourcePos = -1;
}

uint32_t CUIItemUpgrade::MouseProc(uint32_t dwFlags, const POINT& ptCur, const POINT& ptOld)
{
	uint32_t dwRet = UI_MOUSEPROC_NONE;

	if (!m_bVisible)
		return dwRet;

	if (GetState() == UI_STATE_ICON_MOVING
		&& m_pSelectedItem != nullptr
		&& m_pSelectedItem->pUIIcon != nullptr)
	{
		RECT region = GetSampleRect();
		m_pSelectedItem->pUIIcon->SetRegion(region);
		m_pSelectedItem->pUIIcon->SetMoveRect(region);
	}

	return CN3UIBase::MouseProc(dwFlags, ptCur, ptOld);
}

// Returns a rectangle centered at the mouse position, used for moving icons.
RECT CUIItemUpgrade::GetSampleRect()
{
	__ASSERT(m_pAreaResult, "m_pAreaResult not loaded");

	if (m_pAreaResult == nullptr)
		return {};

	POINT ptCur = CGameProcedure::s_pLocalInput->MouseGetPos();
	RECT rect = m_pAreaResult->GetRegion();

	float fWidth	= (float) (rect.right - rect.left) * 0.5f;
	float fHeight	= (float) (rect.bottom - rect.top) * 0.5f;
	rect.left		= ptCur.x - (int) fWidth;
	rect.right		= ptCur.x + (int) fWidth;
	rect.top		= ptCur.y - (int) fHeight;
	rect.bottom		= ptCur.y + (int) fHeight;
	return rect;
}

e_UIWND_DISTRICT CUIItemUpgrade::GetWndDistrict() const
{
	POINT ptCur = CGameProcedure::s_pLocalInput->MouseGetPos();

	if (m_pAreaUpgrade != nullptr
		&& m_pAreaUpgrade->IsIn(ptCur.x, ptCur.y))
		return UIWND_DISTRICT_UPGRADE_SLOT;

	if (m_pAreaResult != nullptr
		&& m_pAreaResult->IsIn(ptCur.x, ptCur.y))
		return UIWND_DISTRICT_UPGRADE_RESULT_SLOT;

	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		if (m_pSlotArea[i] != nullptr
			&& m_pSlotArea[i]->IsIn(ptCur.x, ptCur.y))
			return UIWND_DISTRICT_UPGRADE_SLOT;
	}

	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		if (m_pInvArea[i] != nullptr
			&& m_pInvArea[i]->IsIn(ptCur.x, ptCur.y))
			return UIWND_DISTRICT_UPGRADE_INV;
	}

	return UIWND_DISTRICT_UNKNOWN;
}

// Handles UI messages such as button clicks and icon events.
bool CUIItemUpgrade::ReceiveMessage(CN3UIBase* pSender, uint32_t dwMsg)
{
	if (pSender == nullptr)
		return false;

	if (dwMsg == UIMSG_BUTTON_CLICK)
	{
		if (pSender == m_pBtnClose)
		{
			Close();
		}
		else if (pSender == m_pBtnCancel)
		{
			if (!m_bUpgradeInProgress)
				ResetUpgradeInventory();
		}
		else if (pSender == m_pBtnOk)
		{
			if (!m_bUpgradeInProgress)
				SendToServerUpgradeMsg();
		}

		return true;
	}

	__IconItemSkill* spItem = nullptr;
	e_UIWND_DISTRICT  eUIWnd = UIWND_DISTRICT_UNKNOWN;

	int iOrder = -1;

	switch (dwMsg)
	{
		case UIMSG_ICON_DOWN_FIRST:
		{
			spItem = m_pSelectedItem;
			iOrder = m_iSelectedItemSourcePos;
			eUIWnd = GetWndDistrict();

			if (eUIWnd == UIWND_DISTRICT_UPGRADE_RESULT_SLOT)
				ResetUpgradeInventory();

			if (iOrder == -1
				|| eUIWnd != UIWND_DISTRICT_UPGRADE_INV)
			{
				SetState(UI_STATE_COMMON_NONE);
				return false;
			}

			// Divide countable items
			if (spItem->iCount > 1
				&& spItem->IsStackable())
			{
				__IconItemSkill* pNew = new __IconItemSkill(*spItem);
				CreateUIIconForItem(pNew);
				ShowItemCount(spItem, iOrder);
				m_pSelectedItem = pNew;
			}

			// Set icon region for moving.
			RECT region = GetSampleRect();
			m_pSelectedItem->pUIIcon->SetRegion(region);
			m_pSelectedItem->pUIIcon->SetMoveRect(region);
		}
		break;

		case UIMSG_ICON_UP:
			spItem = m_pSelectedItem;
			iOrder = m_iSelectedItemSourcePos;
			if (spItem == nullptr)
				break;

			if (!ReceiveIconDrop(spItem))
			{
				if (spItem->iCount > 1
					&& spItem->IsStackable())
				{
					ShowItemCount(spItem, iOrder);

					// Clean divided item
					if (spItem != nullptr)
					{
						if (spItem->pUIIcon != nullptr)
						{
							delete spItem->pUIIcon;
							spItem->pUIIcon = nullptr;
						}

						delete spItem;
						spItem = nullptr;
					}
				}
				else
				{
					// Restore the icon position to its original place if drop failed.
					CancelIconDrop(spItem);
				}
			}
			break;

		case UIMSG_ICON_DOWN:
			spItem = m_pSelectedItem;

			if (GetState() == UI_STATE_ICON_MOVING
				&& spItem != nullptr
				&& spItem->pUIIcon != nullptr)
			{
				RECT region = GetSampleRect();
				spItem->pUIIcon->SetRegion(region);
				spItem->pUIIcon->SetMoveRect(region);
			}
			break;

		case UIMSG_ICON_RDOWN_FIRST:
			if (!m_bUpgradeInProgress)
				HandleInventoryIconRightClick(m_pSelectedItem);
			break;
	}
	return true;
}

void CUIItemUpgrade::SetVisibleWithNoSound(bool bVisible, bool bWork, bool bReFocus)
{
	CN3UIBase::SetVisibleWithNoSound(bVisible, bWork, bReFocus);

	if (bVisible)
	{
		GetItemFromInv();
		GoldUpdate();
	}
	else // if (!bVisible)
	{
		if (bWork)
		{
			if (m_pUITooltipDlg != nullptr)
				m_pUITooltipDlg->DisplayTooltipsDisable();

			if (GetState() == UI_STATE_ICON_MOVING)
				CancelIconDrop(m_pSelectedItem);

			// Reset the item's inventory area.
			ResetUpgradeInventory();
			AnimClose();
		}
	}
}

// Loads the UI from file and initializes all required UI components.
bool CUIItemUpgrade::Load(HANDLE hFile)
{
	if (!CN3UIBase::Load(hFile))
		return false;

	std::string szID;

	N3_VERIFY_UI_COMPONENT(m_pBtnClose,			GetChildByID<CN3UIButton>("btn_close"));
	N3_VERIFY_UI_COMPONENT(m_pBtnOk,			GetChildByID<CN3UIButton>("btn_ok"));
	N3_VERIFY_UI_COMPONENT(m_pBtnCancel,		GetChildByID<CN3UIButton>("btn_cancel"));
	N3_VERIFY_UI_COMPONENT(m_pBtnConversation,	GetChildByID<CN3UIButton>("btn_conversation"));
	N3_VERIFY_UI_COMPONENT(m_pAreaUpgrade,		GetChildByID<CN3UIArea>("a_upgrade"));
	N3_VERIFY_UI_COMPONENT(m_pAreaResult,		GetChildByID<CN3UIArea>("a_result"));
	N3_VERIFY_UI_COMPONENT(m_pImageCover1,		GetChildByID<CN3UIImage>("img_cover_01"));
	N3_VERIFY_UI_COMPONENT(m_pImageCover2,		GetChildByID<CN3UIImage>("img_cover_02"));
	N3_VERIFY_UI_COMPONENT(m_pStrMyGold,		GetChildByID<CN3UIString>("text_gold"));

	if (m_pStrMyGold != nullptr)
		m_pStrMyGold->SetString("0");

	if (m_pImageCover1 != nullptr)
		m_pImageCover1->SetVisible(false);

	if (m_pImageCover2 != nullptr)
		m_pImageCover2->SetVisible(false);

	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		szID = fmt::format("a_m_{}", i);
		N3_VERIFY_UI_COMPONENT(m_pSlotArea[i],		GetChildByID<CN3UIArea>(szID));
	}

	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		szID = fmt::format("a_slot_{}", i);
		N3_VERIFY_UI_COMPONENT(m_pInvArea[i],		GetChildByID<CN3UIArea>(szID));

		szID = fmt::format("s_count_{}", i);
		N3_VERIFY_UI_COMPONENT(m_pInvString[i],		GetChildByID<CN3UIString>(szID));
	}

	for (int i = 0; i < FLIPFLOP_MAX_FRAMES; ++i)
	{
		szID = fmt::format("img_s_load_{}", i);
		N3_VERIFY_UI_COMPONENT(m_pImgSuccess[i],	GetChildByID<CN3UIImage>(szID));

		szID = fmt::format("img_f_load_{}", i);
		N3_VERIFY_UI_COMPONENT(m_pImgFail[i],		GetChildByID<CN3UIImage>(szID));

		if (m_pImgFail[i] != nullptr)
			m_pImgFail[i]->SetVisible(false);

		if (m_pImgSuccess[i] != nullptr)
			m_pImgSuccess[i]->SetVisible(false);
	}

	__TABLE_UI_RESRC* pTbl = CGameBase::s_pTbl_UI.Find(CGameBase::s_pPlayer->Nation());
	__ASSERT(pTbl != nullptr, "__TABLE_UI_RESRC: missing nation");
	if (pTbl != nullptr)
	{
		if (m_pUITooltipDlg == nullptr)
			m_pUITooltipDlg = new CUIImageTooltipDlg();

		m_pUITooltipDlg->Init(this);
		m_pUITooltipDlg->LoadFromFile(pTbl->szItemInfo);
		m_pUITooltipDlg->InitPos();
		m_pUITooltipDlg->SetVisible(false);
	}

	return true;
}

// Handles key press events, such as closing the UI with ESC.
bool CUIItemUpgrade::OnKeyPress(int iKey)
{
	switch (iKey)
	{
		case DIK_ESCAPE:
			Close();
			return true;
	}

	return CN3UIBase::OnKeyPress(iKey);
}

// Restores the inventory and slots.
void CUIItemUpgrade::ResetUpgradeInventory()
{
	if (m_iUpgradeItemSlotInvPos != -1)
	{
		SetupIconArea(m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos], m_pInvArea[m_iUpgradeItemSlotInvPos]);
		m_iUpgradeItemSlotInvPos = -1;
	}

	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		int iOrder = m_iUpgradeScrollSlotInvPos[i];
		if (iOrder == -1)
			continue;

		__IconItemSkill* spItem = m_pMyUpgradeInv[iOrder];
		if (spItem != nullptr)
		{
			SetupIconArea(spItem, m_pInvArea[iOrder]);

			if (spItem->iCount > 1
				&& spItem->IsStackable())
			{
				CleanAreaSlot(m_pSlotArea[i]);
				++spItem->iCount;
			}

			ShowItemCount(spItem, iOrder);
		}
	}

	m_bUpgradeSucceeded = false;
	m_bUpgradeInProgress = false;
}

bool CUIItemUpgrade::IsTrina(uint32_t dwID) const
{
	static const std::unordered_set<int> upgradeItemIDs =
	{
		// Trina
		379256000, 379257000, 379258000, 700002000
	};
	return upgradeItemIDs.contains(dwID);
}

// Checks if the given item is allowed to be upgraded (unique or upgrade type).
bool CUIItemUpgrade::IsAllowedUpgradeItem(const __IconItemSkill* spItem) const
{
	if (m_iUpgradeItemSlotInvPos != -1)
		return false;

	if (spItem == nullptr
		|| spItem->pItemBasic == nullptr)
		return false;

	switch (spItem->pItemBasic->byAttachPoint)
	{
		case ITEM_POS_DUAL:
		case ITEM_POS_RIGHTHAND:
		case ITEM_POS_LEFTHAND:
		case ITEM_POS_TWOHANDRIGHT:
		case ITEM_POS_TWOHANDLEFT:
		case ITEM_POS_SHOES:
		case ITEM_POS_GLOVES:
		case ITEM_POS_HEAD:
		case ITEM_POS_LOWER:
		case ITEM_POS_UPPER:
			break;

		default:
			return false;
	}

	e_ItemAttrib eTA = (e_ItemAttrib) (spItem->pItemExt->byMagicOrRare);
	return (eTA == ITEM_ATTRIB_UNIQUE || eTA == ITEM_ATTRIB_UPGRADE || eTA == ITEM_ATTRIB_UNIQUE_REVERSE);
}

void CUIItemUpgrade::SendToServerUpgradeMsg()
{
	if (m_iUpgradeItemSlotInvPos < 0
		|| m_iUpgradeItemSlotInvPos >= MAX_ITEM_INVENTORY
		|| m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos] == nullptr)
		return;

	uint8_t byBuff[512];
	int iOffset = 0;
	int iTotalSent = 0;

	m_bUpgradeInProgress = true;

	CAPISocket::MP_AddByte(byBuff, iOffset, WIZ_ITEM_UPGRADE);
	CAPISocket::MP_AddByte(byBuff, iOffset, ITEM_UPGRADE_PROCESS);
	CAPISocket::MP_AddWord(byBuff, iOffset, m_iNpcID);

	// Add item to upgrade
	CAPISocket::MP_AddDword(byBuff, iOffset, m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos]->GetItemID());
	CAPISocket::MP_AddByte(byBuff, iOffset, m_iUpgradeItemSlotInvPos);
	++iTotalSent;

	// Add requirement items
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		int8_t iOrder = m_iUpgradeScrollSlotInvPos[i];
		if (iOrder < 0
			|| iOrder >= MAX_ITEM_INVENTORY)
			continue;

		CAPISocket::MP_AddDword(byBuff, iOffset, m_pMyUpgradeInv[iOrder]->GetItemID());
		CAPISocket::MP_AddByte(byBuff, iOffset, iOrder);
		++iTotalSent;
	}

	while (iTotalSent < MAX_UPGRADE_MATERIAL)
	{
		CAPISocket::MP_AddDword(byBuff, iOffset, 0); // Item ID
		CAPISocket::MP_AddByte(byBuff, iOffset, -1); // Position
		++iTotalSent;
	}

	CGameProcedure::s_pSocket->Send(byBuff, iOffset);
}

void CUIItemUpgrade::MsgRecv_ItemUpgrade(Packet& pkt)
{
	CUIInventory* pInven = CGameProcedure::s_pProcMain->m_pUIInventory;
	if (pInven == nullptr)
		return;

	uint32_t nItemID[MAX_UPGRADE_MATERIAL];
	uint8_t byPos[MAX_UPGRADE_MATERIAL];

	int8_t result = pkt.read<uint8_t>();

	for (int i = 0; i < MAX_UPGRADE_MATERIAL; i++)
	{
		nItemID[i] = pkt.read<uint32_t>();
		byPos[i] = pkt.read<uint8_t>();
	}

	if (byPos[0] < MAX_ITEM_INVENTORY)
		m_iUpgradeItemSlotInvPos = byPos[0];

	std::string szMsg;

	if (result == ITEM_UPGRADE_RESULT_FAILED)
	{
		for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
		{
			int iOrder = m_iUpgradeScrollSlotInvPos[i];
			if (iOrder != -1)
			{
				__IconItemSkill* spItem = m_pMyUpgradeInv[iOrder];
				if (spItem != nullptr)
				{
					if (spItem->iCount > 1)
					{
						--spItem->iCount;
						--pInven->m_pMyInvWnd[iOrder]->iCount;
					}
					else
					{
						if (pInven->m_pMyInvWnd[iOrder] != nullptr)
						{
							delete pInven->m_pMyInvWnd[iOrder]->pUIIcon;
							pInven->m_pMyInvWnd[iOrder]->pUIIcon = nullptr;
							pInven->m_pMyInvWnd[iOrder] = nullptr;
						}

						if (spItem != nullptr)
							spItem->pUIIcon = nullptr;

						spItem = nullptr;
					}
				}
			}

			CleanAreaSlot(m_pSlotArea[i]);
			m_iUpgradeScrollSlotInvPos[i] = -1;
		}

		if (pInven->m_pMyInvWnd[m_iUpgradeItemSlotInvPos] != nullptr)
		{
			if (pInven->m_pMyInvWnd[m_iUpgradeItemSlotInvPos]->pUIIcon != nullptr)
			{
				delete pInven->m_pMyInvWnd[m_iUpgradeItemSlotInvPos]->pUIIcon;
				pInven->m_pMyInvWnd[m_iUpgradeItemSlotInvPos]->pUIIcon = nullptr;
			}

			delete pInven->m_pMyInvWnd[m_iUpgradeItemSlotInvPos];
			pInven->m_pMyInvWnd[m_iUpgradeItemSlotInvPos] = nullptr;
		}

		if (m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos] != nullptr)
		{
			if (m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos]->pUIIcon != nullptr)
			{
				delete m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos]->pUIIcon;
				m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos]->pUIIcon = nullptr;
			}

			delete m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos];
			m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos] = nullptr;
		}

		m_iUpgradeItemSlotInvPos = -1;

		m_bUpgradeSucceeded = false;
		m_eAnimationState = AnimationState::Start;

		szMsg = fmt::format_text_resource(IDS_ITEM_UPGRADE_FAILED);
		CGameProcedure::s_pProcMain->MsgOutput(szMsg, D3DCOLOR_XRGB(255, 0, 255));
	}
	else if (result == ITEM_UPGRADE_RESULT_SUCCEEDED)
	{
		for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
		{
			int iOrder = m_iUpgradeScrollSlotInvPos[i];
			if (iOrder != -1)
			{
				__IconItemSkill* spItem = m_pMyUpgradeInv[iOrder];
				if (spItem != nullptr)
				{
					if (spItem->iCount > 1)
					{
						--spItem->iCount;
						--pInven->m_pMyInvWnd[iOrder]->iCount;
					}
					else
					{
						if (pInven->m_pMyInvWnd[iOrder] != nullptr)
							pInven->m_pMyInvWnd[iOrder]->pUIIcon = nullptr;
						pInven->m_pMyInvWnd[iOrder] = nullptr;

						if (spItem != nullptr)
							spItem->pUIIcon = nullptr;
						spItem = nullptr;
					}
				}
			}

			CleanAreaSlot(m_pSlotArea[i]);
			m_iUpgradeScrollSlotInvPos[i] = -1;
		}

		if (m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos] != nullptr)
		{
			if (m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos]->pUIIcon != nullptr)
			{
				delete m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos]->pUIIcon;
				m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos]->pUIIcon = nullptr;
			}

			delete m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos];
			m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos] = nullptr;
		}

		m_bUpgradeSucceeded = true;

		if (m_bUpgradeInProgress)
			m_eAnimationState = AnimationState::Start;

		szMsg = fmt::format_text_resource(IDS_ITEM_UPGRADE_SUCCEEDED);
		CGameProcedure::s_pProcMain->MsgOutput(szMsg, D3DCOLOR_XRGB(128, 128, 255));

		e_PartPosition ePart;
		e_PlugPosition ePlug;
		e_ItemType eItemType;
		std::string szIconFN;
		constexpr float fUVAspect = (float) 45.0f / (float) 64.0f;

		__TABLE_ITEM_BASIC* pItemBasic = CGameBase::s_pTbl_Items_Basic.Find(nItemID[0] / 1000 * 1000);
		__TABLE_ITEM_EXT* pItemExt = nullptr;

		if (pItemBasic != nullptr
			&& pItemBasic->byExtIndex >= 0
			&& pItemBasic->byExtIndex < MAX_ITEM_EXTENSION)
			pItemExt = CGameBase::s_pTbl_Items_Exts[pItemBasic->byExtIndex].Find(nItemID[0] % 1000);

		eItemType = CGameBase::MakeResrcFileNameForUPC(pItemBasic, pItemExt, nullptr, &szIconFN, ePart, ePlug, CGameBase::s_pPlayer->Race());
		if (eItemType == ITEM_TYPE_UNKNOWN)
			return;

		__IconItemSkill* spItemNew = new __IconItemSkill();
		spItemNew->pItemBasic = pItemBasic;
		spItemNew->pItemExt = pItemExt;
		spItemNew->szIconFN = szIconFN;
		spItemNew->iCount = 1;
		CreateUIIconForItem(spItemNew);

		m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos] = spItemNew;

		// Upgrade item in inventory
		pInven->m_pMyInvWnd[m_iUpgradeItemSlotInvPos]->pItemBasic = spItemNew->pItemBasic;
		pInven->m_pMyInvWnd[m_iUpgradeItemSlotInvPos]->pItemExt = spItemNew->pItemExt;
		pInven->m_pMyInvWnd[m_iUpgradeItemSlotInvPos]->iDurability = spItemNew->iDurability;

	}
	else if (result == ITEM_UPGRADE_RESULT_TRADING)
	{
		m_bUpgradeInProgress = false;
		ResetUpgradeInventory();

		szMsg = fmt::format_text_resource(IDS_ITEM_UPGRADE_CANNOT_PERFORM);
		CGameProcedure::s_pProcMain->MsgOutput(szMsg, D3DCOLOR_XRGB(255, 0, 255));
	}
	else if (result == ITEM_UPGRADE_RESULT_NEED_COINS)
	{
		m_bUpgradeInProgress = false;
		ResetUpgradeInventory();

		szMsg = fmt::format_text_resource(IDS_ITEM_UPGRADE_NEED_COINS);
		CGameProcedure::s_pProcMain->MsgOutput(szMsg, D3DCOLOR_XRGB(255, 0, 255));
	}
	else if (result == ITEM_UPGRADE_RESULT_NO_MATCH)
	{
		m_bUpgradeInProgress = false;
		ResetUpgradeInventory();

		szMsg = fmt::format_text_resource(IDS_ITEM_UPGRADE_NON_MATCH);
		CGameProcedure::s_pProcMain->MsgOutput(szMsg, D3DCOLOR_XRGB(255, 0, 255));
	}

	GoldUpdate();
}

void CUIItemUpgrade::UpdateCoverAnimation()
{
	constexpr float COVER_ANIMATION_DURATION = 0.8f;

	if (m_pImageCover1 == nullptr
		|| m_pImageCover2 == nullptr)
		return;

	m_fAnimationTimer += CN3Base::s_fSecPerFrm;

	float t = m_fAnimationTimer / COVER_ANIMATION_DURATION;

	// Only handle opening (covers move outward and hide)
	float ease = 1.0f - ((1.0f - t) * (1.0f - t));
	int coverShift = m_rcCover1Original.bottom - m_rcCover1Original.top;

	// Calculate new Y positions for opening
	int y1 = m_rcCover1Original.top + (int) ((m_rcCover1Original.top - coverShift - m_rcCover1Original.top) * ease);
	int y2 = m_rcCover2Original.top + (int) ((m_rcCover2Original.top + coverShift - m_rcCover2Original.top) * ease);

	// Animation completed - hide covers and reset positions
	if (t >= 1.0f)
	{
		m_eAnimationState = AnimationState::None;

		m_pImageCover1->SetVisible(false);
		m_pImageCover2->SetVisible(false);
		m_pImageCover1->SetRegion(m_rcCover1Original);
		m_pImageCover2->SetRegion(m_rcCover2Original);

		m_bUpgradeInProgress = false;
		return;
	}

	// Update cover regions
	RECT rc1 = m_rcCover1Original;
	RECT rc2 = m_rcCover2Original;
	int height1 = rc1.bottom - rc1.top;
	int height2 = rc2.bottom - rc2.top;
	rc1.top = y1;
	rc1.bottom = y1 + height1;
	rc2.top = y2;
	rc2.bottom = y2 + height2;

	m_pImageCover1->SetRegion(rc1);
	m_pImageCover2->SetRegion(rc2);
}

void CUIItemUpgrade::UpdateFlipFlopAnimation()
{
	constexpr float FLIPFLOP_FRAME_DELAY = 0.1f;

	m_fAnimationTimer += CN3Base::s_fSecPerFrm;

	if (m_fAnimationTimer >= FLIPFLOP_FRAME_DELAY)
	{
		m_fAnimationTimer -= FLIPFLOP_FRAME_DELAY;
		++m_iCurrentFrame;

		if (m_iCurrentFrame >= FLIPFLOP_MAX_FRAMES)
		{
			HideAllAnimationFrames();

			m_eAnimationState = AnimationState::Result;
			m_fAnimationTimer = 0.0f;
		}
		else
		{
			FlipFlopAnim();
		}
	}
}

void CUIItemUpgrade::HideAllAnimationFrames()
{
	for (int i = 0; i < FLIPFLOP_MAX_FRAMES; i++)
	{
		// Hide all img_s_load_X frames
		if (m_pImgSuccess[i] != nullptr)
			m_pImgSuccess[i]->SetVisible(false);

		// Hide all img_f_load_X frames
		if (m_pImgFail[i] != nullptr)
			m_pImgFail[i]->SetVisible(false);
	}
}

void CUIItemUpgrade::CreateUIIconForItem(__IconItemSkill* spItem)
{
	if (spItem == nullptr)
		return;

	spItem->pUIIcon = new CN3UIIcon();
	spItem->pUIIcon->Init(this);

	constexpr float UV_ASPECT_RATIO = 45.0f / 64.0f;
	spItem->pUIIcon->SetTex(spItem->szIconFN);
	spItem->pUIIcon->SetUVRect(0, 0, UV_ASPECT_RATIO, UV_ASPECT_RATIO);
	spItem->pUIIcon->SetUIType(UI_TYPE_ICON);
	spItem->pUIIcon->SetStyle(UISTYLE_ICON_ITEM | UISTYLE_ICON_CERTIFICATION_NEED);
	spItem->pUIIcon->SetVisible(true);
}

void CUIItemUpgrade::SetupIconArea(__IconItemSkill* spItem, CN3UIArea* pArea)
{
	if (spItem == nullptr
		|| spItem->pUIIcon == nullptr
		|| pArea == nullptr)
		return;

	spItem->pUIIcon->SetRegion(pArea->GetRegion());
	spItem->pUIIcon->SetMoveRect(pArea->GetRegion());
}

bool CUIItemUpgrade::IsMaterialSlotCompatible(__IconItemSkill* pSrc) const
{
	if (m_bUpgradeInProgress)
		return false;

	if (pSrc->pItemBasic->dwEffectID2 != ITEM_EFFECT2_ITEM_UPGRADE_REQ)
		return false;

	bool bHasTrina = false;
	bool bHasScroll = false;

	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		int iOrder = m_iUpgradeScrollSlotInvPos[i];
		if (iOrder < 0)
			continue;

		if (IsTrina(m_pMyUpgradeInv[iOrder]->pItemBasic->dwID))
			bHasTrina = true;
		else
			bHasScroll = true;
	}

	if (bHasTrina && IsTrina(pSrc->pItemBasic->dwID))
		return false;

	if (bHasScroll && !IsTrina(pSrc->pItemBasic->dwID))
		return false;

	if (bHasTrina && bHasScroll)
		return false;

	return true;
}

void CUIItemUpgrade::FlipFlopAnim()
{
	if (m_bUpgradeSucceeded)
	{
		// Hide previous frame
		if (m_iCurrentFrame > 0
			&& m_pImgSuccess[m_iCurrentFrame - 1] != nullptr)
			m_pImgSuccess[m_iCurrentFrame - 1]->SetVisible(false);

		// Show current frame
		if (m_pImgSuccess[m_iCurrentFrame] != nullptr)
		{
			m_pImgSuccess[m_iCurrentFrame]->SetVisible(true);
			m_pImgSuccess[m_iCurrentFrame]->SetParent(this); // Re-order for rendering
		}
	}
	else
	{
		// Hide previous frame
		if (m_iCurrentFrame > 0
			&& m_pImgFail[m_iCurrentFrame - 1] != nullptr)
			m_pImgFail[m_iCurrentFrame - 1]->SetVisible(false);

		// Show current frame
		if (m_pImgFail[m_iCurrentFrame] != nullptr)
		{
			m_pImgFail[m_iCurrentFrame]->SetVisible(true);
			m_pImgFail[m_iCurrentFrame]->SetParent(this); // Re-order for rendering
		}
	}
}

void CUIItemUpgrade::AnimClose()
{
	if (m_eAnimationState != AnimationState::None)
	{
		m_eAnimationState = AnimationState::None;
		m_fAnimationTimer = 0.0f;
		m_iCurrentFrame = 0;

		if (m_pImageCover1 != nullptr)
			m_pImageCover1->SetVisible(false);

		if (m_pImageCover2 != nullptr)
			m_pImageCover2->SetVisible(false);
	}

	m_bUpgradeInProgress = false;
	HideAllAnimationFrames();
}

void CUIItemUpgrade::StartUpgradeAnim()
{
	if (!m_bUpgradeInProgress)
		return;

	if (m_pImageCover1 == nullptr
		|| m_pImageCover2 == nullptr)
		return;

	m_fAnimationTimer = 0.0f;
	m_iCurrentFrame = 0;

	// Make covers visible during animation
	m_pImageCover1->SetVisible(true);
	m_pImageCover2->SetVisible(true);
	m_pImageCover1->SetParent(this); // Re-order for rendering
	m_pImageCover2->SetParent(this);

	// save original positions
	m_rcCover1Original = m_pImageCover1->GetRegion();
	m_rcCover2Original = m_pImageCover2->GetRegion();
	m_eAnimationState = AnimationState::FlipFlop;
}

bool CUIItemUpgrade::HandleInventoryIconRightClick(__IconItemSkill* spItem)
{
	// Move upgrade result to inv
	if (m_bUpgradeSucceeded)
		ResetUpgradeInventory();

	if (spItem == nullptr || spItem->pUIIcon == nullptr)
		return false;

	POINT ptCur = CGameProcedure::s_pLocalInput->MouseGetPos();
	if (spItem->pUIIcon->IsVisible()
		&& spItem->pUIIcon->IsIn(ptCur.x, ptCur.y))
	{
		if (IsAllowedUpgradeItem(spItem))
		{
			SetupIconArea(spItem, m_pAreaUpgrade);
			m_iUpgradeItemSlotInvPos = m_iSelectedItemSourcePos;

			return true;
		}
		
		if (IsMaterialSlotCompatible(spItem))
		{
			// Split stackable items
			if (spItem->iCount > 1
				&& spItem->IsStackable())
			{
				__IconItemSkill* pNew = new __IconItemSkill(*spItem);
				CreateUIIconForItem(pNew);
				spItem = pNew;
			}

			for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
			{
				if (m_iUpgradeScrollSlotInvPos[i] != -1)
					continue;

				if (MaterialSlotDrop(spItem, i))
					return true;
			}
		}
	}

	return false;
}

void CUIItemUpgrade::ShowItemCount(__IconItemSkill* spItem, int iOrder)
{
	// Display the count for items that should show a count.
	if (spItem == nullptr)
		return;

	if (spItem->IsStackable())
	{
		CN3UIString* pStr = m_pInvString[iOrder];
		if (pStr == nullptr)
			return;

		if (spItem->iCount > 1)
		{
			pStr->SetStringAsInt(spItem->iCount);
			if (GetState() == UI_STATE_ICON_MOVING && m_pSelectedItem != nullptr)
				pStr->SetStringAsInt(spItem->iCount - 1);
			pStr->SetVisible(true);
			pStr->Render();
			pStr->SetParent(this);
		}
		else
		{
			pStr->SetVisible(false);
		}
	}
	else
	{
		CN3UIString* pStr = m_pInvString[iOrder];
		if (pStr != nullptr)
			pStr->SetVisible(false);
	}
}

bool CUIItemUpgrade::MaterialSlotDrop(__IconItemSkill* spItem, int iOrder)
{
	CN3UIArea* pArea = m_pSlotArea[iOrder];
	int iSourceOrder = m_iSelectedItemSourcePos;

	// If countable reduce inv item count
	if (spItem->iCount > 1
		&& spItem->IsStackable())
		--m_pMyUpgradeInv[iSourceOrder]->iCount;

	SetupIconArea(spItem, pArea);
	ShowItemCount(m_pMyUpgradeInv[iSourceOrder], iSourceOrder); // Update inv item count
	m_iUpgradeScrollSlotInvPos[iOrder] = iSourceOrder;

	return true;
}

void CUIItemUpgrade::CleanAreaSlot(CN3UIArea* pArea)
{
	for (auto itor = m_Children.rbegin(); m_Children.rend() != itor; ++itor)
	{
		CN3UIBase* pChild = (*itor);
		int x = pChild->GetRegion().left / 2 + pChild->GetRegion().right / 2;
		int y = pChild->GetRegion().top / 2 + pChild->GetRegion().bottom / 2;
		if (pChild->UIType() == UI_TYPE_ICON
			&& pArea->IsIn(x, y))
			delete pChild;
	}
}
