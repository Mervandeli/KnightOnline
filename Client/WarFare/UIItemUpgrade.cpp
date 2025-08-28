// UIItemUpgrade.cpp: implementation of the CUIItemUpgrade class.
//Author : Monzantys(Mervan)
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LocalInput.h"
#include "APISocket.h"
#include "GameProcMain.h"
#include "UIItemUpgrade.h"
#include "UIImageTooltipDlg.h"
#include "UIInventory.h"
#include "UIManager.h"
#include "PlayerMySelf.h"
#include "text_resources.h"
#include "UIHotKeyDlg.h"
#include "UISkillTreeDlg.h"
#include "resource.h"
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
		m_pUpgradeScrollSlots[i] = nullptr;
		m_iUpgradeScrollSlotInvPos[i] = -1;
	}

	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		m_pMyUpgradeInv[i] = nullptr;
		m_pBackupUpgradeInv[i] = nullptr;
	}

	m_pUpgradeResultSlot = nullptr;
	m_pUpgradeItemSlot = nullptr;
	m_pUITooltipDlg = nullptr;
	m_pStrMyGold = nullptr;

	m_eAnimationState = ANIM_NONE;
	m_fAnimationTimer = 0.0f;
	m_iCurrentFrame = 0;
	m_bUpgradeSucceeded = false;
	m_bUpgradeInProgress = false;
	m_iNpcID = 0;
}

CUIItemUpgrade::~CUIItemUpgrade()
{
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		if (m_pUpgradeScrollSlots[i] != nullptr)
		{
			if (m_pUpgradeScrollSlots[i]->pUIIcon != nullptr)
			{
				delete m_pUpgradeScrollSlots[i]->pUIIcon;
				m_pUpgradeScrollSlots[i]->pUIIcon = nullptr;
			}
			delete m_pUpgradeScrollSlots[i];
			m_pUpgradeScrollSlots[i] = nullptr;
		}
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

		if (m_pBackupUpgradeInv[i] != nullptr)
		{
			if (m_pBackupUpgradeInv[i]->pUIIcon != nullptr)
			{
				m_pBackupUpgradeInv[i]->pUIIcon = nullptr;
			}
			m_pBackupUpgradeInv[i] = nullptr;
		}

	}

	if (m_pUpgradeItemSlot != nullptr)
	{
		if (m_pUpgradeItemSlot->pUIIcon != nullptr)
		{
			delete m_pUpgradeItemSlot->pUIIcon;
			m_pUpgradeItemSlot->pUIIcon = nullptr;
		}
		delete m_pUpgradeItemSlot;
		m_pUpgradeItemSlot = nullptr;
	}

	if (m_pUpgradeResultSlot != nullptr)
	{
		if (m_pUpgradeResultSlot->pUIIcon != nullptr)
		{
			delete m_pUpgradeResultSlot->pUIIcon;
			m_pUpgradeResultSlot->pUIIcon = nullptr;
		}
		delete m_pUpgradeResultSlot;
		m_pUpgradeResultSlot = nullptr;
	}

	m_iUpgradeItemSlotInvPos = -1;

	m_pStrMyGold = nullptr;

	CN3UIWndBase::Release();
}

void CUIItemUpgrade::Release()
{

	CN3UIWndBase::Release();
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
				ShowResultUpgrade();
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
		if (GetState() == UI_STATE_ICON_MOVING && pChild->UIType() == UI_TYPE_ICON && CN3UIWndBase::m_sSelectedIconInfo.pItemSelect &&
			(CN3UIIcon*) pChild == CN3UIWndBase::m_sSelectedIconInfo.pItemSelect->pUIIcon)
			continue;
		pChild->Render();

		if (GetState() == UI_STATE_COMMON_NONE &&
			pChild->UIType() == UI_TYPE_ICON && pChild->GetStyle() & UISTYLE_ICON_HIGHLIGHT)
		{
			bTooltipRender = true;
			spItem = GetHighlightIconItem((CN3UIIcon*) pChild);
		}
	}

	if (GetState() == UI_STATE_ICON_MOVING && CN3UIWndBase::m_sSelectedIconInfo.pItemSelect)
		CN3UIWndBase::m_sSelectedIconInfo.pItemSelect->pUIIcon->Render();

	if (spItem != nullptr)
	{
		m_pUITooltipDlg->DisplayTooltipsEnable(ptCur.x, ptCur.y, spItem, false, false);
	}
}

void CUIItemUpgrade::InitIconWnd(e_UIWND eWnd)
{
	__TABLE_UI_RESRC* pTbl = CGameBase::s_pTbl_UI.Find(CGameBase::s_pPlayer->m_InfoBase.eNation);

	m_pUITooltipDlg = new CUIImageTooltipDlg();
	m_pUITooltipDlg->Init(this);
	m_pUITooltipDlg->LoadFromFile(pTbl->szItemInfo);
	m_pUITooltipDlg->InitPos();
	m_pUITooltipDlg->SetVisible(false);

	CN3UIWndBase::InitIconWnd(eWnd);

	N3_VERIFY_UI_COMPONENT(m_pStrMyGold, (CN3UIString*) GetChildByID("text_gold"));
	if (m_pStrMyGold != nullptr) 
		m_pStrMyGold->SetString("0");
}

__IconItemSkill* CUIItemUpgrade::GetHighlightIconItem(CN3UIIcon* pUIIcon)
{
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		if (m_pUpgradeScrollSlots[i] != nullptr && m_pUpgradeScrollSlots[i]->pUIIcon == pUIIcon)
			return m_pUpgradeScrollSlots[i];
	}

	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		if (m_pMyUpgradeInv[i] != nullptr && m_pMyUpgradeInv[i]->pUIIcon == pUIIcon)
			return m_pMyUpgradeInv[i];
	}

	if (m_pUpgradeItemSlot != nullptr && m_pUpgradeItemSlot->pUIIcon == pUIIcon)
		return m_pUpgradeItemSlot;

	return nullptr;
}

void CUIItemUpgrade::Open()
{
	SetVisibleWithNoSound(true,false,false);
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

void CUIItemUpgrade::ItemMoveFromInvToThis()
{
	CUIInventory* pInven = CGameProcedure::s_pProcMain->m_pUIInventory;
	if (pInven == nullptr)
		return;

	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		if (pInven->m_pMyInvWnd[i] != nullptr)
		{
			__IconItemSkill* spItem = pInven->m_pMyInvWnd[i];
			spItem->pUIIcon->SetParent(this);
			pInven->m_pMyInvWnd[i] = nullptr;

			SetupIconArea(spItem, m_pInvArea[i]);
			ShowItemCount(spItem,i);

			m_pMyUpgradeInv[i] = spItem;
			// Backup the inventory state for restoration if needed.
			m_pBackupUpgradeInv[i] = new __IconItemSkill(*m_pMyUpgradeInv[i]);
		}
		else
		{
			m_pMyUpgradeInv[i] = nullptr;
			m_pBackupUpgradeInv[i] = nullptr;
		}
	}
}

void CUIItemUpgrade::Close()
{
	bool bwork = IsVisible();
	SetVisibleWithNoSound(false, bwork,false);

	if (GetState() == UI_STATE_ICON_MOVING)
		IconRestore();
	SetState(UI_STATE_COMMON_NONE);
	CN3UIWndBase::AllHighLightIconFree();
}

void CUIItemUpgrade::ItemMoveFromThisToInv()
{
	CUIInventory* pInven = CGameProcedure::s_pProcMain->m_pUIInventory;
	if (pInven == nullptr)
		return;

	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		__IconItemSkill* spItem = m_pMyUpgradeInv[i];
		if (spItem != nullptr)
		{
			spItem->pUIIcon->SetParent(pInven);
			CN3UIArea* pArea = pInven->GetChildAreaByiOrder(UI_AREA_TYPE_INV, i);
			SetupIconArea(spItem, pArea);
			pInven->m_pMyInvWnd[i] = spItem;
			m_pMyUpgradeInv[i] = nullptr;
		}
	}
}

bool CUIItemUpgrade::ReceiveIconDrop(__IconItemSkill* spItem, POINT ptCur)
{
	CN3UIArea* pArea;
	e_UIWND_DISTRICT eUIWnd = UIWND_DISTRICT_UNKNOWN;
	if (!m_bVisible)
		return false;

	//  Check if the selected window is correct and the drop is valid.
	if (CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.UIWnd != m_eUIWnd)
		return false;
	if (CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.UIWndDistrict != UIWND_DISTRICT_UPGRADE_SLOT &&
		CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.UIWndDistrict != UIWND_DISTRICT_UPGRADE_INV)
		return false;

	// Find which slot or area the item is being dropped onto.
	int iDestiOrder = -1;
	bool bFound = false;
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		pArea = m_pSlotArea[i];
		if (pArea != nullptr && pArea->IsIn(ptCur.x, ptCur.y))
		{
			bFound = true;
			eUIWnd = UIWND_DISTRICT_UPGRADE_SLOT;
			iDestiOrder = i;
			break;
		}
	}

	CN3UIWndBase::m_sSelectedIconInfo.pItemSelect = spItem;

	CN3UIWndBase::m_sRecoveryJobInfo.UIWndSourceEnd.UIWndDistrict = eUIWnd;

	switch (CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.UIWndDistrict)
	{
		case UIWND_DISTRICT_UPGRADE_INV:
			if (eUIWnd == UIWND_DISTRICT_UPGRADE_SLOT)
			{
				if (iDestiOrder != -1 && m_pUpgradeScrollSlots[iDestiOrder] == nullptr)
				{
					int iSourceOrder = GetItemiOrder(spItem, UIWND_DISTRICT_UPGRADE_INV);
					if (iSourceOrder != -1)
					{
						__IconItemSkill* pSrc = m_pMyUpgradeInv[iSourceOrder];
						if (!HandleSlotDrop(pSrc, iDestiOrder))
							return false;
					}
				}
				return false;
			}
			else if (m_pAreaUpgrade != nullptr && m_pAreaUpgrade->IsIn(ptCur.x, ptCur.y))
			{
				// Handle dropping item into the main upgrade area (a_upgrade)	
				if (HandleUpgradeAreaDrop(spItem))
				{
					CN3UIWndBase::AllHighLightIconFree();
					SetState(UI_STATE_COMMON_NONE);
					return true;
				}
			}
			break;
	}
	return false;
}

// Restores the icon's position to its original inventory area.
void CUIItemUpgrade::IconRestore()
{
	if (CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.UIWndDistrict == UIWND_DISTRICT_UPGRADE_INV)
	{
		int iOrder = CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.iOrder;
		CN3UIArea* pArea = m_pInvArea[iOrder];
		SetupIconArea(m_pMyUpgradeInv[iOrder],pArea);
		ShowItemCount(m_pMyUpgradeInv[iOrder], iOrder);
	}	
}

uint32_t CUIItemUpgrade::MouseProc(uint32_t dwFlags, const POINT& ptCur, const POINT& ptOld)
{
	uint32_t dwRet = UI_MOUSEPROC_NONE;
	
	if (!m_bVisible)
		return dwRet;
	if (dwFlags==MOUSE_RBCLICKED && !m_bUpgradeInProgress)
	{
		HandleInventoryIconRightClick(ptCur);
	}
	if (GetState() == UI_STATE_ICON_MOVING && CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.UIWnd == UIWND_UPGRADE)
	{
		RECT region = GetSampleRect();
		CN3UIWndBase::m_sSelectedIconInfo.pItemSelect->pUIIcon->SetRegion(region);
		CN3UIWndBase::m_sSelectedIconInfo.pItemSelect->pUIIcon->SetMoveRect(region);
	}

	return CN3UIWndBase::MouseProc(dwFlags, ptCur, ptOld);
}

// Returns the index of the given item in the specified window district.
int CUIItemUpgrade::GetItemiOrder(__IconItemSkill* spItem, e_UIWND_DISTRICT eWndDist) const
{
	switch (eWndDist)
	{
		case UIWND_DISTRICT_UPGRADE_SLOT:
			for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
			{
				if (m_pUpgradeScrollSlots[i] != nullptr && m_pUpgradeScrollSlots[i] == spItem)
					return i;
			}
			break;

		case UIWND_DISTRICT_UPGRADE_INV:
			for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
			{
				if (m_pMyUpgradeInv[i] != nullptr && m_pMyUpgradeInv[i] == spItem)
					return i;
			}
			break;
	}
	return -1;
}

// Returns a rectangle centered at the mouse position, used for moving icons.
RECT CUIItemUpgrade::GetSampleRect()
{
	RECT rect;
	CN3UIArea* pArea;
	POINT ptCur = CGameProcedure::s_pLocalInput->MouseGetPos();
	pArea = m_pInvArea[0];
	rect = pArea->GetRegion();
	float fWidth = (float) (rect.right - rect.left);
	float fHeight = (float) (rect.bottom - rect.top);
	fWidth *= 0.5f; fHeight *= 0.5f;
	rect.left = ptCur.x - (int) fWidth;  rect.right = ptCur.x + (int) fWidth;
	rect.top = ptCur.y - (int) fHeight; rect.bottom = ptCur.y + (int) fHeight;
	return rect;
}

// Determines which window district (slot or inventory) the given item belongs to.
e_UIWND_DISTRICT CUIItemUpgrade::GetWndDistrict(__IconItemSkill* spItem)
{
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		if (m_pUpgradeScrollSlots[i] != nullptr && m_pUpgradeScrollSlots[i] == spItem)
			return UIWND_DISTRICT_UPGRADE_SLOT;
	}

	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		if (m_pMyUpgradeInv[i] != nullptr && m_pMyUpgradeInv[i] == spItem)
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
			Close();
		else if (pSender == m_pBtnCancel && !m_bUpgradeInProgress)
			RestoreInventoryFromBackup();
		else if (pSender == m_pBtnOk && !m_bUpgradeInProgress)
		{
			SendToServerUpgradeMsg();
		}
		return true;
	}

	__IconItemSkill* spItem = nullptr;
	e_UIWND_DISTRICT eUIWnd;
	int iOrder;
	RECT region = GetSampleRect();
	uint32_t dwBitMask = 0x000f0000;

	switch (dwMsg & dwBitMask)
	{
		case UIMSG_ICON_DOWN_FIRST:
			// Get the item being interacted with.
			spItem = GetHighlightIconItem((CN3UIIcon*) pSender);

			// Save Select Info..
			CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.UIWnd = UIWND_UPGRADE;
			eUIWnd = GetWndDistrict(spItem);
			if (eUIWnd != UIWND_DISTRICT_UPGRADE_INV)
				return false;

			CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.UIWndDistrict = eUIWnd;
			iOrder = GetItemiOrder(spItem, eUIWnd);
			if (iOrder == -1)	
			{
				SetState(UI_STATE_COMMON_NONE);
				return false;
			}
			CN3UIWndBase::m_sSelectedIconInfo.UIWndSelect.iOrder = iOrder;
			CN3UIWndBase::m_sSelectedIconInfo.pItemSelect = spItem;
			// Set icon region for moving.
			pSender->SetRegion(region);
			pSender->SetMoveRect(region);
			// Play item sound.
			if (spItem != nullptr) PlayItemSound(spItem->pItemBasic);

			// Show item count with icount-1 when icon is dragged (if countable)
			if (spItem != nullptr && 
				(spItem->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE ||
				 spItem->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE_SMALL))
			{
				if (spItem->iCount > 0)
				{
					ShowItemCount(spItem, iOrder);
				}
					
			}
			break;

		case UIMSG_ICON_UP:
			POINT ptCur = CGameProcedure::s_pLocalInput->MouseGetPos();
			spItem = CN3UIWndBase::m_sSelectedIconInfo.pItemSelect;
			if (!ReceiveIconDrop(spItem, ptCur))
				IconRestore();// Restore the icon position to its original place if drop failed.
			break;

		case UIMSG_ICON_DOWN:
			spItem = CN3UIWndBase::m_sSelectedIconInfo.pItemSelect;
			if (GetState() == UI_STATE_ICON_MOVING && spItem != nullptr)
			{
				spItem->pUIIcon->SetRegion(region);
				spItem->pUIIcon->SetMoveRect(region);
			}
			break;
	}
	return true;
}

void CUIItemUpgrade::SetVisibleWithNoSound(bool bVisible, bool bWork, bool bReFocus)
{
	CN3UIWndBase::SetVisibleWithNoSound(bVisible, bWork, bReFocus);

	if (bVisible)
	{
		CGameProcedure::s_pUIMgr->SetVisibleFocusedUI(this);
		ItemMoveFromInvToThis();
	}

	if (bWork && !bVisible)
	{

		if (GetState() == UI_STATE_ICON_MOVING)
			IconRestore();
		SetState(UI_STATE_COMMON_NONE);
		CN3UIWndBase::AllHighLightIconFree();

		// Move the items from this window's inventory area to the inventory area of this inventory window.
		RestoreInventoryFromBackup();
		ItemMoveFromThisToInv();
		AnimClose();
		
		if (m_pUITooltipDlg != nullptr) m_pUITooltipDlg->DisplayTooltipsDisable();
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

	if (m_pImageCover1 != nullptr && m_pImageCover2 != nullptr)
	{
		m_pImageCover1->SetVisible(false);
		m_pImageCover2->SetVisible(false);
	}

	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; ++i)
	{
		std::string szID = fmt::format("a_m_{}", i);
		m_pSlotArea[i] = (CN3UIArea*) GetChildByID(szID);
	}

	for (int i = 0; i < MAX_ITEM_INVENTORY; ++i)
	{
		std::string szID = fmt::format("a_slot_{}", i);
		m_pInvArea[i] = (CN3UIArea*) GetChildByID(szID);
	}

	for (int i = 0; i < MAX_ITEM_INVENTORY; ++i)
	{
		std::string szID = fmt::format("s_count_{}", i);
		m_pInvString[i] = (CN3UIString*) GetChildByID(szID);
	}
	
	for (int i = 0; i < FLIPFLOP_MAX_FRAMES; ++i)
	{
		std::string szID;
		szID = fmt::format("img_s_load_{}", i);
		if (CN3UIImage* pImg = (CN3UIImage*) GetChildByID(szID))
			pImg->SetVisible(false);
		szID = fmt::format("img_f_load_{}", i);
		if (CN3UIImage* pImg = (CN3UIImage*) GetChildByID(szID))
			pImg->SetVisible(false);
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

void CUIItemUpgrade::UpdateBackupUpgradeInv()
{
	// Clear existing backup
	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		m_pBackupUpgradeInv[i] = nullptr;
	}
	// Create new backup from current inventory
	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		if (m_pMyUpgradeInv[i] != nullptr)
		{
			m_pBackupUpgradeInv[i] = new __IconItemSkill(*m_pMyUpgradeInv[i]);
		}
	}
}

// Restores the inventory and slots from the backup, recreating icons as needed.
void CUIItemUpgrade::RestoreInventoryFromBackup()
{
	if (m_pUpgradeItemSlot != nullptr)
	{
		if (m_pUpgradeItemSlot->pUIIcon != nullptr)
		{
			delete m_pUpgradeItemSlot->pUIIcon;
			m_pUpgradeItemSlot->pUIIcon = nullptr;
		}
		delete m_pUpgradeItemSlot;
		m_pUpgradeItemSlot = nullptr;
	}

	if (m_pUpgradeResultSlot != nullptr)
	{
		SetupIconArea(m_pUpgradeResultSlot, m_pInvArea[m_iUpgradeItemSlotInvPos]);
		m_pUpgradeResultSlot = nullptr;
	}

	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; i++)
	{
		if (m_pUpgradeScrollSlots[i] != nullptr)
		{
			if (m_pUpgradeScrollSlots[i]->pUIIcon != nullptr)
			{
				delete m_pUpgradeScrollSlots[i]->pUIIcon;
				m_pUpgradeScrollSlots[i]->pUIIcon = nullptr;
			}
			delete m_pUpgradeScrollSlots[i];
			m_pUpgradeScrollSlots[i] = nullptr;
		}
	}
	// Restore items from backup
	for (int i = 0; i < MAX_ITEM_INVENTORY; i++)
	{
		// Clear first
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

		if (m_pBackupUpgradeInv[i] != nullptr)
		{
			m_pMyUpgradeInv[i] = new __IconItemSkill(*m_pBackupUpgradeInv[i]);

			// Set the UI position based on the inventory area
			CreateUIIconForItem(m_pMyUpgradeInv[i]);
			CN3UIArea* pArea = m_pInvArea[i];
			SetupIconArea(m_pMyUpgradeInv[i], pArea);
			ShowItemCount(m_pMyUpgradeInv[i], i);
		}
	}
}

bool CUIItemUpgrade::IsUpgradeScroll(uint32_t dwEffectID2) const
{
	if (dwEffectID2 == UpgradeMaterial)
		return true;
	return false;
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
	if (m_pUpgradeItemSlot != nullptr)
		return false;
	if (spItem != nullptr  && spItem->pItemBasic != nullptr)
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
				break;
		}
	}
	e_ItemAttrib eTA = (e_ItemAttrib) (spItem->pItemExt->byMagicOrRare);
	return (eTA == ITEM_ATTRIB_UNIQUE || eTA == ITEM_ATTRIB_UPGRADE || eTA == ITEM_ATTRIB_UNIQUE_REVERSE);
}

void CUIItemUpgrade::SendToServerUpgradeMsg()
{
	if (m_pUpgradeItemSlot == nullptr || m_pUpgradeScrollSlots == nullptr)
		return;
	m_bUpgradeInProgress = true;
	uint8_t byBuff[512];
	int iOffset = 0;
	int itotalSent = 0;

	CAPISocket::MP_AddByte(byBuff, iOffset, WIZ_ITEM_UPGRADE);
	CAPISocket::MP_AddByte(byBuff, iOffset, ITEM_UPGRADE_PROCESS);
	CAPISocket::MP_AddByte(byBuff, iOffset, 1);
	CAPISocket::MP_AddByte(byBuff, iOffset, m_iNpcID);

	int32_t nItemID;
	int8_t bPos;

	// Add Upgrade Item
	if (m_pUpgradeItemSlot != nullptr)
	{
		nItemID= m_pUpgradeItemSlot->pItemBasic->dwID + m_pUpgradeItemSlot->pItemExt->dwID;
		bPos= m_iUpgradeItemSlotInvPos; // m_pUpgradeItemSlot position

		CAPISocket::MP_AddDword(byBuff, iOffset, nItemID);
		CAPISocket::MP_AddByte(byBuff, iOffset, bPos);
		++itotalSent;
	}

	// Add Upgrade Slots
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; ++i)
	{
		if (m_pUpgradeScrollSlots[i] != nullptr)
		{
			nItemID= m_pUpgradeScrollSlots[i]->pItemBasic->dwID +
				m_pUpgradeScrollSlots[i]->pItemExt->dwID;
			bPos = m_iUpgradeScrollSlotInvPos[i];
			CAPISocket::MP_AddDword(byBuff, iOffset, nItemID);
			CAPISocket::MP_AddByte(byBuff, iOffset, bPos);
			++itotalSent;
		}
	}

	nItemID = 0;
	bPos = -1;
	while (itotalSent< MAX_UPGRADE_MATERIAL)
	{
		CAPISocket::MP_AddDword(byBuff, iOffset, nItemID);
		CAPISocket::MP_AddByte(byBuff, iOffset, bPos);
		++itotalSent;
	}

	CGameProcedure::s_pSocket->Send(byBuff, iOffset);
}

void CUIItemUpgrade::MsgRecv_ItemUpgrade(Packet& pkt)
{
	int8_t result = pkt.read<uint8_t>();
	uint32_t nItemID[10];
	uint8_t bPos[10];
	for (int i = 0; i < MAX_UPGRADE_MATERIAL; i++)
	{
		pkt >> nItemID[i];
		pkt >> bPos[i];
	}

	__TABLE_ITEM_EXT* itemExt = nullptr;
	__TABLE_ITEM_BASIC* itemBasic = nullptr;
	e_PartPosition ePart;
	e_PlugPosition ePlug;
	std::string szIconFN;
	float fUVAspect = (float) 45.0f / (float) 64.0f;
	std::string szMsg;

	// Clean upgrade scroll slots
	for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; ++i)
	{
		if (m_pUpgradeScrollSlots[i] != nullptr)
		{
			if (m_pUpgradeScrollSlots[i]->pUIIcon != nullptr)
			{
				delete m_pUpgradeScrollSlots[i]->pUIIcon;
				m_pUpgradeScrollSlots[i]->pUIIcon = nullptr;
			}
			delete m_pUpgradeScrollSlots[i];
			m_pUpgradeScrollSlots[i] = nullptr;
		}
	}

	// Clean  item upgrade slot
	if (m_pUpgradeItemSlot != nullptr)
	{
		if (m_pUpgradeItemSlot->pUIIcon != nullptr)
		{
			delete m_pUpgradeItemSlot->pUIIcon;
			m_pUpgradeItemSlot->pUIIcon = nullptr;
		}
		delete m_pUpgradeItemSlot;
		m_pUpgradeItemSlot = nullptr;
	}

	if (result == 0 || result == 1)
	{
		m_eAnimationState = ANIM_START;

		if (result == 0)
		{
			m_bUpgradeSucceeded = false;
			szMsg = fmt::format_text_resource(6701);
			CGameProcedure::s_pProcMain->MsgOutput(szMsg, D3DCOLOR_RGBA(255, 0, 255, 255));
		}
		else if (result == 1)
		{
			m_bUpgradeSucceeded = true;
			szMsg = fmt::format_text_resource(6700);
			CGameProcedure::s_pProcMain->MsgOutput(szMsg, D3DCOLOR_RGBA(128, 128, 255, 255));

			itemBasic = CGameBase::s_pTbl_Items_Basic.Find(nItemID[0] / 1000 * 1000);
			if (itemBasic && itemBasic->byExtIndex >= 0 && itemBasic->byExtIndex <= MAX_ITEM_EXTENSION)
				itemExt = CGameBase::s_pTbl_Items_Exts[itemBasic->byExtIndex].Find(nItemID[0] % 1000);
			else
				itemExt = nullptr;
			e_ItemType eType = CGameBase::MakeResrcFileNameForUPC(itemBasic, itemExt, nullptr, &szIconFN, ePart, ePlug, CGameBase::s_pPlayer->m_InfoBase.eRace);
			if (ITEM_TYPE_UNKNOWN == eType)
				return;
			__IconItemSkill* spItemNew;
			spItemNew = new __IconItemSkill;
			spItemNew->pItemBasic = itemBasic;
			spItemNew->pItemExt = itemExt;
			spItemNew->szIconFN = szIconFN;
			spItemNew->iCount = 1;
			CreateUIIconForItem(spItemNew);

			if (m_pAreaResult != nullptr)
			{
				m_pUpgradeResultSlot = spItemNew;
				m_pMyUpgradeInv[m_iUpgradeItemSlotInvPos] = m_pUpgradeResultSlot;
			}
		}
	}
	else
	{
		m_bUpgradeInProgress = false;
		RestoreInventoryFromBackup();
		if (result == 2)
		{
			// Cannot perform item upgrade
			szMsg = fmt::format_text_resource(6702);
			CGameProcedure::s_pProcMain->MsgOutput(szMsg, D3DCOLOR_RGBA(255, 0, 255, 255));
		}
		else if (result == 3)
		{
			// Upgrade Need coin
			szMsg = fmt::format_text_resource(6703);
			CGameProcedure::s_pProcMain->MsgOutput(szMsg, D3DCOLOR_RGBA(255, 0, 255, 255));
		}
		else if (result == 4)
		{
			// Upgrade Not Match or other
			szMsg = fmt::format_text_resource(6704);
			CGameProcedure::s_pProcMain->MsgOutput(szMsg, D3DCOLOR_RGBA(255, 0, 255, 255));
		}
	}
	UpdateBackupUpgradeInv();
	GoldUpdate();
	CN3UIWndBase::AllHighLightIconFree();
	SetState(UI_STATE_COMMON_NONE);
}

void CUIItemUpgrade::UpdateCoverAnimation()
{
    m_fAnimationTimer += CN3Base::s_fSecPerFrm;
	static const float COVER_ANIMATION_DURATION = 0.8f;
    float t = m_fAnimationTimer / COVER_ANIMATION_DURATION;
    if (t > 1.0f) t = 1.0f;

    // Only handle opening (covers move outward and hide)
    float ease = 1.0f - ((1.0f - t) * (1.0f - t));
	int m_iCoverShift = m_rcCover1Original.bottom - m_rcCover1Original.top;
    // Calculate new Y positions for opening
    int y1 = m_rcCover1Original.top + (int)((m_rcCover1Original.top - m_iCoverShift - m_rcCover1Original.top) * ease);
    int y2 = m_rcCover2Original.top + (int)((m_rcCover2Original.top + m_iCoverShift - m_rcCover2Original.top) * ease);

    // Animation completed - hide covers and reset positions
    if (t >= 1.0f)
    {
        m_eAnimationState = ANIM_NONE;
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
	m_fAnimationTimer += CN3Base::s_fSecPerFrm;
	static const float FLIPFLOP_FRAME_DELAY = 0.1f;

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
    for (int i = 0;; ++i)
    {
		std::string szID = fmt::format("img_s_load_{}", i);
        CN3UIImage* pImg = (CN3UIImage*)GetChildByID(szID);
        if (pImg == nullptr) break;
        pImg->SetVisible(false);
    }
    // Hide all img_f_load_X frames
    for (int i = 0;; ++i)
    {
		std::string szID = fmt::format("img_f_load_{}", i);
        CN3UIImage* pImg = (CN3UIImage*)GetChildByID(szID);
        if (pImg == nullptr) break;
        pImg->SetVisible(false);
    }
}

void CUIItemUpgrade::CreateUIIconForItem(__IconItemSkill* spItem)
{
	if (spItem == nullptr)
		return;

	spItem->pUIIcon = new CN3UIIcon;
	spItem->pUIIcon->Init(this);

	static const float UV_ASPECT_RATIO = 45.0f / 64.0f;
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

bool CUIItemUpgrade::HandleUpgradeAreaDrop(__IconItemSkill* spItem)
{
	// Only Upgrade and Unique items can be dropped here
	if (!IsAllowedUpgradeItem(spItem) || m_bUpgradeInProgress)
		return false;
	if (m_pUpgradeResultSlot != nullptr)
	{
		SetupIconArea(m_pUpgradeResultSlot, m_pInvArea[m_iUpgradeItemSlotInvPos]);
		m_pUpgradeResultSlot = nullptr;
	}
		
	
	// Move the item to the upgrade slot
	m_pUpgradeItemSlot = spItem;

	// Remove the item from inventory
	for (int i = 0; i < MAX_ITEM_INVENTORY; ++i)
	{
		if (m_pMyUpgradeInv[i] == spItem)
		{
			m_pMyUpgradeInv[i] = nullptr;
			m_iUpgradeItemSlotInvPos = i; // m_pUpgradeItemSlot position
			break;
		}
	}
	// Update the item's UI position
	SetupIconArea(spItem, m_pAreaUpgrade);
	return true;
}

bool CUIItemUpgrade::IsSlotCompatible(__IconItemSkill* pSrc) const
{
	if (!IsUpgradeScroll(pSrc->pItemBasic->dwEffectID2))
		return false;

	bool m_bhasTrina = false;
	bool m_bhasScroll = false;

	for (int k = 0; k < MAX_ITEM_UPGRADE_SLOT; ++k)
	{
		if (m_pUpgradeScrollSlots[k] != nullptr)
		{
			if (IsTrina(m_pUpgradeScrollSlots[k]->pItemBasic->dwID))
			{
				m_bhasTrina = true;
			}
			else 
				m_bhasScroll = true;
		}
	}
	if (m_bhasTrina && IsTrina(pSrc->pItemBasic->dwID))
		return false;
	if (m_bhasScroll && !IsTrina(pSrc->pItemBasic->dwID))
		return false;
	if (m_bhasTrina && m_bhasScroll)
		return false;
	return true;
}

bool CUIItemUpgrade::HandleSlotDrop(__IconItemSkill* spItem, int iDestiOrder)
{
	if (iDestiOrder == -1 || m_pUpgradeScrollSlots[iDestiOrder] != nullptr || m_bUpgradeInProgress)
		return false;

	int iSourceOrder = GetItemiOrder(spItem, UIWND_DISTRICT_UPGRADE_INV);
	if (iSourceOrder == -1)
		return false;

	__IconItemSkill* pSrc = m_pMyUpgradeInv[iSourceOrder];
	if (!IsSlotCompatible(pSrc))
		return false;

	// Handle countable items
	if (pSrc->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE ||
		pSrc->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE_SMALL)
	{
		if (pSrc->iCount > 1)
		{
			// Create a new icon, put it in 1 slot,
			__IconItemSkill* pNew = new __IconItemSkill(*pSrc);
			pNew->iCount = 1;
			CreateUIIconForItem(pNew);
			CN3UIArea* pSlotArea = m_pSlotArea[iDestiOrder];
			SetupIconArea(pNew, pSlotArea);
			m_pUpgradeScrollSlots[iDestiOrder] = pNew;

			// Reduce the number in the inventory
			pSrc->iCount -= 1;
			ShowItemCount(pSrc, iSourceOrder);
		}
		else
		{
			// If the last one, move directly
			m_pUpgradeScrollSlots[iDestiOrder] = pSrc;
			m_pMyUpgradeInv[iSourceOrder] = nullptr;
			CN3UIArea* pSlotArea = m_pSlotArea[iDestiOrder];
			SetupIconArea(pSrc, pSlotArea);
		}
	}
	else
	{
		// If is not countable item, just move it
		m_pUpgradeScrollSlots[iDestiOrder] = pSrc;
		m_pMyUpgradeInv[iSourceOrder] = nullptr;
		CN3UIArea* pSlotArea = m_pSlotArea[iDestiOrder];
		SetupIconArea(pSrc, pSlotArea);
	}
	m_iUpgradeScrollSlotInvPos[iDestiOrder] = iSourceOrder;
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
		szID = fmt::format(fmt::runtime(m_bUpgradeSucceeded ? "img_s_load_{}" : "img_f_load_{}"), m_iCurrentFrame - 1);
		if (CN3UIImage* pImg = (CN3UIImage*)GetChildByID(szID))
			pImg->SetVisible(false);
	}

	// Show current frame
	szID = fmt::format(fmt::runtime(m_bUpgradeSucceeded ? "img_s_load_{}" : "img_f_load_{}"), m_iCurrentFrame);
	if (CN3UIImage* pImg = (CN3UIImage*) GetChildByID(szID))
	{
		pImg->SetVisible(true);
		pImg->SetParent(this);
	}
}
void CUIItemUpgrade::AnimClose()
{
	if (m_eAnimationState != ANIM_NONE)
	{
		float m_fAnimationTimer = 0.0f;
		int m_iCurrentFrame = 0;
		m_pImageCover1->SetVisible(false);
		m_pImageCover2->SetVisible(false);
	}
	m_eAnimationState = ANIM_NONE;
	m_bUpgradeInProgress = false;
	HideAllAnimationFrames();
}

void CUIItemUpgrade::ShowResultUpgrade()
{
	// Show result item in result area if upgrade was successful
	if (m_bUpgradeSucceeded && m_pUpgradeResultSlot->pUIIcon != nullptr && m_pAreaResult != nullptr)
	{
		// Set icon position to result area
		SetupIconArea(m_pUpgradeResultSlot, m_pAreaResult);
	}
	m_eAnimationState = ANIM_COVER_OPENING;
}

void CUIItemUpgrade::StartUpgradeAnim()
{
	if (m_bUpgradeInProgress)
	{
		if (m_pImageCover1 == nullptr || m_pImageCover2 == nullptr)
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
}

void CUIItemUpgrade::HandleInventoryIconRightClick(POINT ptCur)
{
    // Check for right mouse button click using MOUSE_RBCLICKED
	if (m_pUpgradeResultSlot != nullptr)
	{
		SetupIconArea(m_pUpgradeResultSlot, m_pInvArea[m_iUpgradeItemSlotInvPos]);
		m_pUpgradeResultSlot = nullptr;
	}
		
    // Find the icon under the mouse
    for (int i = 0; i < MAX_ITEM_INVENTORY; ++i)
    {
        __IconItemSkill* pIconSkill = m_pMyUpgradeInv[i];
        if (pIconSkill == nullptr || pIconSkill->pUIIcon == nullptr)
			continue;

        CN3UIIcon* pIcon = pIconSkill->pUIIcon;
        if (pIcon->IsVisible() && pIcon->IsIn(ptCur.x, ptCur.y))
        {
            // Call HandleSlotDrop and HandleUpgradeAreaDrop for the icon under the mouse
			if (HandleUpgradeAreaDrop(pIconSkill))
			{
				SetupIconArea(pIconSkill, m_pAreaUpgrade);
			}

			for (int i = 0; i < MAX_ITEM_UPGRADE_SLOT; ++i)
			{
				if (HandleSlotDrop(pIconSkill, i)) 
					return;
			}				
            return; // Only handle one icon per click
        }
    }
    
}

void CUIItemUpgrade::ShowItemCount(__IconItemSkill* spItem,int iorder) 
{
	// Display the count for items that should show a count.
	if (spItem != nullptr && spItem->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE || 
		spItem->pItemBasic->byContable == UIITEM_TYPE_COUNTABLE_SMALL)
	{
		CN3UIString* pStr = m_pInvString[iorder];
		if (pStr != nullptr)
		{		
			if (spItem->pUIIcon->IsVisible())
			{
				pStr->SetStringAsInt(spItem->iCount);
				if (GetState() == UI_STATE_ICON_MOVING && CN3UIWndBase::m_sSelectedIconInfo.pItemSelect)
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
		CN3UIString* pStr = m_pInvString[iorder];
		if (pStr != nullptr)
			pStr->SetVisible(false);
	}
	
}

