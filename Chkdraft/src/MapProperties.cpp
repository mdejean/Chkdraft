#include "MapProperties.h"
#include "CommonFiles/Constants.h"
#include "Chkdraft.h"
#include <string>

enum ID {
	EDIT_MAPTITLE = ID_FIRST,
	EDIT_MAPDESCRIPTION,
	CB_MAPTILESET,
	CB_NEWMAPTERRAIN,
	EDIT_NEWMAPWIDTH,
	EDIT_NEWMAPHEIGHT,
	BUTTON_APPLY,
	CB_P1OWNER,
	CB_P2OWNER,
	CB_P3OWNER,
	CB_P4OWNER,
	CB_P5OWNER,
	CB_P6OWNER,
	CB_P7OWNER,
	CB_P8OWNER,
	CB_P9OWNER,
	CB_P10OWNER,
	CB_P11OWNER,
	CB_P12OWNER,
	CB_P1RACE,
	CB_P2RACE,
	CB_P3RACE,
	CB_P4RACE,
	CB_P5RACE,
	CB_P6RACE,
	CB_P7RACE,
	CB_P8RACE,
	CB_P9RACE,
	CB_P10RACE,
	CB_P11RACE,
	CB_P12RACE,
	CB_P1COLOR,
	CB_P2COLOR,
	CB_P3COLOR,
	CB_P4COLOR,
	CB_P5COLOR,
	CB_P6COLOR,
	CB_P7COLOR,
	CB_P8COLOR
};

MapPropertiesWindow::MapPropertiesWindow() : possibleTitleUpdate(false), possibleDescriptionUpdate(false), refreshing(false)
{

}

bool MapPropertiesWindow::CreateThis(HWND hParent, u32 windowId)
{
	if ( getHandle() != NULL )
		return SetParent(hParent);

	if ( ClassWindow::RegisterWindowClass(0, NULL, NULL, NULL, NULL, "MapProperties", NULL, false) &&
		 ClassWindow::CreateClassWindow(0, "MapProperties", WS_VISIBLE|WS_CHILD, 4, 22, 592, 524, hParent, (HMENU)windowId) )
	{
		refreshing = true;
		GuiMapPtr map = chkd.maps.curr;
		HWND hMapProperties = getHandle();

		ChkdString mapTitle(""), mapDescription("");
		u16 currTileset = 0,
			currWidth = 0,
			currHeight = 0;

		if ( map != nullptr )
		{
			map->getMapTitle(mapTitle);
			map->getMapDescription(mapDescription);
			currTileset = map->getTileset(),
			currWidth = map->XSize(),
			currHeight = map->YSize();
		}

		textMapTitle.CreateThis(hMapProperties, 5, 5, 50, 20, "Map Title", 0);
		editMapTitle.CreateThis(hMapProperties, 5, 25, 582, 20, false, EDIT_MAPTITLE);
		editMapTitle.SetText(mapTitle.c_str());

		textMapDescription.CreateThis(hMapProperties, 5, 60, 100, 20, "Map Description", 0);
		editMapDescription.CreateThis(hMapProperties, 5, 80, 582, 100, true, EDIT_MAPDESCRIPTION);
		editMapDescription.SetText(mapDescription.c_str());

		std::string sCurrWidth(std::to_string(currWidth));
		std::string sCurrHeight(std::to_string(currHeight));

		textMapTileset.CreateThis(hMapProperties, 5, 185, 100, 20, "Map Tileset", 0);
		dropMapTileset.CreateThis(hMapProperties, 5, 205, 185, 400, false, false, CB_MAPTILESET, tilesetNames, defaultFont);
		textNewMapTerrain.CreateThis(hMapProperties, 195, 185, 100, 20, "[New] Terrain", 0);
		dropNewMapTerrain.CreateThis(hMapProperties, 195, 205, 185, 400, false, false, CB_NEWMAPTERRAIN, initTerrains[currTileset], defaultFont);
		textNewMapWidth.CreateThis(hMapProperties, 385, 185, 50, 20, "Width", 0);
		editMapWidth.CreateThis(hMapProperties, 385, 205, 50, 20, false, EDIT_NEWMAPWIDTH);
		editMapWidth.SetText(sCurrWidth.c_str());
		textNewMapHeight.CreateThis(hMapProperties, 440, 185, 50, 20, "Height", 0);
		editMapHeight.CreateThis(hMapProperties, 440, 205, 50, 20, false, EDIT_NEWMAPHEIGHT);
		editMapHeight.SetText(sCurrHeight.c_str());
		buttonApply.CreateThis(hMapProperties, 494, 205, 91, 20, "Apply", BUTTON_APPLY);

		const char* sPlayers[] = { "Player 1", "Player 2" , "Player 3" , "Player 4",
								   "Player 5", "Player 6" , "Player 7" , "Player 8",
								   "Player 9", "Player 10", "Player 11", "Player 12" };

		for ( int yBox=0; yBox<3; yBox++ )
		{
			for ( int xBox=0; xBox<4; xBox++ )
			{
				int player = yBox*4+xBox;
				u8 displayOwner = 0, race = 0, color = (u8)player;
				if ( map != nullptr )
				{
					displayOwner = map->getDisplayOwner((u8)player);
					map->getPlayerRace((u8)player, race);
				}

				groupMapPlayers[yBox*4+xBox].CreateThis(hMapProperties, 5+146*xBox, 242+95*yBox, 141, 91, sPlayers[yBox*4+xBox], 0);
				textPlayerOwner[yBox*4+xBox].CreateThis(hMapProperties, 15+146*xBox, 257+95*yBox, 50, 20, "Owner", 0);
				dropPlayerOwner[yBox*4+xBox].CreateThis(hMapProperties, 60+146*xBox, 257+95*yBox, 80, 140, false, false, CB_P1OWNER+player, playerOwners, defaultFont);
				textPlayerRace[yBox*4+xBox].CreateThis(hMapProperties, 15+146*xBox, 282+95*yBox, 50, 20, "Race", 0);
				dropPlayerRaces[yBox*4+xBox].CreateThis(hMapProperties, 60+146*xBox, 282+95*yBox, 80, 110, false, false, CB_P1RACE+player, playerRaces, defaultFont);

				if ( yBox < 2 )
				{
					if ( map != nullptr )
						map->getPlayerColor((u8)player, color);

					textPlayerColor[player].CreateThis(hMapProperties, 15+146*xBox, 307+95*yBox, 50, 20, "Color", 0);
					dropPlayerColor[player].CreateThis(hMapProperties, 60+146*xBox, 307+95*yBox, 80, 140, true, false, CB_P1COLOR+player, playerColors, defaultFont);
				}
			}
		}
		refreshing = false;
		return true;
	}
	else
		return false;
}

void MapPropertiesWindow::RefreshWindow()
{
	refreshing = true;
	GuiMapPtr map = chkd.maps.curr;
	if ( map != nullptr )
	{
		ChkdString mapTitle, mapDescription;
		map->getMapTitle(mapTitle);
		map->getMapDescription(mapDescription);
		u16 tileset = map->getTileset(),
			currWidth = map->XSize(),
			currHeight = map->YSize();

		std::string sCurrWidth(std::to_string(currWidth));
		std::string sCurrHeight(std::to_string(currHeight));
		
		editMapTitle.SetText(mapTitle.c_str());
		editMapDescription.SetText(mapDescription.c_str());
		possibleTitleUpdate = false;
		possibleDescriptionUpdate = false;
		dropMapTileset.SetSel(tileset);
		dropMapTileset.ClearEditSel();
		dropNewMapTerrain.SetSel(0);
		dropNewMapTerrain.ClearEditSel();
		editMapWidth.SetText(sCurrWidth.c_str());
		editMapHeight.SetText(sCurrHeight.c_str());
					
		for ( int player=0; player<12; player++ )
		{
			u8 displayOwner(map->getDisplayOwner(player)), race(0), color(0);
			map->getPlayerRace(player, race);
			dropPlayerOwner[player].SetSel(displayOwner);
			dropPlayerOwner[player].ClearEditSel();
			dropPlayerRaces[player].SetSel(race);
			dropPlayerRaces[player].ClearEditSel();

			if ( player < 8 )
			{
				map->getPlayerColor(player, color);
				dropPlayerColor[player].SetSel(0);
				dropPlayerColor[player].ClearEditSel();
			}
		}
	}
	refreshing = false;
}

LRESULT MapPropertiesWindow::Command(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
	switch ( LOWORD(wParam) )
	{
	case EDIT_MAPTITLE:
		if ( HIWORD(wParam) == EN_CHANGE && !refreshing )
			possibleTitleUpdate = true;
		else if ( HIWORD(wParam) == EN_KILLFOCUS )
			CheckReplaceMapTitle();
		break;

	case EDIT_MAPDESCRIPTION:
		if ( HIWORD(wParam) == EN_CHANGE && !refreshing )
			possibleDescriptionUpdate = true;
		else if ( HIWORD(wParam) == EN_KILLFOCUS )
			CheckReplaceMapDescription();
		break;

	case BUTTON_APPLY:
	{
		if ( HIWORD(wParam) == BN_CLICKED )
		{
			LRESULT newTileset = SendMessage(GetDlgItem(hWnd, CB_MAPTILESET), CB_GETCURSEL, 0, 0);
			chkd.maps.curr->setTileset((u16)newTileset);
			u16 newWidth, newHeight;
			if ( editMapWidth.GetEditNum<u16>(newWidth) && editMapHeight.GetEditNum<u16>(newHeight) )
				chkd.maps.curr->setDimensions((u16)newWidth, (u16)newHeight);

			// Apply new terrain...

			chkd.maps.curr->notifyChange(false);
			chkd.maps.curr->Redraw(true);
		}
	}
	break;

	case CB_MAPTILESET:
		if ( HIWORD(wParam) == CBN_SELCHANGE )
		{
			HWND hMapTileset = GetDlgItem(hWnd, CB_MAPTILESET), hMapNewTerrain = GetDlgItem(hWnd, CB_NEWMAPTERRAIN);
			LRESULT currTileset = SendMessage(hMapTileset, CB_GETCURSEL, 0, 0);
			if ( currTileset != CB_ERR && currTileset < (LRESULT)tilesetNames.size())
			{
				while ( SendMessage(hMapNewTerrain, CB_DELETESTRING, 0, 0) != CB_ERR );

				for ( auto tileset : initTerrains.at(currTileset) )
					SendMessage(hMapNewTerrain, CB_ADDSTRING, 0, (LPARAM)tileset.c_str());

				SendMessage(hMapNewTerrain, WM_SETFONT, (WPARAM)defaultFont, MAKELPARAM(TRUE, 0));
				SendMessage(hMapNewTerrain, CB_SETCURSEL, 0, 0);
				PostMessage(hMapNewTerrain, CB_SETEDITSEL, 0, MAKELPARAM(-1, 0));
			}
		}
		break;
	case CB_P1OWNER: case CB_P2OWNER: case CB_P3OWNER: case CB_P4OWNER:
	case CB_P5OWNER: case CB_P6OWNER: case CB_P7OWNER: case CB_P8OWNER:
	case CB_P9OWNER: case CB_P10OWNER: case CB_P11OWNER: case CB_P12OWNER:
		if ( HIWORD(wParam) == CBN_SELCHANGE )
		{
			u32 player = LOWORD(wParam) - CB_P1OWNER; // 0 based player
			u8 newOwner = 0;
			LRESULT sel = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
			if ( player >= 0 && player < 12 && sel != CB_ERR )
			{
				switch ( sel )
				{
				case 0: newOwner = 4; break;
				case 1: newOwner = 3; break;
				case 2: newOwner = 5; break;
				case 3: newOwner = 6; break;
				case 4: newOwner = 7; break;
				}
				if ( newOwner != 0 )
				{
					chkd.maps.curr->setPlayerOwner((u8)player, newOwner);
					chkd.maps.curr->notifyChange(false);
				}
			}
		}
		break;

	case CB_P1RACE: case CB_P2RACE: case CB_P3RACE: case CB_P4RACE:
	case CB_P5RACE: case CB_P6RACE: case CB_P7RACE: case CB_P8RACE:
	case CB_P9RACE: case CB_P10RACE: case CB_P11RACE: case CB_P12RACE:
		if ( HIWORD(wParam) == CBN_SELCHANGE )
		{
			u32 player = LOWORD(wParam) - CB_P1RACE; // 0 based player
			LRESULT newRace = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
			if ( player >= 0 && player < 12 && newRace != CB_ERR && newRace >= 0 && newRace < 8 )
			{
				chkd.maps.curr->setPlayerRace((u8)player, (u8)newRace);
				chkd.maps.curr->notifyChange(false);
			}
		}
		break;

	case CB_P1COLOR: case CB_P2COLOR: case CB_P3COLOR: case CB_P4COLOR:
	case CB_P5COLOR: case CB_P6COLOR: case CB_P7COLOR: case CB_P8COLOR:
		switch ( HIWORD(wParam) )
		{
		case CBN_SELCHANGE:
		{
			u32 player = LOWORD(wParam) - CB_P1COLOR; // 0 based player
			LRESULT newColor = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);
			if ( player >= 0 && player < 12 && newColor != CB_ERR && newColor >= 0 && newColor < 16 )
			{
				if ( chkd.maps.curr->setPlayerColor((u8)player, (u8)newColor) )
					chkd.maps.curr->Redraw(true);

				chkd.maps.curr->notifyChange(false);
			}
		}
		break;
		case CBN_EDITCHANGE:
		{
			u32 player = LOWORD(wParam) - CB_P1COLOR; // 0 based player
			u8 newColor;
			if ( dropPlayerColor[player].GetEditNum<u8>(newColor) )
			{
				if ( chkd.maps.curr->setPlayerColor((u8)player, newColor) )
					chkd.maps.curr->Redraw(true);

				chkd.maps.curr->notifyChange(false);
			}
		}

		}
		break;
	}
	return ClassWindow::Command(hWnd, wParam, lParam);
}

LRESULT MapPropertiesWindow::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch ( msg )
	{
		case WM_SHOWWINDOW:
			if ( wParam == TRUE )
			{
				RefreshWindow();
				chkd.mapSettingsWindow.SetTitle("Map Settings");
			}
			else if ( !refreshing )
			{
				CheckReplaceMapTitle();
				CheckReplaceMapDescription();
			}
			return ClassWindow::WndProc(hWnd, msg, wParam, lParam);
			break;

		default:
			return ClassWindow::WndProc(hWnd, msg, wParam, lParam);
			break;
	}
	return 0;
}

void MapPropertiesWindow::CheckReplaceMapTitle()
{
	ChkdString newMapTitle;
	if ( possibleTitleUpdate == true && editMapTitle.GetEditText(newMapTitle) )
	{
		u16* mapTitleString;
		if ( chkd.maps.curr->SPRP().getPtr<u16>(mapTitleString, 0, 2) &&
			 chkd.maps.curr->replaceString(newMapTitle, *mapTitleString, false, true) )
		{
			chkd.maps.curr->notifyChange(false);
		}
		possibleTitleUpdate = false;
	}
}

void MapPropertiesWindow::CheckReplaceMapDescription()
{
	ChkdString newMapDescription;
	if ( possibleDescriptionUpdate == true && editMapDescription.GetEditText(newMapDescription) )
	{
		u16* mapDescriptionString;
		if ( chkd.maps.curr->SPRP().getPtr<u16>(mapDescriptionString, 2, 2) &&
			 chkd.maps.curr->replaceString(newMapDescription, *mapDescriptionString, false, true) )
		{
			chkd.maps.curr->notifyChange(false);
		}
		possibleDescriptionUpdate = false;
	}
}
