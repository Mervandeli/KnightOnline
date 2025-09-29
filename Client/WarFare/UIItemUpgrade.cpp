// UIItemUpgrade.cpp: implementation of the CUIItemUpgrade class.
//Author : Monzantys(Mervan)
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LocalInput.h"
#include "APISocket.h"
#include "GameProcMain.h"
#include "UIItemUpgrade.h"
#include "GameProcedure.h"
#include "GameDef.h"
#include "UIInventory.h"
#include "PlayerMySelf.h"
#include "text_resources.h"
#include <unordered_set>



#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CUIItemUpgrade::CUIItemUpgrade()
{
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		m_iUpgradeScrollSlotInvPos[i] = -1;
	}

	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		m_pMyUpgradeInv[i] = nullptr;
	}
	m_iUpgradeItemSlotInvPos = -1;

	m_pUITooltipDlg = nullptr;
	m_pStrMyGold = nullptr;

	m_eAnimationState = ANIM_NONE;
	m_fAnimationTimer = 0.0f;
	m_iCurrentFrame = 0;
	m_bUpgradeSucceeded = false;
	m_bUpgradeInProgress = false;
	m_iNpcID = 0;
	m_sSelectedIconInfo.pSelectedItem = nullptr;
	m_sSelectedIconInfo.iSourceOrder = -1;
}

CUIItemUpgrade::~CUIItemUpgrade()
{
	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		if (m_pMyUpgradeInv[i] != nullptr)
		{
			if (m_pMyUpgradeInv[i]->pUIIcon != nullptr)
			{
				m_pMyUpgradeInv[i]->pUIIcon = nullptr;
			}
			m_pMyUpgradeInv[i] = nullptr;
		}
	}

	m_pUITooltipDlg = nullptr;
	m_pStrMyGold = nullptr;

	CN3UIBase::Release();
}

void CUIItemUpgrade::Release()
{

	CN3UIBase::Release();
}

void CUIItemUpgrade::Tick()
{
	if (m_pImageCover1 != nullptr && m_pImageCover2 != nullptr)
	{
		switch (m_eAnimationState)
		{
			case ANIM_START:
				StartUpgradeAnim();
				break;
			case ANIM_FLIPFLOP:
				UpdateFlipFlopAnimation();
				break;
			case ANIM_RESULT:
				if (m_bUpgradeSucceeded)
				{
					SetupIconArea(m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos], m_pAreaResult);
				}
				m_eAnimationState = ANIM_COVER_OPENING;
				break;
			case ANIM_COVER_OPENING:
				UpdateCoverAnimation();
				break;
			case ANIM_NONE:
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

	for (UIListReverseItor itor = m_Children.rbegin(); m_Children.rend() != itor; ++itor)
	{
		CN3UIBase* pChild = (*itor);
		if (GetState() == UI_STATE_ICON_MOVING && pChild->UIType() == UI_TYPE_ICON && m_sSelectedIconInfo.pSelectedItem != nullptr &&
			(CN3UIIcon*) pChild == m_sSelectedIconInfo.pSelectedItem->pUIIcon)
			continue;
		pChild->Render();

		if (GetState() == UI_STATE_COMMON_NONE &&
			pChild->UIType() == UI_TYPE_ICON && pChild->GetStyle() & UISTYLE_ICON_HIGHLIGHT)
		{
			bTooltipRender = true;
			SetSelectedIconInfo((CN3UIIcon*) pChild);
		}
	}

	if (GetState() == UI_STATE_ICON_MOVING && m_sSelectedIconInfo.pSelectedItem != nullptr && m_sSelectedIconInfo.pSelectedItem->pUIIcon != nullptr)
		m_sSelectedIconInfo.pSelectedItem->pUIIcon->Render();

	if (bTooltipRender && m_sSelectedIconInfo.pSelectedItem != nullptr)
	{
		m_pUITooltipDlg->DisplayTooltipsEnable(ptCur.x, ptCur.y, m_sSelectedIconInfo.pSelectedItem, false, false);
	}
}

void CUIItemUpgrade::InitIconWnd()
{
	__TABLE_UI_RESRC* pTbl = CGameBase::s_pTbl_UI.Find(CGameBase::s_pPlayer->m_InfoBase.eNation);

	m_pUITooltipDlg = new CUIImageTooltipDlg();
	m_pUITooltipDlg->Init(this);
	m_pUITooltipDlg->LoadFromFile(pTbl->szItemInfo);
	m_pUITooltipDlg->InitPos();
	m_pUITooltipDlg->SetVisible(false);
}

void CUIItemUpgrade::SetSelectedIconInfo(CN3UIIcon* pUIIcon)
{
	POINT ptCur = CGameProcedure::s_pLocalInput->MouseGetPos();
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		int iOrder = m_iUpgradeScrollSlotInvPos[i];
		if (iOrder != -1 && m_pSlotArea[i]->IsIn(ptCur.x, ptCur.y))
		{
			if (m_pMyUpgradeInv[iOrder] != nullptr
				&& m_pMyUpgradeInv[iOrder]->pUIIcon != nullptr)
			{
				m_sSelectedIconInfo.iSourceOrder = -1; // Can not move
				m_sSelectedIconInfo.pSelectedItem = m_pMyUpgradeInv[iOrder];
				return;
			}
		}
	}

	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		if (m_pMyUpgradeInv[i] != nullptr && m_pMyUpgradeInv[i]->pUIIcon != nullptr
			&& m_pMyUpgradeInv[i]->pUIIcon == pUIIcon)
		{
			m_sSelectedIconInfo.iSourceOrder = i;
			m_sSelectedIconInfo.pSelectedItem = m_pMyUpgradeInv[i];
			return;
		}	
	}
}

void CUIItemUpgrade::Open()
{
	SetVisibleWithNoSound(true, false, false);
	GoldUpdate();
}

void CUIItemUpgrade::SetNpcID(int iNpcID)
{
	m_iNpcID = iNpcID;
}

void CUIItemUpgrade::GoldUpdate()
{
	if (m_pStrMyGold != nullptr)
	{
		m_pStrMyGold->SetString(CGameBase::FormatNumber(CGameBase::s_pPlayer->m_InfoExt.iGold));
	}
}

void CUIItemUpgrade::GetItemFromInv()
{
	CUIInventory* pInven = CGameProcedure::s_pProcMain->m_pUIInventory;
	if (pInven == nullptr)
		return;

	m_sSelectedIconInfo.pSelectedItem = nullptr;
	m_sSelectedIconInfo.iSourceOrder = -1;
	m_iUpgradeItemSlotInvPos = -1;
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		m_iUpgradeScrollSlotInvPos[i] = -1;
	}

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

		__IconItemSkill* spItem = pInven->m_pMyInvWnd[i];
		if (spItem != nullptr)
		{
			spItem = new __IconItemSkill(*pInven->m_pMyInvWnd[i]);
			CreateUIIconForItem(spItem);
			SetupIconArea(spItem, m_pInvArea[i]);
			ShowItemCount(spItem, i);
			m_pMyUpgradeInv[i] = spItem;
		}
	}
}

void CUIItemUpgrade::Close()
{
	bool bwork = IsVisible();
	SetVisibleWithNoSound(false, bwork, false);
}

bool CUIItemUpgrade::ReceiveIconDrop(__IconItemSkill* spItem)
{
	if (spItem == nullptr)
		return false;

	POINT ptCur = CGameProcedure::s_pLocalInput->MouseGetPos();
	if (IsAllowedUpgradeItem(spItem))
	{
		if (m_iUpgradeItemSlotInvPos == -1 && m_pAreaUpgrade->IsIn(ptCur.x, ptCur.y))
		{
			if (m_iUpgradeItemSlotInvPos != -1)
			{
				SetupIconArea(m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos], m_pInvArea[m_iUpgradeItemSlotInvPos]);
			}
			SetupIconArea(spItem, m_pAreaUpgrade);
			m_iUpgradeItemSlotInvPos = m_sSelectedIconInfo.iSourceOrder;
			return true;
		}
	}
	else if (IsMaterialSlotCompatible(spItem))
	{
		for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; ++i)
		{
			CN3UIArea* pArea = m_pSlotArea[i];
			if (pArea != nullptr && pArea->IsIn(ptCur.x, ptCur.y))
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
	if (spItem != nullptr)
	{
		int iOrder = m_sSelectedIconInfo.iSourceOrder;
		if (iOrder != -1)
		{
			SetupIconArea(spItem, m_pInvArea[iOrder]);
			ShowItemCount(spItem, iOrder);
			m_sSelectedIconInfo.pSelectedItem = nullptr;
			m_sSelectedIconInfo.iSourceOrder = -1;
		}
	}
}

uint32_t CUIItemUpgrade::MouseProc(uint32_t dwFlags, const POINT& ptCur, const POINT& ptOld)
{
	uint32_t dwRet = UI_MOUSEPROC_NONE;

	if (!m_bVisible)
		return dwRet;

	if (GetState() == UI_STATE_ICON_MOVING)
	{
		RECT region = GetSampleRect();

		if (m_sSelectedIconInfo.pSelectedItem != nullptr)
		{
			CN3UIIcon* pUIIcon = m_sSelectedIconInfo.pSelectedItem->pUIIcon;
			pUIIcon->SetRegion(region);
			pUIIcon->SetMoveRect(region);
		}
	}

	return CN3UIBase::MouseProc(dwFlags, ptCur, ptOld);
}

// Returns a rectangle centered at the mouse position, used for moving icons.
RECT CUIItemUpgrade::GetSampleRect()
{
	RECT rect;
	CN3UIArea* pArea;
	POINT ptCur = CGameProcedure::s_pLocalInput->MouseGetPos();
	pArea = m_pAreaResult;
	rect = pArea->GetRegion();
	float fWidth = (float) (rect.right - rect.left);
	float fHeight = (float) (rect.bottom - rect.top);
	fWidth *= 0.5f; fHeight *= 0.5f;
	rect.left = ptCur.x - (int) fWidth;  rect.right = ptCur.x + (int) fWidth;
	rect.top = ptCur.y - (int) fHeight; rect.bottom = ptCur.y + (int) fHeight;
	return rect;
}

e_UI_DISTRICT CUIItemUpgrade::GetWndDistrict() const
{
	POINT ptCur = CGameProcedure::s_pLocalInput->MouseGetPos();

	if (m_pAreaUpgrade != nullptr && m_pAreaUpgrade->IsIn(ptCur.x, ptCur.y))
	{
		return UIWND_DISTRICT_UPGRADE_SLOT;
	}

	if (m_pAreaResult != nullptr && m_pAreaResult->IsIn(ptCur.x, ptCur.y))
	{
		return UIWND_DISTRICT_UPGRADE_RESULT_SLOT;
	}

	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; ++i)
	{
		if (m_pSlotArea[i] != nullptr && m_pSlotArea[i]->IsIn(ptCur.x, ptCur.y))
		{
			return UIWND_DISTRICT_UPGRADE_SLOT;
		}
	}

	for (int i = 0; i < MAX_ITEM_INVENTORY; ++i)
	{
		if (m_pInvArea[i] != nullptr && m_pInvArea[i]->IsIn(ptCur.x, ptCur.y))
		{
			return UIWND_DISTRICT_UPGRADE_INV;
		}
	}

	return UIWND_DISTRICT_UPGRADE_UNKNOWN; // Other icons can not move
}

// Handles UI messages such as button clicks and icon events.
bool CUIItemUpgrade::ReceiveMessage(CN3UIBase* pSender, uint32_t dwMsg)
{
	if (pSender == nullptr)
		return false;

	if (dwMsg == UIMSG_BUTTON_CLICK)
	{
		if (pSender == m_pBtnClose)
			Close();
		else if (pSender == m_pBtnCancel && !m_bUpgradeInProgress)
			ResetUpgradeInventory();
		else if (pSender == m_pBtnOk && !m_bUpgradeInProgress)
		{
			SendToServerUpgradeMsg();
		}
		return true;
	}
	__IconItemSkill* spItem = nullptr;
	e_UI_DISTRICT  eUIWnd = UIWND_DISTRICT_UPGRADE_UNKNOWN;

	int iOrder = -1;
	RECT region = GetSampleRect();

	switch (dwMsg)
	{
		case UIMSG_ICON_DOWN_FIRST:
			spItem = m_sSelectedIconInfo.pSelectedItem;
			iOrder = m_sSelectedIconInfo.iSourceOrder;
			eUIWnd = GetWndDistrict();

			if (eUIWnd == UIWND_DISTRICT_UPGRADE_RESULT_SLOT)
				ResetUpgradeInventory();
			if (iOrder == -1 || (eUIWnd != UIWND_DISTRICT_UPGRADE_INV))
			{
				SetState(UI_STATE_COMMON_NONE);
				return false;
			}

			// Divide countable items
			if (spItem->iCount > 1 && (spItem->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE
				|| spItem->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE_SMALL))
			{
				__IconItemSkill* pNew = new __IconItemSkill(*spItem);
				CreateUIIconForItem(pNew);
				ShowItemCount(spItem, iOrder);
				m_sSelectedIconInfo.pSelectedItem = pNew;
			}

			// Set icon region for moving.
			m_sSelectedIconInfo.pSelectedItem->pUIIcon->SetRegion(region);
			m_sSelectedIconInfo.pSelectedItem->pUIIcon->SetMoveRect(region);
			break;

		case UIMSG_ICON_UP:
			spItem = m_sSelectedIconInfo.pSelectedItem;
			iOrder = m_sSelectedIconInfo.iSourceOrder;
			if (spItem == nullptr)
				break;
			if (!ReceiveIconDrop(spItem))
			{
				if (spItem->iCount > 1 && (spItem->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE
					|| spItem->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE_SMALL))
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
			spItem = m_sSelectedIconInfo.pSelectedItem;
			if (GetState() == UI_STATE_ICON_MOVING && spItem != nullptr)
			{
				spItem->pUIIcon->SetRegion(region);
				spItem->pUIIcon->SetMoveRect(region);
			}
			break;

		case UIMSG_ICON_RDOWN_FIRST:
			if (!m_bUpgradeInProgress)
			{
				HandleInventoryIconRightClick(m_sSelectedIconInfo.pSelectedItem);
			}
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
	}

	if (bWork && !bVisible)
	{
		if (m_pUITooltipDlg != nullptr) m_pUITooltipDlg->DisplayTooltipsDisable();
		if (GetState() == UI_STATE_ICON_MOVING)
			CancelIconDrop(m_sSelectedIconInfo.pSelectedItem);

		// Move the items inventory area.
		ResetUpgradeInventory();
		AnimClose();
	}
}

// Loads the UI from file and initializes all required UI components.
bool CUIItemUpgrade::Load(HANDLE hFile)
{
	if (!CN3UIBase::Load(hFile))
		return false;

	N3_VERIFY_UI_COMPONENT(m_pBtnClose, (CN3UIButton*) GetChildByID("btn_close"));
	N3_VERIFY_UI_COMPONENT(m_pBtnOk, (CN3UIButton*) GetChildByID("btn_ok"));
	N3_VERIFY_UI_COMPONENT(m_pBtnCancel, (CN3UIButton*) GetChildByID("btn_cancel"));
	N3_VERIFY_UI_COMPONENT(m_pBtnConversation, (CN3UIButton*) GetChildByID("btn_conversation"));
	N3_VERIFY_UI_COMPONENT(m_pAreaUpgrade, (CN3UIArea*) GetChildByID("a_upgrade"));
	N3_VERIFY_UI_COMPONENT(m_pAreaResult, (CN3UIArea*) GetChildByID("a_result"));
	N3_VERIFY_UI_COMPONENT(m_pImageCover1, (CN3UIImage*) GetChildByID("img_cover_01"));
	N3_VERIFY_UI_COMPONENT(m_pImageCover2, (CN3UIImage*) GetChildByID("img_cover_02"));
	N3_VERIFY_UI_COMPONENT(m_pStrMyGold, (CN3UIString*) GetChildByID("text_gold"));

	if (m_pStrMyGold != nullptr)
		m_pStrMyGold->SetString("0");

	if (m_pImageCover1 != nullptr && m_pImageCover2 != nullptr)
	{
		m_pImageCover1->SetVisible(false);
		m_pImageCover2->SetVisible(false);
	}

	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; ++i)
	{
		std::string szID = fmt::format("a_m_{}", i);
		N3_VERIFY_UI_COMPONENT(m_pSlotArea[i], (CN3UIArea*) GetChildByID(szID));
	}

	for (int i = 0; i < MAX_ITEM_INVENTORY; ++i)
	{
		std::string szID;
		szID = fmt::format("a_slot_{}", i);
		N3_VERIFY_UI_COMPONENT(m_pInvArea[i], (CN3UIArea*) GetChildByID(szID));
		szID = fmt::format("s_count_{}", i);
		N3_VERIFY_UI_COMPONENT(m_pInvString[i], (CN3UIString*) GetChildByID(szID));
	}

	for (int i = 0; i < FLIPFLOP_MAX_FRAMES; ++i)
	{
		std::string szID;
		szID = fmt::format("img_s_load_{}", i);
		N3_VERIFY_UI_COMPONENT(m_pImgSuccess[i], (CN3UIImage*) GetChildByID(szID));
		szID = fmt::format("img_f_load_{}", i);
		N3_VERIFY_UI_COMPONENT(m_pImgFail[i], (CN3UIImage*) GetChildByID(szID));
		m_pImgFail[i]->SetVisible(false);
		m_pImgSuccess[i]->SetVisible(false);
	}
	InitIconWnd();

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
		if (iOrder != -1)
		{
			__IconItemSkill* spItem = m_pMyUpgradeInv[iOrder];
			if (spItem != nullptr)
			{
				SetupIconArea(spItem, m_pInvArea[iOrder]);
				if (spItem->iCount > 1 && (spItem->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE
					|| spItem->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE_SMALL))
				{
					CleanAreaSlot(m_pSlotArea[i]);
					++spItem->iCount;
				}
				ShowItemCount(spItem, iOrder);
			}
		}
		m_iUpgradeScrollSlotInvPos[i] = -1;
	}
	m_bUpgradeSucceeded = false;
	m_bUpgradeInProgress = false;
}

bool CUIItemUpgrade::IsTrina(uint32_t dwID) const
{
	static const std::unordered_set<int> upgradeItemIDs = {
	// Trina
	379256000, 379257000, 379258000, 700002000
	};
	return upgradeItemIDs.contains(dwID);
}

// Checks if the given item is allowed to be upgraded (unique or upgrade type).
bool CUIItemUpgrade::IsAllowedUpgradeItem(__IconItemSkill* spItem) const
{
	if (m_iUpgradeItemSlotInvPos != -1)
		return false;
	if (spItem != nullptr && spItem->pItemBasic != nullptr)
	{
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
	return false;
}

void CUIItemUpgrade::SendToServerUpgradeMsg()
{
	if (m_iUpgradeItemSlotInvPos == -1)
		return;
	m_bUpgradeInProgress = true;
	uint8_t byBuff[512];
	int iOffset = 0;
	int itotalSent = 0;

	CAPISocket::MP_AddByte(byBuff, iOffset, WIZ_ITEM_UPGRADE);
	CAPISocket::MP_AddByte(byBuff, iOffset, ITEM_UPGRADE_PROCESS);
	CAPISocket::MP_AddWord(byBuff, iOffset, m_iNpcID);

	int32_t nItemID = 0;
	int8_t bPos = -1;

	// Add Upgrade Item
	if (m_iUpgradeItemSlotInvPos != -1)
	{
		nItemID = m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos]->pItemBasic->dwID + m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos]->pItemExt->dwID;
		bPos = m_iUpgradeItemSlotInvPos;

		CAPISocket::MP_AddDword(byBuff, iOffset, nItemID);
		CAPISocket::MP_AddByte(byBuff, iOffset, bPos);
		++itotalSent;
	}

	// Add Upgrade Slots
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; ++i)
	{
		int iOrder = m_iUpgradeScrollSlotInvPos[i];
		if (iOrder != -1)
		{
			nItemID = m_pMyUpgradeInv[iOrder]->pItemBasic->dwID +
				m_pMyUpgradeInv[iOrder]->pItemExt->dwID;
			bPos = iOrder;
			CAPISocket::MP_AddDword(byBuff, iOffset, nItemID);
			CAPISocket::MP_AddByte(byBuff, iOffset, bPos);
			++itotalSent;
		}
	}

	nItemID = 0;
	bPos = -1;
	while (itotalSent < MAX_UPGRADE_MATERIAL)
	{
		CAPISocket::MP_AddDword(byBuff, iOffset, nItemID);
		CAPISocket::MP_AddByte(byBuff, iOffset, bPos);
		++itotalSent;
	}

	CGameProcedure::s_pSocket->Send(byBuff, iOffset);
}

void CUIItemUpgrade::MsgRecv_ItemUpgrade(Packet& pkt)
{
	CUIInventory* pInven = CGameProcedure::s_pProcMain->m_pUIInventory;
	if (pInven == nullptr)
		return;
	enum UpgradeErrorCodes
	{
		UpgradeFailed = 0,
		UpgradeSucceeded = 1,
		UpgradeTrading = 2,
		UpgradeNeedCoins = 3,
		UpgradeNoMatch = 4
	};

	int8_t result = pkt.read<uint8_t>();
	uint32_t nItemID[10] = { 0 };
	int8_t bPos[10] = { -1 };
	for (int i = 0; i < MAX_UPGRADE_MATERIAL; i++)
	{
		nItemID[i] = pkt.read<uint32_t>();
		bPos[i] = pkt.read<uint8_t>();
		if (bPos[i] < 0 || bPos[i] > MAX_ITEM_INVENTORY)
			bPos[i] = -1;
	}
	if (bPos[0] != -1)
		m_iUpgradeItemSlotInvPos = bPos[0];

	std::string szMsg;

	if (result == UpgradeFailed)
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
		m_eAnimationState = ANIM_START;
		szMsg = fmt::format_text_resource(IDS_ITEM_UPGRADE_FAIL);
		CGameProcedure::s_pProcMain->MsgOutput(szMsg, D3DCOLOR_XRGB(255, 0, 255));
	}
	else if (result == UpgradeSucceeded)
	{
		__TABLE_ITEM_EXT* itemExt = nullptr;
		__TABLE_ITEM_BASIC* itemBasic = nullptr;
		e_PartPosition ePart;
		e_PlugPosition ePlug;
		std::string szIconFN;
		float fUVAspect = (float) 45.0f / (float) 64.0f;

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
			m_eAnimationState = ANIM_START;
		szMsg = fmt::format_text_resource(IDS_ITEM_UPGRADE_SUCCESS);
		CGameProcedure::s_pProcMain->MsgOutput(szMsg, D3DCOLOR_XRGB(128, 128, 255));

		itemBasic = CGameBase::s_pTbl_Items_Basic.Find(nItemID[0] / 1000 * 1000);
		itemExt = nullptr;

		if (itemBasic && itemBasic->byExtIndex >= 0 && itemBasic->byExtIndex < MAX_ITEM_EXTENSION)
			itemExt = CGameBase::s_pTbl_Items_Exts[itemBasic->byExtIndex].Find(nItemID[0] % 1000);

		if (ITEM_TYPE_UNKNOWN == CGameBase::MakeResrcFileNameForUPC(itemBasic, itemExt, nullptr, &szIconFN, ePart, ePlug, CGameBase::s_pPlayer->m_InfoBase.eRace))
			return;
		__IconItemSkill* spItemNew;
		spItemNew = new __IconItemSkill;
		spItemNew->pItemBasic = itemBasic;
		spItemNew->pItemExt = itemExt;
		spItemNew->szIconFN = szIconFN;
		spItemNew->iCount = 1;
		CreateUIIconForItem(spItemNew);
		m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos] = spItemNew;

		//Upgrade Item in Inventory
		pInven->m_pMyInvWnd[m_iUpgradeItemSlotInvPos]->pItemBasic = spItemNew->pItemBasic;
		pInven->m_pMyInvWnd[m_iUpgradeItemSlotInvPos]->pItemExt = spItemNew->pItemExt;
		pInven->m_pMyInvWnd[m_iUpgradeItemSlotInvPos]->iDurability = spItemNew->iDurability;

	}
	else if (result == UpgradeTrading)
	{
		m_bUpgradeInProgress = false;
		ResetUpgradeInventory();
		szMsg = fmt::format_text_resource(IDS_ITEM_UPGRADE_CANNOT_PERFORM);
		CGameProcedure::s_pProcMain->MsgOutput(szMsg, D3DCOLOR_XRGB(255, 0, 255));
	}
	else if (result == UpgradeNeedCoins)
	{
		m_bUpgradeInProgress = false;
		ResetUpgradeInventory();
		szMsg = fmt::format_text_resource(IDS_ITEM_UPGRADE_NEED_COIN);
		CGameProcedure::s_pProcMain->MsgOutput(szMsg, D3DCOLOR_XRGB(255, 0, 255));
	}
	else if (result == UpgradeNoMatch)
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
	m_fAnimationTimer += CN3Base::s_fSecPerFrm;
	constexpr float COVER_ANIMATION_DURATION = 0.8f;
	float t = m_fAnimationTimer / COVER_ANIMATION_DURATION;

	// Only handle opening (covers move outward and hide)
	float ease = 1.0f - ((1.0f - t) * (1.0f - t));
	int m_iCoverShift = m_rcCover1Original.bottom - m_rcCover1Original.top;
	// Calculate new Y positions for opening
	int y1 = m_rcCover1Original.top + (int) ((m_rcCover1Original.top - m_iCoverShift - m_rcCover1Original.top) * ease);
	int y2 = m_rcCover2Original.top + (int) ((m_rcCover2Original.top + m_iCoverShift - m_rcCover2Original.top) * ease);

	// Animation completed - hide covers and reset positions
	if (t >= 1.0f)
	{
		m_eAnimationState = ANIM_NONE;
		if (m_pImageCover1 != nullptr && m_pImageCover1 != nullptr)
		{
			m_pImageCover1->SetVisible(false);
			m_pImageCover2->SetVisible(false);
			m_pImageCover1->SetRegion(m_rcCover1Original);
			m_pImageCover2->SetRegion(m_rcCover2Original);
		}
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

	if (m_pImageCover1 != nullptr && m_pImageCover1 != nullptr)
	{
		m_pImageCover1->SetRegion(rc1);
		m_pImageCover2->SetRegion(rc2);
	}
}

void CUIItemUpgrade::UpdateFlipFlopAnimation()
{
	m_fAnimationTimer += CN3Base::s_fSecPerFrm;
	constexpr float FLIPFLOP_FRAME_DELAY = 0.1f;

	if (m_fAnimationTimer >= FLIPFLOP_FRAME_DELAY)
	{
		m_fAnimationTimer -= FLIPFLOP_FRAME_DELAY;
		m_iCurrentFrame++;

		if (m_iCurrentFrame >= FLIPFLOP_MAX_FRAMES)
		{
			HideAllAnimationFrames();

			m_eAnimationState = ANIM_RESULT;
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
	// Hide all img_s_load_X frames
	for (int i = 0; i < FLIPFLOP_MAX_FRAMES; ++i)
	{
		if (m_pImgSuccess[i] != nullptr)
			m_pImgSuccess[i]->SetVisible(false);
	}
	// Hide all img_f_load_X frames
	for (int i = 0; i < FLIPFLOP_MAX_FRAMES; ++i)
	{
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
	std::string iconFile = spItem->szIconFN;
	spItem->pUIIcon->SetTex(iconFile);
	spItem->pUIIcon->SetUVRect(0, 0, UV_ASPECT_RATIO, UV_ASPECT_RATIO);
	spItem->pUIIcon->SetUIType(UI_TYPE_ICON);
	spItem->pUIIcon->SetStyle(UISTYLE_ICON_ITEM | UISTYLE_ICON_CERTIFICATION_NEED);
	spItem->pUIIcon->SetVisible(true);
}

void CUIItemUpgrade::SetupIconArea(__IconItemSkill* spItem, CN3UIArea* pArea)
{
	if (spItem == nullptr || spItem->pUIIcon == nullptr || pArea == nullptr)
		return;

	spItem->pUIIcon->SetRegion(pArea->GetRegion());
	spItem->pUIIcon->SetMoveRect(pArea->GetRegion());
}

bool CUIItemUpgrade::IsMaterialSlotCompatible(__IconItemSkill* pSrc) const
{
	if (pSrc->pItemBasic->dwEffectID2 != UpgradeMaterial && !m_bUpgradeInProgress)
		return false;

	bool bhasTrina = false;
	bool bhasScroll = false;

	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; ++i)
	{
		int iOrder = m_iUpgradeScrollSlotInvPos[i];
		if (iOrder != -1)
		{
			if (IsTrina(m_pMyUpgradeInv[iOrder]->pItemBasic->dwID))
			{
				bhasTrina = true;
			}
			else
				bhasScroll = true;
		}
	}
	if (bhasTrina && IsTrina(pSrc->pItemBasic->dwID))
		return false;
	if (bhasScroll && !IsTrina(pSrc->pItemBasic->dwID))
		return false;
	if (bhasTrina && bhasScroll)
		return false;
	return true;
}

void CUIItemUpgrade::FlipFlopAnim()
{
	if (m_eAnimationState != ANIM_FLIPFLOP)
		return;
	std::string szID;

	// Hide before frame
	if (m_iCurrentFrame > 0)
	{
		if (m_bUpgradeSucceeded)
			m_pImgSuccess[m_iCurrentFrame - 1]->SetVisible(false);
		else
			m_pImgFail[m_iCurrentFrame - 1]->SetVisible(false);
	}

	// Show current frame
	if (m_bUpgradeSucceeded)
	{
		m_pImgSuccess[m_iCurrentFrame]->SetVisible(true);
		m_pImgSuccess[m_iCurrentFrame]->SetParent(this);
	}
	else
	{
		m_pImgFail[m_iCurrentFrame]->SetVisible(true);
		m_pImgFail[m_iCurrentFrame]->SetParent(this);
	}
}
void CUIItemUpgrade::AnimClose()
{
	if (m_eAnimationState != ANIM_NONE)
	{
		m_fAnimationTimer = 0.0f;
		m_iCurrentFrame = 0;
		if (m_pImageCover1 != nullptr && m_pImageCover2 != nullptr)
		{
			m_pImageCover1->SetVisible(false);
			m_pImageCover2->SetVisible(false);
		}
	}
	m_eAnimationState = ANIM_NONE;
	m_bUpgradeInProgress = false;
	HideAllAnimationFrames();
}

void CUIItemUpgrade::StartUpgradeAnim()
{
	if (!m_bUpgradeInProgress)
		return;
	m_fAnimationTimer = 0.0f;
	m_iCurrentFrame = 0;

	// Make covers visible during animation
	m_pImageCover1->SetVisible(true);
	m_pImageCover2->SetVisible(true);
	m_pImageCover1->SetParent(this);
	m_pImageCover2->SetParent(this);

	// save original positions
	m_rcCover1Original = m_pImageCover1->GetRegion();
	m_rcCover2Original = m_pImageCover2->GetRegion();
	m_eAnimationState = ANIM_FLIPFLOP;
}

bool CUIItemUpgrade::HandleInventoryIconRightClick(__IconItemSkill* spItem)
{
	// Move upgrade result to inv
	if (m_bUpgradeSucceeded)
	{
		ResetUpgradeInventory();
	}

	if (spItem == nullptr || spItem->pUIIcon == nullptr)
		return false;

	POINT ptCur = CGameProcedure::s_pLocalInput->MouseGetPos();
	if (spItem->pUIIcon->IsVisible() && spItem->pUIIcon->IsIn(ptCur.x, ptCur.y))
	{
		if (IsAllowedUpgradeItem(spItem))
		{
			SetupIconArea(spItem, m_pAreaUpgrade);
			m_iUpgradeItemSlotInvPos = m_sSelectedIconInfo.iSourceOrder;

			return true;
		}
		else if (IsMaterialSlotCompatible(spItem))
		{
			// Divide countable items
			if (spItem->iCount > 1 && (spItem->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE
				|| spItem->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE_SMALL))
			{
				__IconItemSkill* pNew = new __IconItemSkill(*spItem);
				CreateUIIconForItem(pNew);
				spItem = pNew;
			}

			for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; ++i)
			{
				if (m_iUpgradeScrollSlotInvPos[i] == -1)
				{
					if (MaterialSlotDrop(spItem, i));
					return true;
				}
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
	if (spItem->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE
		|| spItem->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE_SMALL)
	{
		CN3UIString* pStr = m_pInvString[iOrder];
		if (pStr != nullptr)
		{
			if (spItem->iCount > 1)
			{
				pStr->SetStringAsInt(spItem->iCount);
				if (GetState() == UI_STATE_ICON_MOVING && m_sSelectedIconInfo.pSelectedItem != nullptr)
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
	int iSourceOrder = m_sSelectedIconInfo.iSourceOrder;

	// If countable reduce inv item count
	if (spItem->iCount > 1 && (spItem->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE
		|| spItem->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE_SMALL))
	{
		--m_pMyUpgradeInv[iSourceOrder]->iCount;
	}

	SetupIconArea(spItem, pArea);
	ShowItemCount(m_pMyUpgradeInv[iSourceOrder], iSourceOrder); // Update inv item count
	m_iUpgradeScrollSlotInvPos[iOrder] = iSourceOrder;

	return true;
}

void CUIItemUpgrade::CleanAreaSlot(CN3UIArea* pArea)
{
	for (UIListReverseItor itor = m_Children.rbegin(); m_Children.rend() != itor; ++itor)
	{
		CN3UIBase* pChild = (*itor);
		int x = pChild->GetRegion().left / 2 + pChild->GetRegion().right / 2;
		int y = pChild->GetRegion().top / 2 + pChild->GetRegion().bottom / 2;
		if (pChild->UIType() == UI_TYPE_ICON && pArea->IsIn(x, y))
		{
			delete pChild;
		}
	}
}
