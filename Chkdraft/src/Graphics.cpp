#include "Graphics.h"
#include "CommonFiles/CommonFiles.h"
#include "Chkdraft.h"
#include <algorithm>
#include <string>

DWORD ColorCycler::ccticks(0);

colorcycle ColorCycler::cctable[4][8] = { // Different color cycling definition tables
	{ { 1,8,0,1,6 },{ 1,8,0,7,13 },{ 1,8,0,248,254 },{ 0 } },
	{ { 0 } },
	{ { 1,8,0,1,4 },{ 1,8,0,5,8 },{ 1,8,0,9,13 },{ 0 } },
	{ { 1,8,0,1,13 },{ 1,8,0,248,254 },{ 0 } }
};

const u8 ColorCycler::cctilesets[8] = { 0,1,1,2,0,3,3,3 };

bool ColorCycler::CycleColors(const u16 tileset)
{
	bool redraw = false;
	if ( GetTickCount() > ccticks ) // Time to update for next tick?
	{
		colorcycle* cc = cctable[cctilesets[tileset]]; // Get current tileset's definition table
		for ( int ccset = 0; ccset < 8; ccset++ ) // For each record in table
		{
			if ( cc[ccset].enabled == false ) continue; // Not enabled
			if ( cc[ccset].timer > 0 ) // Timer hasn't expired
			{
				cc[ccset].timer--;
			}
			else // Do our thing !
			{
				cc[ccset].timer = cc[ccset].steps; // Reset timer
				for ( int i = cc[ccset].stop; i > cc[ccset].start; i-- ) // For each from start to stop
					chkd.scData.tilesets.set[tileset].wpe.swap<u32>(i * 4, (i - 1) * 4); // Rotate right
				
				redraw = true; // Tell the caller the map should be redrawn
			}
		}
		ccticks = GetTickCount();
	}
	return redraw;
}

inline u32 AdjustPx(u32 pixel, u8 redOffset, u8 greenOffset, u8 blueOffset)
{
	((u8*)&pixel)[0] += redOffset;
	((u8*)&pixel)[1] += greenOffset;
	((u8*)&pixel)[2] += blueOffset;
	return pixel;
}

void Graphics::DrawMap(u16 bitWidth, u16 bitHeight, s32 screenLeft, s32 screenTop, ChkdBitmap& bitmap,
					   SELECTIONS& selections, u32 layer, HDC hDC, bool showAnywhere)
{
	this->screenLeft = screenLeft;
	this->screenTop = screenTop;

	screenWidth = bitWidth;
	screenHeight = bitHeight;

	mapWidth = chk.XSize();
	mapHeight = chk.YSize();

	if ( displayingElevations )
		DrawTileElevations(bitmap);
	else
		DrawTerrain(bitmap);

	DrawGrid(bitmap);

	DrawUnits(bitmap, selections);

	DrawSprites(bitmap);

	if ( layer == LAYER_LOCATIONS )
		DrawLocations(bitmap, selections, showAnywhere);

	BITMAPINFO bmi = GetBMI(screenWidth, screenHeight);
	SetDIBitsToDevice( hDC, 0, 0, screenWidth, screenHeight, 0, 0, 0,
					   screenHeight, bitmap.data(), &bmi, DIB_RGB_COLORS);

	if ( layer == LAYER_LOCATIONS )	
		DrawLocationNames(hDC);

	if ( displayingTileNums )
		DrawTileNumbers(hDC);
}

void Graphics::DrawTerrain(ChkdBitmap& bitmap)
{
	buffer& ERA  = chk.ERA (),
		  & MTXM = chk.MTXM();

	u32 maxRowX, maxRowY, mtxmRef;

	u16 TileValue,
		yTile, xTile,
		tileset;

	if ( !ERA.get<u16>(tileset, 0) ) return; // Map must have a tileset

	TileSet* tiles = &chkd.scData.tilesets.set[tileset];

	if ( screenHeight > u32(mapHeight*32) )
		maxRowY = mapHeight;
	else
		maxRowY = (screenTop+screenHeight)/32+1;

	if ( screenWidth > u32(mapWidth*32) )
		maxRowX = mapWidth;
	else
		maxRowX = (screenLeft+screenWidth)/32+1;

	for ( yTile = (u16)(screenTop/32); yTile < maxRowY; yTile++ ) // Cycle through all rows on the screen
	{
		for ( xTile = (u16)(screenLeft/32); xTile < maxRowX; xTile++ ) // Cycle through all columns on the screen
		{
			mtxmRef = 2*(mapWidth*yTile+xTile);
			if ( MTXM.get<u16>(TileValue, mtxmRef) )
			{
				TileToBits(bitmap,
						    tiles,
						    s32(xTile)*32-screenLeft,
						    s32(yTile)*32-screenTop,
						    u16(screenWidth),
						    u16(screenHeight),
						    TileValue
						  );
			}
		}
	}
}

void Graphics::DrawTileElevations(ChkdBitmap& bitmap)
{
	buffer& MTXM = chk.MTXM();

	u32 maxRowX, maxRowY,
				  xc, yc;

	u16 wTileHex;

	if ( screenHeight > u32(mapHeight*32) )
		maxRowY = mapHeight;
	else
		maxRowY = (screenTop+screenHeight)/32+1;

	if ( screenWidth > u32(mapWidth*32) )
		maxRowX = mapWidth;
	else
		maxRowX = (screenLeft+screenWidth)/32+1;

	BITMAPINFO bmi = GetBMI(32, 32);

	u8 tileset;
	if ( !chk.ERA().get<u8>(tileset, 0) ) return; // Map must have a tileset
	TileSet* tiles = &chkd.scData.tilesets.set[tileset];

	for ( yc=screenTop/32; yc<maxRowY; yc++ )
	{
		for ( xc=screenLeft/32; xc<maxRowX; xc++ )
		{
			if ( MTXM.get<u16>(wTileHex, 2*mapWidth*yc+2*xc) )
				TileElevationsToBits(bitmap, screenWidth, screenHeight, tiles,
									  s16(xc*32-screenLeft), s16(yc*32-screenTop), wTileHex, bmi, 0 );
		}
	}
}

void Graphics::DrawGrid(ChkdBitmap& bitmap)
{
	u16 gridXSize,
		gridYSize,
		x, y;

	for ( u32 i=0; i<2; i++ )
	{
		GRID& currGrid = grids[i];
		gridXSize = currGrid.size.x;
		gridYSize = currGrid.size.y;

		if ( gridXSize > 0 )
		{
			for ( x = gridXSize-(screenLeft%gridXSize); x < screenWidth; x += gridXSize ) // Draw vertical lines
			{
				for ( y = 0; y < screenHeight; y++ )
					bitmap[y*screenWidth + x] = (u32&)currGrid.red;
			}
		}
			
		if ( gridYSize > 0 )
		{
			for ( y = gridYSize-(screenTop%gridYSize); y < screenHeight; y += gridYSize )
			{
				for ( x = 0; x < screenWidth; x++ )
					bitmap[y*screenWidth + x] = (u32&)currGrid.red;
			}
		}
	}
}

void Graphics::DrawLocations(ChkdBitmap& bitmap, SELECTIONS& selections, bool showAnywhere)
{
	buffer& MRGN = chk.MRGN();
	ChkLocation* loc;
	s32 screenRight = screenLeft+screenWidth;
	s32 screenBottom = screenTop+screenHeight;
	u32 bitMax = screenWidth*screenHeight;

	for ( u32 locPos=0; locPos<MRGN.size(); locPos+=CHK_LOCATION_SIZE )
	{
		if ( MRGN.getPtr<ChkLocation>(loc, locPos, CHK_LOCATION_SIZE) && ( locPos != 63*CHK_LOCATION_SIZE || showAnywhere ) )
		{
			s32 leftMost = std::min(loc->xc1, loc->xc2);
			if ( leftMost < screenRight )
			{
				s32 rightMost = std::max(loc->xc1, loc->xc2);
				if ( rightMost > screenLeft )
				{
					s32 topMost = std::min(loc->yc1, loc->yc2);
					if ( topMost < screenBottom )
					{
						s32 bottomMost = std::max(loc->yc1, loc->yc2);
						if ( bottomMost > screenTop )
						{
							bool leftMostOnScreen	= true,
								 rightMostOnScreen	= true,
								 topMostOnScreen	= true,
								 bottomMostOnScreen = true,
								 inverted = (loc->xc1 > loc->xc2 || loc->yc1 > loc->yc2);

							if ( leftMost < screenLeft )
							{
								leftMost = screenLeft;
								leftMostOnScreen = false;
							}
							if ( rightMost > screenRight )
							{
								rightMost = screenRight;
								rightMostOnScreen = false;
							}
							if ( topMost < screenTop )
							{
								topMost = screenTop;
								topMostOnScreen = false;
							}
							if ( bottomMost > screenBottom )
							{
								bottomMost = screenBottom;
								bottomMostOnScreen = false;
							}

							leftMost -= screenLeft;
							rightMost -= screenLeft;
							topMost -= screenTop;
							bottomMost -= screenTop;

							if ( leftMostOnScreen )
							{
								for ( s32 y = topMost; y < bottomMost; y++ )
									bitmap[y*screenWidth + leftMost] = 0;
							}
							if ( rightMostOnScreen )
							{
								for ( s32 y = topMost; y < bottomMost; y++ )
									bitmap[y*screenWidth + rightMost] = 0;
							}
							if ( topMostOnScreen )
							{
								for ( s32 x = leftMost; x < rightMost; x++ )
									bitmap[topMost*screenWidth + x] = 0;
							}
							if ( bottomMostOnScreen )
							{
								for ( s32 x = leftMost; x < rightMost; x++ )
									bitmap[bottomMost*screenWidth + x] = 0;
							}

							if ( inverted )
							{
								// Base: +20, -10, -10

								for ( s32 y=topMost; y<bottomMost; y++ )
								{
									for ( s32 x=leftMost; x<rightMost; x++ )
									{
										if ( bitmap[y*screenWidth+x+2] < 236 )
											bitmap[y*screenWidth+x+2] += 20;
										else
											bitmap[y*screenWidth+x+2] = 255;

										if ( bitmap[y*screenWidth+x+1] > 9 )
											bitmap[y*screenWidth+x+1] -= 10;
										else
											bitmap[y*screenWidth+x+1] = 0;

										if ( bitmap[y*screenWidth+x] > 9 )
											bitmap[y*screenWidth+x] -= 10;
										else
											bitmap[y*screenWidth+x] = 0;
									}
								}
							}
							else
							{
								// Base: -10, +10, +15
								for ( s32 y=topMost; y<bottomMost; y++ )
								{
									for ( s32 x=leftMost; x<rightMost; x++ )
									{
										if ( bitmap[y*screenWidth+x+2] > 9 )
											bitmap[y*screenWidth+x+2] -= 10;
										else
											bitmap[y*screenWidth+x+2] = 0;

										if ( bitmap[y*screenWidth+x+1] < 246 )
											bitmap[y*screenWidth+x+1] += 10;
										else
											bitmap[y*screenWidth+x+1] = 0;

										if ( bitmap[y*screenWidth+x] < 241 )
											bitmap[y*screenWidth+x] += 15;
										else
											bitmap[y*screenWidth+x] = 255;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	u16 selectedLoc = selections.getSelectedLocation();
	if ( selectedLoc != NO_LOCATION )
	{
		if ( MRGN.getPtr<ChkLocation>(loc, selectedLoc*CHK_LOCATION_SIZE, CHK_LOCATION_SIZE) )
		{
			s32 leftMost = std::min(loc->xc1, loc->xc2);
			s32 rightMost = std::max(loc->xc1, loc->xc2);
			s32 topMost = std::min(loc->yc1, loc->yc2);
			s32 bottomMost = std::max(loc->yc1, loc->yc2);
			if ( leftMost < screenRight && rightMost > screenLeft && topMost < screenBottom && bottomMost > screenTop )
			{
				bool leftMostOnScreen	= leftMost >= screenLeft,
					 rightMostOnScreen	= rightMost <= screenRight,
					 topMostOnScreen	= topMost >= screenTop,
					 bottomMostOnScreen = bottomMost <= screenBottom;

				if ( !leftMostOnScreen )
					leftMost = 0;
				else
					leftMost -= screenLeft;

				if ( !rightMostOnScreen )
					rightMost = screenWidth;
				else
					rightMost -= screenLeft;

				if ( !topMostOnScreen )
					topMost = 0;
				else
					topMost -= screenTop;

				if ( !bottomMostOnScreen )
					bottomMost = screenHeight;
				else
					bottomMost -= screenTop;

				if ( leftMostOnScreen )
				{
					for ( s32 y = topMost; y < bottomMost; y++ )
						bitmap[y*screenWidth + leftMost] = RGB(255, 255, 255);
				}
				if ( rightMostOnScreen )
				{
					for ( s32 y = topMost; y < bottomMost; y++ )
						bitmap[y*screenWidth + rightMost] = RGB(255, 255, 255);
				}
				if ( topMostOnScreen )
				{
					for ( s32 x = leftMost; x < rightMost; x++ )
						bitmap[topMost*screenWidth + x] = RGB(255, 255, 255);
				}
				if ( bottomMostOnScreen )
				{
					for ( s32 x = leftMost; x < rightMost; x++ )
						bitmap[bottomMost*screenWidth + x] = RGB(255, 255, 255);
				}
			}
		}
	}
}

void Graphics::DrawUnits(ChkdBitmap& bitmap, SELECTIONS& selections)
{
	buffer& UNIT = chk.UNIT(),
		  & ERA  = chk.ERA (),
		  & COLR = chk.COLR();

	s32	UnitTableSize = UNIT.size();

	u32 screenRight = screenLeft+screenWidth,
		screenBottom = screenTop+screenHeight;

	u8 tileset;
	if ( !ERA.get<u8>(tileset, 0) ) return; // Map must have a tileset

	buffer* palette = &chkd.scData.tilesets.set[tileset].wpe;
	ChkUnit* unit;

	for ( s32 pos = 0; pos < UnitTableSize; pos+=UNIT_STRUCT_SIZE )
	{
		if ( UNIT.getPtr(unit, pos, UNIT_STRUCT_SIZE) )
		{
			if ( unit->xc+MAX_UNIT_RIGHT > (s64)screenLeft &&
				 unit->xc-MAX_UNIT_LEFT < (s64)screenRight )
				// If within screen x-bounds
			{
				if ( unit->yc+MAX_UNIT_DOWN > (s64)screenTop &&
					 unit->yc-MAX_UNIT_UP < (s64)screenBottom )
					// If within screen y-bounds
				{
					u16 frame = 0;
					u8 color;
					if ( unit->owner < 8 )
						color = COLR.get<u8>(unit->owner);
					else
						color = unit->owner;

					bool isSelected = selections.unitIsSelected(u16(pos/UNIT_STRUCT_SIZE));

					UnitToBits(bitmap, palette, color, u16(screenWidth), u16(screenHeight),
								screenLeft, screenTop, unit->id, unit->xc, unit->yc,
								u16(frame), isSelected );
				}
			}
		}
	}
}

void Graphics::DrawSprites(ChkdBitmap& bitmap)
{
	buffer& THG2 = chk.THG2(),
		  & ERA  = chk.ERA (),
		  & COLR = chk.COLR();

	s32	SpriteTableSize = THG2.size();

	u32 screenRight = screenLeft + screenWidth,
		screenBottom = screenTop + screenHeight;

	u8 tileset;
	if ( !ERA.get<u8>(tileset, 0) ) return; // Map must have a tileset

	buffer* palette = &chkd.scData.tilesets.set[tileset].wpe;
	ChkSprite* sprite;

	for ( s32 pos = 0; pos < SpriteTableSize; pos += SPRITE_STRUCT_SIZE )
	{
		if ( THG2.getPtr(sprite, pos, SPRITE_STRUCT_SIZE) )
		{
			if ( sprite->xc+MAX_UNIT_RIGHT > (s64)screenLeft &&
				 sprite->xc-MAX_UNIT_LEFT < (s64)screenRight )
				 // If within screen x-bounds
			{
				if ( sprite->yc+MAX_UNIT_DOWN > (s64)screenTop &&
					 sprite->yc-MAX_UNIT_UP < (s64)screenBottom )
					 // If within screen y-bounds
				{
					u16 frame = 0;
					bool isSprite = (sprite->flags & FLAG_DRAW_AS_SPRITE) > 0;
					u8 color;
					if ( sprite->owner < 8 )
						color = COLR.get<u8>(sprite->owner);
					else
						color = sprite->owner;

					if ( isSprite )
						SpriteToBits(bitmap, palette, color, u16(screenWidth), u16(screenHeight),
									  screenLeft, screenTop, sprite->id, sprite->xc, sprite->yc );
					else
						UnitToBits(bitmap, palette, color, u16(screenWidth), u16(screenHeight),
									screenLeft, screenTop, sprite->id, sprite->xc, sprite->yc,
									frame, false );
				}
			}
		}
	}
}

void Graphics::DrawLocationNames(HDC hDC)
{
	s32 screenRight = screenLeft + screenWidth;
	s32 screenBottom = screenTop + screenHeight;
	buffer& MRGN = chk.MRGN();
	ChkLocation* loc;

	HFONT NumFont = CreateFont(14, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Microsoft Sans Serif");
	SelectObject(hDC, NumFont);
	SetBkMode( hDC, TRANSPARENT );
	SetTextColor(hDC, RGB(255, 255, 0));

	for ( u32 locPos=0; locPos<MRGN.size(); locPos+=CHK_LOCATION_SIZE )
	{
		if ( MRGN.getPtr<ChkLocation>(loc, locPos, CHK_LOCATION_SIZE) && locPos != 63*CHK_LOCATION_SIZE )
		{
			s32 leftMost = std::min(loc->xc1, loc->xc2);
			if ( leftMost < screenRight )
			{
				s32 rightMost = std::max(loc->xc1, loc->xc2);
				if ( rightMost > screenLeft )
				{
					s32 topMost = std::min(loc->yc1, loc->yc2);
					if ( topMost < screenBottom )
					{
						s32 bottomMost = std::max(loc->yc1, loc->yc2);
						if ( bottomMost > screenTop )
						{
							ChkdString str;
							if ( chk.getLocationName(u16(locPos/CHK_LOCATION_SIZE), str) )
							{
								leftMost = leftMost-screenLeft+2;
								topMost = topMost-screenTop+2;
								RECT rect = { };
								SIZE strSize = { };
								GetTextExtentPoint32(hDC, &str[0], str.size(), &strSize);
								s32 lineHeight = strSize.cy;
								if ( clipLocationNames )
								{
									rect.left = (leftMost < 0)? 0 : leftMost;
									rect.top  = ( topMost < 0)? 0 : topMost;
									rect.bottom = bottomMost-screenTop-1;
									rect.right = rightMost-screenLeft-1;
									LONG rectWidth = rect.right-rect.left,
										 rectHeight = rect.bottom-rect.top;

									if ( strSize.cx < rectWidth )
										ExtTextOut(hDC, leftMost, topMost, ETO_CLIPPED, &rect, &str[0], str.length(), 0);
									else if ( rectHeight > lineHeight ) // Can word wrap
									{
										u32 lastCharPos = str.size()-1;
										s32 prevBottom = rect.top;

										while ( rect.bottom-prevBottom > lineHeight && str.size() > 0 )
										{
											// Binary search for the character length of this line
											u32 floor = 0;
											u32 ceil = str.size();
											while ( ceil-1 > floor )
											{
												lastCharPos = (ceil-floor)/2 + floor;
												GetTextExtentPoint32(hDC, &str[0], lastCharPos, &strSize);
												if ( strSize.cx > rectWidth )
													ceil = lastCharPos;
												else
													floor = lastCharPos;
											}
											GetTextExtentPoint32(hDC, &str[0], floor+1, &strSize); // Correct last character if needed
											if ( strSize.cx > rectWidth )
												lastCharPos = floor;
											else
												lastCharPos = ceil;
											// End binary search

											ExtTextOut(hDC, leftMost, prevBottom, ETO_CLIPPED, &rect, &str[0], lastCharPos, 0);
											str = str.substr(lastCharPos, str.size());
											prevBottom += lineHeight;
										}
									}
								}
								else
									ExtTextOut(hDC, leftMost, topMost, 0, &rect, &str[0], str.length(), 0);
							}
						}
					}
				}
			}
		}
	}
	DeleteObject(NumFont);
}

void Graphics::DrawTileNumbers(HDC hDC)
{
	buffer* tileBuf = nullptr;
	if ( tileNumsFromMTXM )
		tileBuf = &chk.MTXM();
	else
		tileBuf = &chk.TILE();

	u32 maxRowX, maxRowY,
		xc, yc;

	u16 wTileHex;

	if ( screenHeight > u32(mapHeight*32) )
		maxRowY = mapHeight;
	else
		maxRowY = (screenTop+screenHeight)/32+1;

	if ( screenWidth > u32(mapWidth*32) )
		maxRowX = mapWidth;
	else
		maxRowX = (screenLeft+screenWidth)/32+1;

	HFONT NumFont = CreateFont(14, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Microsoft Sans Serif");
	SelectObject(hDC, NumFont);
	SetBkMode( hDC, TRANSPARENT );
	SetTextColor(hDC, RGB(255, 255, 0));
	RECT nullRect = { };
	std::string TileHex;

	for ( yc=screenTop/32; yc<maxRowY; yc++ )
	{
		for ( xc=screenLeft/32; xc<maxRowX; xc++ )
		{
			if ( tileBuf->get<u16>(wTileHex, 2 * mapWidth*yc + 2 * xc) )
				TileHex = std::to_string(wTileHex);
			else
				TileHex = "DNE";

			ExtTextOut(hDC, xc * 32 - screenLeft + 3, yc * 32 - screenTop + 2, ETO_OPAQUE, &nullRect, TileHex.c_str(), TileHex.length(), 0);
		}
	}
	DeleteObject(NumFont);
}

void Graphics::ToggleTileNumSource(bool MTXMoverTILE)
{
	if ( !( displayingTileNums && tileNumsFromMTXM != MTXMoverTILE ) )
		displayingTileNums = !displayingTileNums;

	tileNumsFromMTXM = MTXMoverTILE;
}

bool Graphics::mtxmOverTile()
{
	return tileNumsFromMTXM;
}

bool Graphics::DisplayingTileNums()
{
	return displayingTileNums;
}

void Graphics::ToggleLocationNameClip()
{
	clipLocationNames = !clipLocationNames;
}

bool Graphics::ClippingLocationNames()
{
	return clipLocationNames;
}

void Graphics::ToggleDisplayElevations()
{
	displayingElevations = !displayingElevations;
}

bool Graphics::DisplayingElevations()
{
	return displayingElevations;
}

GRID& Graphics::grid(u32 gridNum)
{
	if ( gridNum >=0 && gridNum < 2 )
		return grids[gridNum];
	else
		return grids[0];
}

BITMAPINFO GetBMI(s32 width, s32 height)
{
	BITMAPINFOHEADER bmiH = {};
	bmiH.biSize = sizeof(BITMAPINFOHEADER);
	bmiH.biWidth = width;
	bmiH.biHeight = -height;
	bmiH.biPlanes = 1;
	bmiH.biBitCount = 32;
	bmiH.biCompression = BI_RGB;
	bmiH.biXPelsPerMeter = 1;
	bmiH.biYPelsPerMeter = 1;
					
	BITMAPINFO bmi = {};
	bmi.bmiHeader = bmiH;
	return bmi;
}

void TileElevationsToBits(ChkdBitmap& bitmap, u32 &bitWidth, u32 &bitHeight, TileSet* tiles,
						   s16 xOffset, s16 yOffset, u16 &TileValue, BITMAPINFO &bmi, u8 miniTileSeparation )
{
	u32 bitMax = bitWidth*bitHeight*3,
		cv5Ref, MegaTileRef; // Pointers and index's to tile components

	u8 yMiniTile,  xMiniTile;

	if ( GetCV5References(tiles, cv5Ref, TileValue) ) // Get tile CV5 start point for the given MTXM value
	{
		MegaTileRef = GetMegaTileRef(tiles, cv5Ref); // Get tile VX4 start point ('MegaTile') for the given CV5 struct

		for ( yMiniTile=0; yMiniTile<4; yMiniTile++ ) // Cycle through the 4 mini tile rows
		{
			for ( xMiniTile=0; xMiniTile<4; xMiniTile++ ) // Cycle through the 4 mini tile columns
			{
				u8 miniFlags = 0;
				if ( tiles->vf4.get<u8>(miniFlags, MegaTileRef+2*(4*yMiniTile+xMiniTile)) )
				{
					u8 red = 0, blue = 0, green = 100;
					if ( miniFlags & 0x2 ) // Mid ground
					{
						blue = 100;
						green = 0;
					}
					if ( miniFlags & 0x4 ) // High ground
					{
						red = 100;
						green = 0;
					}
					if ( !(miniFlags & 0x1) ) // Not walkable
					{
						red /= 3;
						green /= 3;
						blue /= 3;
					}

					u32 miniTileYC = yOffset+yMiniTile*(8+miniTileSeparation),
						miniTileXC = xOffset+xMiniTile*(8+miniTileSeparation);

					for ( u32 yc=0; yc<8; yc++ )
					{
						if ( yc + miniTileYC < bitHeight )
						{
							for ( u32 xc=0; xc<8; xc++ )
							{
								if ( xc + miniTileXC < bitWidth )
									bitmap[(yc + miniTileYC)*bitWidth + xc + miniTileXC] = RGB(red, green, blue);
							}
						}
					}
				}
			}
		}
	}
}

void GrpToBits(ChkdBitmap& bitmap, u16 &bitWidth, u16 &bitHeight, s32 &xStart, s32 &yStart,
				GRP* grp, u16 grpXC, u16 grpYC, u16 frame, buffer* palette, u8 color, bool flipped )
{
	if ( frame > grp->numFrames() )
		frame = grp->numFrames()-1;

	u8 frameXOffset = grp->xOffset(frame),
	   frameYOffset = grp->yOffset(frame);

	s16 frameHeight = grp->frameHeight(frame),
		frameWidth = grp->frameWidth(frame),
		line = 0;

	u16 grpHeight = grp->GrpHeight(),
		grpWidth = grp->GrpWidth();

	s32 xOffset = grpXC-xStart-grpWidth/2+frameXOffset,
		yOffset = grpYC-yStart-grpHeight/2+frameYOffset;

	u8* lineDat,
		compSect,
		pos,
		x;

	if ( line + yOffset < 0 ) // Prevent writing before the first line
		line = (s16)(-yOffset);

	if ( yOffset+frameHeight > bitHeight ) // Prevent writing past the last line
		frameHeight -= (s16)yOffset+frameHeight-bitHeight;

	while ( line < frameHeight )
	{
		lineDat = grp->data(frame, line);

		pos = 0;
		x = 0;

		while ( x < frameWidth )
		{
			compSect = lineDat[pos];
			pos++;

			if ( compSect >= 0x80 ) // Skip over transparent pixels
			{
				x += compSect - 0x80;
			}
			else if ( compSect >= 0x40 ) // Place compSect-0x40 pixels of specified color
			{
				for ( u8 i=0; i<compSect-0x40; i++ )
				{
					u32 bitmapIndex;
					if ( flipped )
						bitmapIndex = (line+yOffset)*bitWidth+frameWidth-x+xOffset;
					else
						bitmapIndex = (line+yOffset)*bitWidth+x+xOffset;

					if ( x+xOffset < bitWidth && x+xOffset > 0 )
					{
						if ( lineDat[pos] < 8 )
							bitmap[bitmapIndex] = chkd.scData.tselect.pcxDat.get<u32>(lineDat[pos] + 16 * 4);
						else if ( lineDat[pos] < 16 )
							bitmap[bitmapIndex] = chkd.scData.tselect.pcxDat.get<u32>((color * 8 + lineDat[pos] - 8) * 4);
						else
							bitmap[bitmapIndex] = palette->get<u32>(lineDat[pos] * 4);
					}
					x++;
				}
				pos++;
			}
			else // Place compSect pixels directly from data
			{
				for ( u8 i=0; i<compSect; i++ )
				{
					u32 bitmapIndex = ((u32)line+(u32)yOffset)*(u32)bitWidth+(u32)x+(u32)xOffset;
					if ( x+xOffset < bitWidth && x+xOffset > 0 )
					{
						if ( lineDat[pos] < 8 )
							bitmap[bitmapIndex] = chkd.scData.tselect.pcxDat.get<u32>((lineDat[pos] + 16) * 4);
						else if ( lineDat[pos] < 16 )
							bitmap[bitmapIndex] = chkd.scData.tunit.pcxDat.get<u32>((color * 8 + lineDat[pos] - 8) * 4);
						else
							bitmap[bitmapIndex] = palette->get<u32>(lineDat[pos] * 4);
					}
					x++;
					pos++;
				}
			}
		}
		line ++;
	}
}

void UnitToBits(ChkdBitmap& bitmap, buffer* palette, u8 color, u16 bitWidth, u16 bitHeight,
				 s32 &xStart, s32 &yStart, u16 unitID, u16 unitXC, u16 unitYC,  u16 frame, bool selected )
{
	GRP* curr = &chkd.scData.grps[chkd.scData.SpriteDat(chkd.scData.FlingyDat(chkd.scData.UnitDat(unitID)->Graphics)->Sprite)->SelectionCircleImage+561];
	
	if ( unitID < 228 )
	{
		curr = &chkd.scData.grps[chkd.scData.ImageDat(chkd.scData.SpriteDat(chkd.scData.FlingyDat(chkd.scData.UnitDat(unitID)->Graphics)->Sprite)->ImageFile)->GRPFile-1];
		if ( selected )
		{
			GRP* selCirc = &chkd.scData.grps[chkd.scData.ImageDat(chkd.scData.SpriteDat(chkd.scData.FlingyDat(chkd.scData.UnitDat(unitID)->Graphics)->Sprite)->SelectionCircleImage+561)->GRPFile-1];
			u16 offsetY = unitYC + chkd.scData.SpriteDat(chkd.scData.FlingyDat(chkd.scData.UnitDat(unitID)->Graphics)->Sprite)->SelectionCircleOffset;
			GrpToBits(bitmap, bitWidth, bitHeight, xStart, yStart, selCirc, unitXC, offsetY, frame, palette, 0, false);
		}
	}
	else // Extended unit, use ID:0's graphics (for now)
	{
		curr = &chkd.scData.grps[chkd.scData.ImageDat(chkd.scData.SpriteDat(chkd.scData.FlingyDat(chkd.scData.UnitDat(0)->Graphics)->Sprite)->ImageFile)->GRPFile-1];
		if ( selected )
		{
			GRP* selCirc = &chkd.scData.grps[chkd.scData.ImageDat(chkd.scData.SpriteDat(chkd.scData.FlingyDat(chkd.scData.UnitDat(0)->Graphics)->Sprite)->SelectionCircleImage+561)->GRPFile-1];
			u16 offsetY = unitYC + chkd.scData.SpriteDat(chkd.scData.FlingyDat(chkd.scData.UnitDat(0)->Graphics)->Sprite)->SelectionCircleOffset;
			GrpToBits(bitmap, bitWidth, bitHeight, xStart, yStart, selCirc, unitXC, offsetY, frame, palette, 0, false);
		}
	}

	GrpToBits(bitmap, bitWidth, bitHeight, xStart, yStart, curr, unitXC, unitYC, frame, palette, color, false);
}

void SpriteToBits(ChkdBitmap& bitmap, buffer* palette, u8 color, u16 bitWidth, u16 bitHeight,
				   s32 &xStart, s32 &yStart, u16 spriteID, u16 spriteXC, u16 spriteYC )
{
	GRP* curr = &chkd.scData.grps[chkd.scData.ImageDat(chkd.scData.SpriteDat(spriteID)->ImageFile)->GRPFile-1];
	GrpToBits(bitmap, bitWidth, bitHeight, xStart, yStart, curr, spriteXC, spriteYC, 0, palette, color, false);
}

void TileToBits(ChkdBitmap& bitmap, TileSet* tiles, s32 xStart, s32 yStart, u16 width, u16 height, u16 &TileValue)
{
	u32 cv5Ref, MegaTileRef, MiniTileRef, // Pointers and index's of tile components
		yMiniOffset, xMiniOffset, // Processing optimizers
		yPixel, xPixel; // Bitmap coordinates

	u16 wpeRef; // Pointer to WPE start

	u8 xMiniTile,  yMiniTile ,
	   xMiniPixel, yMiniPixel;
	
	buffer* palette = &tiles->wpe;

	s8 negative; // Simplifies calculating flipped tiles

	if ( GetCV5References(tiles, cv5Ref, TileValue) ) // Get tile CV5 start point for the given MTXM value
	{
		MegaTileRef = GetMegaTileRef(tiles, cv5Ref); // Get tile VX4 start point ('MegaTile') for the given CV5 struct
		
		for ( yMiniTile=0; yMiniTile<4; yMiniTile++ ) // Cycle through the 4 MiniTile rows
		{
			yMiniOffset = yStart + yMiniTile*8; // Calculate the MiniTile's y-contribution once rather than 8 times
			for ( xMiniTile=0; xMiniTile<4; xMiniTile++ ) // Cycle through the 4 mini tile columns
			{
				if ( tiles->vx4.get<u8>(MegaTileRef+2*(4*yMiniTile+xMiniTile)) & 1 ) // If the MiniTile's xPixels are flipped (1st bit)
				{
					xMiniOffset = xStart + xMiniTile*8 + 7; // Calculate the MiniTile's x-contribution once rather than 64 times
					negative = -1;
				}
				else // Not Flipped
				{
					xMiniOffset = xStart + xMiniTile*8; // Calculate the MiniTile's x-contribution once rather than 64 times
					negative =  1;
				}

				MiniTileRef = GetMiniTileRef(tiles, MegaTileRef, xMiniTile, yMiniTile); // Get VR4 start point for the given minitile

				for ( yMiniPixel=0; yMiniPixel<8; yMiniPixel++ ) // Cycle through the 8 mini tile pixel rows
				{
					yPixel = yMiniOffset + yMiniPixel; // The y-position within the current bitmap
					if ( yPixel < height )
					{
						for ( xMiniPixel=0; xMiniPixel<8; xMiniPixel++ ) // Cycle through the 8 mini tile pixel columns
						{
							xPixel = negative*xMiniPixel + xMiniOffset; // The x-position within the current bitmap
							if ( xPixel < width )
							{
								wpeRef = tiles->vr4.get<u8>(MiniTileRef+yMiniPixel*8+xMiniPixel)*4; // WPE start point for the given pixel
								bitmap[yPixel*width + xPixel] = palette->get<u32>(wpeRef);
							}
						}
					}
				}
			}
		}
	}
}

void DrawMiniTileElevation(HDC hDC, TileSet* tiles, s16 xOffset, s16 yOffset, u16 tileValue, u8 miniTileX, u8 miniTileY, BITMAPINFO &bmi)
{
	u32 cv5Ref, MegaTileRef; // Pointers and index's to tile components

	ChkdBitmap graphicBits;
	graphicBits.resize(64);

	if ( GetCV5References(tiles, cv5Ref, tileValue) ) // Get tile CV5 start point for the given MTXM value
	{
		MegaTileRef = GetMegaTileRef(tiles, cv5Ref); // Get tile VX4 start point ('MegaTile') for the given CV5 struct

		u8 miniFlags = 0;
		if ( tiles->vf4.get<u8>(miniFlags, MegaTileRef+2*(4*miniTileY+miniTileX)) )
		{
			u8 red = 0, blue = 0, green = 100;
			if ( miniFlags & 0x2 ) // Mid ground
			{
				blue = 100;
				green = 0;
			}
			if ( miniFlags & 0x4 ) // High ground
			{
				red = 100;
				green = 0;
			}
			if ( !(miniFlags & 0x1) ) // Not walkable
			{
				red /= 3;
				green /= 3;
				blue /= 3;
			}

			for ( u32 yc=0; yc<8; yc++ )
			{
				for ( u32 xc = 0; xc < 8; xc++ )
					graphicBits[yc * 8 + xc] = RGB(red, green, blue);
			}
		}
	}
	SetDIBitsToDevice(hDC, xOffset, yOffset, 8, 8, 0, 0, 0, 8, &graphicBits[0], &bmi, DIB_RGB_COLORS);
}

void DrawTileElevation(HDC hDC, TileSet* tiles, s16 xOffset, s16 yOffset, u16 tileValue, BITMAPINFO &bmi)
{
	u32 cv5Ref, MegaTileRef; // Pointers and index's to tile components

	u8 yMiniTile, xMiniTile;

	ChkdBitmap graphicBits;
	graphicBits.resize(1024);

	if ( GetCV5References(tiles, cv5Ref, tileValue) ) // Get tile CV5 start point for the given MTXM value
	{
		MegaTileRef = GetMegaTileRef(tiles, cv5Ref); // Get tile VX4 start point ('MegaTile') for the given CV5 struct

		for ( yMiniTile=0; yMiniTile<4; yMiniTile++ ) // Cycle through the 4 mini tile rows
		{
			for ( xMiniTile=0; xMiniTile<4; xMiniTile++ ) // Cycle through the 4 mini tile columns
			{
				u8 miniFlags = 0;
				if ( tiles->vf4.get<u8>(miniFlags, MegaTileRef+2*(4*yMiniTile+xMiniTile)) )
				{
					u8 red = 0, blue = 0, green = 100;
					if ( miniFlags & 0x2 ) // Mid ground
					{
						blue = 100;
						green = 0;
					}
					if ( miniFlags & 0x4 ) // High ground
					{
						red = 100;
						green = 0;
					}
					if ( !(miniFlags & 0x1) ) // Not walkable
					{
						red /= 3;
						green /= 3;
						blue /= 3;
					}

					u32 miniTileYC = yMiniTile*(8),
						  miniTileXC = xMiniTile*(8);

					for ( u32 yc=0; yc<8; yc++ )
					{
						for ( u32 xc = 0; xc < 8; xc++ )
							graphicBits[(yc + miniTileYC) * 32 + xc + miniTileXC] = RGB(red, green, blue);
					}
				}
			}
		}
	}
	SetDIBitsToDevice(hDC, xOffset, yOffset, 32, 32, 0, 0, 0, 32, &graphicBits[0], &bmi, DIB_RGB_COLORS);
}

void DrawTile(HDC hDC, TileSet* tiles, s16 xOffset, s16 yOffset, u16 &TileValue, BITMAPINFO &bmi, u8 blueOffset, u8 greenOffset, u8 redOffset)
{
	u32 cv5Ref, MegaTileRef, MiniTileRef; // Pointers and index's to tile components

	u16 MiniTileOffset, yMiniOffset, // Processing optimizers
		wpeRef; // Pointer to WPE start

	u8 yMiniTile,  xMiniTile ,
	   yMiniPixel, xMiniPixel;

	s8 negative; // Simplifies calculating flipped tiles

	if ( GetCV5References(tiles, cv5Ref, TileValue) ) // Get tile CV5 start point for the given MTXM value
	{
		ChkdBitmap graphicBits;
		graphicBits.resize(1024);
		MegaTileRef = GetMegaTileRef(tiles, cv5Ref); // Get tile VX4 start point ('MegaTile') for the given CV5 struct

		for ( yMiniTile=0; yMiniTile<4; yMiniTile++ ) // Cycle through the 4 mini tile rows
		{
			yMiniOffset = yMiniTile*32;
			for ( xMiniTile=0; xMiniTile<4; xMiniTile++ ) // Cycle through the 4 mini tile columns
			{
				if ( tiles->vx4.get<u8>(MegaTileRef+2*(4*yMiniTile+xMiniTile)) & 1 ) // If the MiniTile's xPixels are flipped (1st bit)
				{
					MiniTileOffset = (yMiniOffset + xMiniTile)*8 + 7; // Simplified: (yMiniTile*32*8 + xMiniTile*8 + 7*flipped)
					negative = -1;
				}
				else
				{
					MiniTileOffset = (yMiniOffset + xMiniTile)*8; // Simplified: (yMiniTile*32*8 + xMiniTile*8)
					negative =  1;
				}

				MiniTileRef = GetMiniTileRef(tiles, MegaTileRef, xMiniTile, yMiniTile); // Get VR4 start point for the given minitile

				for ( yMiniPixel=0; yMiniPixel<8; yMiniPixel++ ) // Cycle through the 8 mini tile pixel rows
				{
					for ( xMiniPixel=0; xMiniPixel<8; xMiniPixel++ ) // Cycle through the 8 mini tile pixel columns
					{
						wpeRef = tiles->vr4.get<u8>((MiniTileRef+yMiniPixel*8+xMiniPixel))*4; // WPE start point for the given pixel
						graphicBits[(yMiniPixel * 32 + negative*xMiniPixel) + MiniTileOffset] =
							AdjustPx(tiles->wpe.get<u32>(wpeRef), redOffset, greenOffset, blueOffset);
					}
				}
			}
		}
		SetDIBitsToDevice(hDC, xOffset, yOffset, 32, 32, 0, 0, 0, 32, &graphicBits[0], &bmi, DIB_RGB_COLORS);
	}
}

void DrawTools( HDC hDC, HBITMAP bitmap, u16 width, u16 height, u32 screenLeft, u32 screenTop,
				SELECTIONS& selections, bool pasting, CLIPBOARD& clipboard, Scenario &chk, u8 layer )
{
	if ( layer == LAYER_TERRAIN && selections.hasTiles() ) // Draw selected tiles
		DrawTileSel(hDC, width, height, screenLeft, screenTop, selections, chk);
	else if ( layer == LAYER_LOCATIONS ) // Draw Location Creation/Movement Graphics
		DrawTempLocs(hDC, screenLeft, screenTop, selections, chk);

	if ( pasting ) // Draw paste graphics
		DrawPasteGraphics(hDC, bitmap, width, height, screenLeft, screenTop, selections, clipboard, chk, layer);
}

void DrawTileSel(HDC hDC, u16 width, u16 height, u32 screenLeft, u32 screenTop, SELECTIONS& selections, Scenario &chk)
{
	HPEN pen = CreatePen(PS_SOLID, 0, RGB(0, 255, 255));
	SelectObject(hDC, pen);
	RECT rect, rcBorder;

	u16 tilesetNum;
	if ( !chk.ERA().get<u16>(tilesetNum, 0) ) return;
	TileSet* tileset = &chkd.scData.tilesets.set[tilesetNum];

	BITMAPINFO bmi = GetBMI(32, 32);
	rcBorder.left	= (0	  + screenLeft)/32 - 1;
	rcBorder.right	= (width  + screenLeft)/32 + 1;
	rcBorder.top	= (0	  + screenTop)/32 - 1;
	rcBorder.bottom	= (height + screenTop)/32 + 1;

	std::vector<TileNode> &tiles = selections.getTiles();
	for ( auto &tile : tiles )
	{
		if ( tile.xc > rcBorder.left &&
			 tile.xc < rcBorder.right &&
			 tile.yc > rcBorder.top &&
			 tile.yc < rcBorder.bottom )
		// If tile is within current map border
		{
			// Draw tile with a blue haze, might replace with blending
				DrawTile( hDC,
						  tileset,
						  s16(tile.xc*32-screenLeft),
						  s16(tile.yc*32-screenTop),
						  tile.value,
						  bmi,
						  0,
						  0,
						  32
						);

			if ( tile.neighbors > 0 ) // if any edges need to be drawn
			{
				rect.left	= tile.xc*32 - screenLeft;
				rect.right	= tile.xc*32 - screenLeft + 32;
				rect.top	= tile.yc*32 - screenTop;
				rect.bottom = tile.yc*32 - screenTop + 32;

				if ( tile.neighbors & NEIGHBOR_TOP )
				{
					MoveToEx(hDC, rect.left , rect.top, NULL);
					LineTo	(hDC, rect.right, rect.top		);
				}
				if ( tile.neighbors & NEIGHBOR_RIGHT )
				{
					if ( rect.right >= width )
						rect.right --;

					MoveToEx(hDC, rect.right, rect.top	   , NULL);
					LineTo	(hDC, rect.right, rect.bottom+1		 );
				}
				if ( tile.neighbors & NEIGHBOR_BOTTOM )
				{
					if ( rect.bottom >= height )
						rect.bottom --;

					MoveToEx(hDC, rect.left	, rect.bottom, NULL);
					LineTo	(hDC, rect.right, rect.bottom	   );
				}
				if ( tile.neighbors & NEIGHBOR_LEFT )
				{
					MoveToEx(hDC, rect.left, rect.bottom, NULL);
					LineTo	(hDC, rect.left, rect.top-1		  );
				}
			}
		}
	}
}

void DrawPasteGraphics( HDC hDC, HBITMAP bitmap, u16 width, u16 height, u32 screenLeft, u32 screenTop,
						SELECTIONS& selections, CLIPBOARD& clipboard, Scenario &chk, u8 layer )
{
	if ( layer == LAYER_TERRAIN )
	{
		HPEN pen = CreatePen(PS_SOLID, 0, RGB(0, 255, 255));
		SelectObject(hDC, pen);
	
		RECT rect, rcShade;
		u16 tileset;
		if ( !chk.ERA().get<u16>(tileset, 0) ) return;
		TileSet* tiles = &chkd.scData.tilesets.set[tileset];
	
		BITMAPINFO bmi = GetBMI(32, 32);
		rcShade.left   = -32;
		rcShade.right  = width  + 32;
		rcShade.top	   = -32;
		rcShade.bottom = height + 32;
	
		POINT center;
		center.x = selections.getEndDrag().x + 16;
		center.y = selections.getEndDrag().y + 16;
	
		std::vector<PasteTileNode> &pasteTiles = clipboard.getTiles();
		for ( auto &tile : pasteTiles )
		{
			rect.left	= ( tile.xc + center.x	  )/32*32 - screenLeft;
			rect.top	= ( tile.yc + center.y	  )/32*32 - screenTop;
			rect.right	= ( tile.xc + 32 + center.x )/32*32 - screenLeft;
			rect.bottom = ( tile.yc + 32 + center.y )/32*32 - screenTop;

			if ( rect.left > rcShade.left && rect.left < rcShade.right && rect.top > rcShade.top && rect.top < rcShade.bottom )
			// If tile is within current map border
			{
				// Draw tile with a blue haze
					DrawTile(hDC, tiles, (s16)rect.left, (s16)rect.top, tile.value, bmi, 32, 0, 0);

				if ( tile.neighbors ) // if any edges need to be drawn
				{
					if ( tile.neighbors & NEIGHBOR_TOP )
					{
						MoveToEx(hDC, rect.left	, rect.top, NULL);
						LineTo	(hDC, rect.right, rect.top		);
					}
					if ( tile.neighbors & NEIGHBOR_RIGHT )
					{
						if ( rect.right >= width )
							rect.right --;
	
						MoveToEx(hDC, rect.right, rect.top, NULL);
						LineTo	(hDC, rect.right, rect.bottom+1	);
					}
					if ( tile.neighbors & NEIGHBOR_BOTTOM )
					{
						if ( rect.bottom >= height )
							rect.bottom --;

						MoveToEx(hDC, rect.left	, rect.bottom, NULL);
						LineTo	(hDC, rect.right, rect.bottom	   );
					}
					if ( tile.neighbors & NEIGHBOR_LEFT )
					{
						MoveToEx(hDC, rect.left, rect.bottom, NULL);
						LineTo	(hDC, rect.left, rect.top-1		  );
					}
				}
			}
		}
	}
	else if ( layer == LAYER_UNITS )
	{
		ChkdBitmap graphicBits;
		graphicBits.resize(((u32)width)*((u32)height));

		BITMAPINFO bmi = GetBMI(width, height);
		GetDIBits(hDC, bitmap, 0, height, &graphicBits[0], &bmi, DIB_RGB_COLORS);

		u8 tileset;
		if ( !chk.ERA().get<u8>(tileset, 0) ) return; // Map must have a tileset

		buffer* palette = &chkd.scData.tilesets.set[tileset].wpe;
		s32 sScreenLeft = screenLeft;
		s32 sScreenTop = screenTop;

		POINT cursor = selections.getEndDrag();
		if ( cursor.x != -1 && cursor.y != -1 )
		{
			std::vector<PasteUnitNode> units = clipboard.getUnits();
			for ( auto &pasteUnit : units )
			{
				u8 color = pasteUnit.unit.owner;
				chk.getPlayerColor(pasteUnit.unit.owner, color);
				if ( cursor.y+ pasteUnit.yc >= 0 )
				{
					UnitToBits(graphicBits, palette, color, width, height, sScreenLeft, sScreenTop,
						pasteUnit.unit.id, u16(cursor.x+ pasteUnit.xc), u16(cursor.y+ pasteUnit.yc), 0, false );
				}
			}
		}

		SetDIBitsToDevice( hDC, 0, 0, width, height, 0, 0, 0,
						   height, &graphicBits[0], &bmi, DIB_RGB_COLORS);
	}
}

void DrawTempLocs(HDC hDC, u32 screenLeft, u32 screenTop, SELECTIONS& selections, Scenario &chk)
{
	POINT start = selections.getStartDrag();
	POINT end = selections.getEndDrag();
	if ( selections.getLocationFlags() == LOC_SEL_NOTHING ) // Draw location creation preview
	{
		s32 left = start.x-screenLeft,
			top = start.y-screenTop,
			right = end.x-screenLeft,
			bottom = end.y-screenTop;
		DrawLocationFrame(hDC, start.x-screenLeft, start.y-screenTop, end.x-screenLeft, end.y-screenTop);
	}
	else
	{
		ChkLocation* loc;
		u16 selectedLocation = selections.getSelectedLocation();
		if ( selectedLocation != NO_LOCATION && chk.getLocation(loc, selectedLocation) ) // Draw location resize/movement graphics
		{
			s32 locLeft = loc->xc1-screenLeft;
			s32 locRight = loc->xc2-screenLeft;
			s32 locTop = loc->yc1-screenTop;
			s32 locBottom = loc->yc2-screenTop;
			s32 dragX = end.x-start.x;
			s32 dragY = end.y-start.y;

			u8 locFlags = selections.getLocationFlags();
			if ( locFlags != LOC_SEL_MIDDLE )
			{
				if ( locTop > locBottom )
				{
					if ( locFlags & LOC_SEL_NORTH )
						locFlags = locFlags&(~LOC_SEL_NORTH)|LOC_SEL_SOUTH;
					else if ( locFlags & LOC_SEL_SOUTH )
						locFlags = locFlags&(~LOC_SEL_SOUTH)|LOC_SEL_NORTH;
				}

				if ( locLeft > locRight )
				{
					if ( locFlags & LOC_SEL_WEST )
						locFlags = locFlags&(~LOC_SEL_WEST)|LOC_SEL_EAST;
					else if ( locFlags & LOC_SEL_EAST )
						locFlags = locFlags&(~LOC_SEL_EAST)|LOC_SEL_WEST;
				}
			}

			switch ( locFlags )
			{
				case LOC_SEL_NORTH : DrawLocationFrame(hDC, locLeft, locTop+dragY, locRight, locBottom); break;
				case LOC_SEL_SOUTH : DrawLocationFrame(hDC, locLeft, locTop, locRight, locBottom+dragY); break;
				case LOC_SEL_EAST  : DrawLocationFrame(hDC, locLeft, locTop, locRight+dragX, locBottom); break;
				case LOC_SEL_WEST  : DrawLocationFrame(hDC, locLeft+dragX, locTop, locRight, locBottom); break;
				case LOC_SEL_NW	   : DrawLocationFrame(hDC, locLeft+dragX, locTop+dragY, locRight, locBottom); break;
				case LOC_SEL_NE	   : DrawLocationFrame(hDC, locLeft, locTop+dragY, locRight+dragX, locBottom); break;
				case LOC_SEL_SW	   : DrawLocationFrame(hDC, locLeft+dragX, locTop, locRight, locBottom+dragY); break;
				case LOC_SEL_SE	   : DrawLocationFrame(hDC, locLeft, locTop, locRight+dragX, locBottom+dragY); break;
				case LOC_SEL_MIDDLE:
					DrawLocationFrame(hDC, locLeft+dragX, locTop+dragY, locRight+dragX, locBottom+dragY);
					break;
			}
		}
	}
}

void DrawSelectingFrame(HDC hDC, SELECTIONS& selections, u32 screenLeft, u32 screenTop, s32 width,  s32 height, double scale)
{
	if ( !selections.selectionAreaIsNull() )
	{
		POINT startDrag = selections.getStartDrag(),
			  endDrag = selections.getEndDrag();

		RECT rect;
		rect.left	= s32(((s32)startDrag.x-(s32)screenLeft)/**scale*/)  ;
		rect.right	= s32(((s32)  endDrag.x-(s32)screenLeft)/**scale*/)+1;
		rect.top	= s32(((s32)startDrag.y-(s32)screenTop )/**scale*/)  ;
		rect.bottom	= s32(((s32)  endDrag.y-(s32)screenTop )/**scale*/)+1;
	
		if ( rect.right < rect.left )
		{
			s32 xSave	= rect.left		;
			rect.left	= rect.right - 1;
			rect.right	= xSave		 + 1; 
		}
		if ( rect.bottom < rect.top )
		{
			s32 ySave	= rect.top		 ;
			rect.top	= rect.bottom - 1;
			rect.bottom	= ySave		  + 1;
		}
		if ( rect.right >= width )
			rect.right --;
		if ( rect.bottom >= height )
			rect.bottom --;

		HBRUSH brush = CreateSolidBrush(RGB(255, 0, 0));
		FrameRect(hDC, &rect, brush);
		DeleteObject(brush);
	}
}

void DrawLocationFrame(HDC hDC, s32 left, s32 top, s32 right, s32 bottom)
{
	HBRUSH brush = CreateSolidBrush(RGB(255, 0, 0));

	RECT rect;
	if ( left < right ) {
		rect.left = left;
		rect.right = right;
	} else {
		rect.left = right;
		rect.right = left;
	} if ( top < bottom ) {
		rect.top = top;
		rect.bottom = bottom;
	} else {
		rect.top = bottom;
		rect.bottom = top;
	}

	FrameRect(hDC, &rect, brush);

	rect.left --;
	rect.top --;
	rect.right ++;
	rect.bottom ++;

	FrameRect(hDC, &rect, brush);

	DeleteObject(brush);
}

void DrawMiniMapTiles(ChkdBitmap& bitmap, u16 bitWidth, u16 bitHeight, u16 xSize, u16 ySize,
					   u16 xOffset, u16 yOffset, float scale, TileSet* tiles, Scenario &chk )
{
	buffer& MTXM = chk.MTXM();

	// minimap colors are equivilant to pixel 7, 6 of the minitile given by (MiniMapCoord%scale)
	u32 screenWidth  = bitWidth,
		screenHeight = bitHeight,
		mtxmReference,
		cv5Reference,
		MegaTileReference, MiniTileRef;

	u8 xMiniTile = 0, yMiniTile = 0;

	u16 TileValue,
		yTile, xTile,
		yc, xc,
		wpeRef;

	for ( yc=0; yc<128-2*yOffset; yc++ ) // Cycle through all minimap pixel rows
	{
		yTile = (u16)(yc/scale); // Get the yc of tile used for the pixel
		if ( scale > 1 )
		{
			yMiniTile = yc%(int)scale; // Get the y-minitile used for the pixel
			if ( yMiniTile > 3 )
				yMiniTile %= 4; // Correct for invalid y-minitiles
		}
	
		for ( xc=0; xc<128-2*xOffset; xc++ ) // Cycle through all minimap pixel columns
		{
			xTile = (u16)(xc/scale); // Get the xc of the tile used for the pixel
			if ( scale > 1 )
			{
				xMiniTile = xc%(int)scale; // Get the x-minitile used for the pixel
				if ( xMiniTile > 3 )
					xMiniTile %= 4; // Correct for invalid x-minitiles
			}
	
			mtxmReference = 2*(xSize*yTile+xTile);
	
			if ( MTXM.get<u16>(TileValue, mtxmReference) )
			{
				if ( GetCV5References(tiles, cv5Reference, TileValue) ) // Get tile CV5 start point for the given MTXM value
				{
					MegaTileReference = GetMegaTileRef(tiles, cv5Reference); // Get tile VX4 start point ('MegaTile') for the given CV5 struct
					MiniTileRef = GetMiniTileRef(tiles, MegaTileReference, xMiniTile, yMiniTile); // Get VR4 start point for the given minitile
					wpeRef = 4 * tiles->vr4.get<u8>(MiniTileRef+6*8+7); // WPE start point for the given pixel
					bitmap[(yc + yOffset) * 128 + xc + xOffset] = tiles->wpe.get<u32>(wpeRef);
				}
			}
		}
	}
}

#define MINI_MAP_MAXBIT 65536 // Maximum graphicBits position

void DrawMiniMapUnits(ChkdBitmap& bitmap, u16 bitWidth, u16 bitHeight, u16 xSize, u16 ySize,
					   u16 xOffset, u16 yOffset, float scale, TileSet* tiles, Scenario &chk )
{
	buffer& UNIT = chk.UNIT(),
		  & THG2 = chk.THG2(),
		  & COLR = chk.COLR();

	u32 unitTableSize = UNIT.size();
	u32 spriteTableSize = THG2.size();
	ChkUnit* unit;
	for ( u32 pos = 0; pos < unitTableSize; pos += UNIT_STRUCT_SIZE )
	{
		if ( UNIT.getPtr(unit, pos, UNIT_STRUCT_SIZE) )
		{
			u8 color;
			if ( unit->owner < 8 )
				color = COLR.get<u8>(unit->owner);
			else
				color = unit->owner;

			u32 bitIndex = (
								((u32)((unit->yc/32)*scale)+yOffset)*128
								+ (u32)((unit->xc/32)*scale)+xOffset
						   );

			if ( bitIndex < MINI_MAP_MAXBIT )
				bitmap[bitIndex] = chkd.scData.tminimap.pcxDat.get<u32>(color * 4);
		}
	}

	for ( u32 pos = 0; pos < spriteTableSize; pos += SPRITE_STRUCT_SIZE )
	{
		ChkSprite* sprite;
		if ( THG2.getPtr(sprite, pos, SPRITE_STRUCT_SIZE) )
		{
			if ( sprite->flags & FLAG_DRAW_AS_SPRITE )
			{
				u8 color;
				if ( sprite->owner < 8 )
					color = COLR.get<u8>(sprite->owner);
				else
					color = sprite->owner;

				u32 bitIndex = (((u32)((sprite->yc/32)*scale)+yOffset)*128
								 + (u32)((sprite->xc/32)*scale)+xOffset);

				if ( bitIndex < MINI_MAP_MAXBIT )
					bitmap[bitIndex] = chkd.scData.tminimap.pcxDat.get<u32>(color * 4);
			}
		}
	}
}

void DrawMiniMap(HDC hDC, HWND hWnd, u16 xSize, u16 ySize, float scale, Scenario &chk)
{
	buffer& ERA  = chk.ERA ();

	ChkdBitmap graphicBits;
	graphicBits.resize(65536);

	u16 tileset;

	BITMAPINFO bmi = GetBMI(128, 128);

	if ( !ERA.get<u16>(tileset, 0) ) return; // Map must have a tileset

	TileSet* tiles = nullptr;		

	u16 xOffset = (u16)((128-xSize*scale)/2),
		yOffset = (u16)((128-ySize*scale)/2);

	tiles = &chkd.scData.tilesets.set[tileset];
	DrawMiniMapTiles(graphicBits, 128, 128, xSize, ySize, xOffset, yOffset, scale, tiles, chk);
	DrawMiniMapUnits(graphicBits, 128, 128, xSize, ySize, xOffset, yOffset, scale, tiles, chk);
	SetDIBitsToDevice(hDC, xOffset, yOffset, 128-2*xOffset, 128-2*yOffset, xOffset, yOffset, 0, 128, &graphicBits[0], &bmi, DIB_RGB_COLORS);

	// Draw Map Borders

	HPEN pen = CreatePen(PS_SOLID, 0, RGB(255, 255, 255));
	SelectObject(hDC, pen);

	MoveToEx(hDC, xOffset-1, yOffset-1, NULL);
	LineTo(hDC, 128-xOffset, yOffset-1);
	LineTo(hDC, 128-xOffset, 128-yOffset);
	LineTo(hDC, xOffset-1, 128-yOffset);
	LineTo(hDC, xOffset-1, yOffset-1);
}

void DrawMiniMapBox(HDC hDC, u32 screenLeft, u32 screenTop, u16 screenWidth, u16 screenHeight, u16 xSize, u16 ySize, float scale)
{
	HPEN pen = CreatePen(PS_SOLID, 0, RGB(255, 255, 255));

	u16 xOffset = (u16)((128-xSize*scale)/2),
		yOffset = (u16)((128-ySize*scale)/2);

	// Draw Current Area Box
	u16 X1 = (u16)((screenLeft+16)/32*scale + xOffset),
		Y1 = (u16)((screenTop +16)/32*scale + yOffset),

		X2 = (u16)(((screenLeft+16)/32
			+ ((screenWidth - 16)/32)+1)*scale + xOffset),

		Y2 = (u16)(((screenTop +16)/32
			+ ((screenHeight - 16)/32)+1)*scale + yOffset);

	if ( X2 > 127-xOffset )
		X2 = 127-xOffset;

	if ( Y2 > 127-yOffset )
		Y2 = 127-yOffset;

	SelectObject(hDC, pen);

	MoveToEx(hDC, X1, Y1, NULL);
	LineTo(hDC, X2, Y1);
	LineTo(hDC, X2, Y2);
	LineTo(hDC, X1, Y2);
	LineTo(hDC, X1, Y1);

	DeleteObject(pen);
}

UINT GetStringDrawWidth(HDC hDC, std::string str)
{
	return LOWORD(GetTabbedTextExtent(hDC, (LPCSTR)str.c_str(), str.size(), 0, NULL));
}

bool GetLineDrawSize(HDC hDC, SIZE* strSize, std::string str)
{
	DWORD result = GetTabbedTextExtent(hDC, (LPCSTR)str.c_str(), str.size(), 0, NULL);
	if ( result != 0 )
	{
		strSize->cx = LOWORD(result);
		strSize->cy = HIWORD(result);
		return true;
	}
	else
		return false;
}

void DrawStringChunk(HDC hDC, UINT xPos, UINT yPos, std::string str)
{
	TabbedTextOut(hDC, xPos, yPos, (LPCSTR)str.c_str(), str.size(), 0, NULL, 0);
}

void DrawStringLine(HDC hDC, UINT xPos, UINT yPos, LONG width, COLORREF defaultColor, std::string str)
{
	COLORREF lastColor = defaultColor;
	const char* cStr = str.c_str();
	int size = str.size();
	int chunkStartChar = 0;
	bool center = false,
		 right = false;
	for ( int i=0; i<size; i++ )
	{
		if ( u8(cStr[i]) < 32 && cStr[i] != '\11' ) // Not tab, must be color or alignment
		{
			if ( i > chunkStartChar ) // Output everything prior to this...
			{
				std::string chunk = str.substr(chunkStartChar, i-chunkStartChar);
				if ( center )
				{
					UINT chunkWidth, chunkHeight;
					if ( GetStringDrawSize(hDC, chunkWidth, chunkHeight, chunk) )
					{
						xPos = UINT(width)/2-chunkWidth/2;
						if ( xPos >= UINT(width) )
							xPos = 0;
					}
				}
				else if ( right )
				{
					UINT chunkWidth, chunkHeight;
					if ( GetStringDrawSize(hDC, chunkWidth, chunkHeight, chunk) )
					{
						xPos = UINT(width)-chunkWidth-1;
						if ( xPos >= UINT(width) )
							xPos = 0;
					}
				}
				DrawStringChunk(hDC, xPos, yPos, chunk); // Output everything before the color/alignment
				xPos += GetStringDrawWidth(hDC, chunk);
				chunkStartChar = i+1;
			}
			else if ( i == chunkStartChar )
				chunkStartChar ++;

			if ( cStr[i] < '\x20' ) // Possible special character, change color/alignment
			{
				switch ( ((u8)cStr[i]) )
				{
					case 0x01: lastColor = RGB(184, 184, 232); break;
					case 0x02: lastColor = RGB(184, 184, 232); break;
					case 0x03: lastColor = RGB(220, 220,  60); break;
					case 0x04: lastColor = RGB(255, 255, 255); break;
					case 0x05: lastColor = RGB(132, 116, 116); break;
					case 0x06: lastColor = RGB(200,  24,  24); break;
					case 0x07: lastColor = RGB( 16, 252,  24); break;
					case 0x08: lastColor = RGB(244,   4,   4); break;
					case 0x0B: lastColor = defaultColor; break;
					case 0x0C: lastColor = defaultColor; break;
					case 0x0E: lastColor = RGB( 12,  72, 204); break;
					case 0x0F: lastColor = RGB( 44, 180, 148); break;
					case 0x10: lastColor = RGB(136,  64, 156); break;
					case 0x11: lastColor = RGB(248, 140,  20); break;
					case 0x12: right = true; center = false; break;
					case 0x13: if ( !right ) center = true; break;
					case 0x14: lastColor = defaultColor; break;
					case 0x15: lastColor = RGB(112,  48,  20); break;
					case 0x16: lastColor = RGB(204, 224, 208); break;
					case 0x17: lastColor = RGB(252, 252,  56); break;
					case 0x18: lastColor = RGB(  8, 128,   8); break;
					case 0x19: lastColor = RGB(252, 252, 124); break;
					case 0x1A: lastColor = RGB(184, 184, 232); break;
					case 0x1B: lastColor = RGB(236, 196, 176); break;
					case 0x1C: lastColor = RGB( 64, 104, 212); break;
					case 0x1D: lastColor = RGB(116, 164, 124); break;
					case 0x1E: lastColor = RGB(144, 144, 184); break;
					case 0x1F: lastColor = RGB(  0, 228, 252); break;
				}
				SetTextColor(hDC, lastColor);
			}
		}
	}

	if ( chunkStartChar < size )
	{
		std::string chunk = str.substr(chunkStartChar, size-chunkStartChar);
		if ( center )
		{
			UINT chunkWidth, chunkHeight;
			if ( GetStringDrawSize(hDC, chunkWidth, chunkHeight, chunk) )
			{
				xPos = UINT(width)/2-chunkWidth/2;
				if ( xPos >= UINT(width) )
					xPos = 0;
			}
		}
		else if ( right )
		{
			UINT chunkWidth, chunkHeight;
			if ( GetStringDrawSize(hDC, chunkWidth, chunkHeight, chunk) )
			{
				xPos = (UINT(width))-chunkWidth-1;
				if ( (LONG(xPos)) >= width )
					xPos = 0;
			}
		}
		DrawStringChunk(hDC, xPos, yPos, chunk);
	}
}

bool GetStringDrawSize(HDC hDC, UINT &width, UINT &height, std::string str)
{
	// This method is very time sensative and should be optimized as much as possible
	width = 0;
	height = 0;

	SIZE strSize;
		height = 0;

	size_t start = 0,
		   loc,
		   max = str.size(),
		   npos = std::string::npos;

	loc = str.find("\r\n");
	if ( loc != npos ) // Has new lines
	{
		// Do first line
		std::string firstLine = str.substr(0, loc);
		if ( GetLineDrawSize(hDC, &strSize, firstLine) )
		{
			start = loc+2;
			height += strSize.cy;
			if ( strSize.cx > LONG(width) )
				width = UINT(strSize.cx);

			do // Do subsequent lines
			{
				loc = str.find("\r\n", start);
				if ( loc != npos )
				{
					std::string line = " ";
					if ( loc-start > 0 )
						line = str.substr(start, loc-start);
					start = loc+2;
					if ( GetLineDrawSize(hDC, &strSize, line) )
					{
						height += strSize.cy;
						if ( strSize.cx > LONG(width) )
							width = UINT(strSize.cx);
					}
					else
						break;
				}
				else
				{
					// Do last line
					std::string lastLine = str.substr(start, max-start);
					if ( GetLineDrawSize(hDC, &strSize, lastLine) )
					{
						height += strSize.cy;
						if ( strSize.cx > LONG(width) )
							width = UINT(strSize.cx);
					}
					break;
				}
			} while ( start < max );

			return true;
		}
	}
	else if ( GetLineDrawSize(hDC, &strSize, str) )
	{
		height += strSize.cy;
		if ( strSize.cx > LONG(width) )
			width = UINT(strSize.cx);

		return true;
	}
	return false;
}

void DrawString(HDC hDC, UINT xPos, UINT yPos, LONG width, COLORREF defaultColor, std::string str)
{
	SetTextColor(hDC, defaultColor);
	SIZE strSize;
	HGDIOBJ sel = SelectObject(hDC, defaultFont);
	if ( sel != NULL && sel != HGDI_ERROR )
	{
		size_t start = 0,
			   loc,
			   max = str.size(),
			   npos = std::string::npos;

		loc = str.find("\r\n");
		if ( loc != npos ) // Has new lines
		{
			// Do first line
			std::string firstLine = str.substr(0, loc);
			DrawStringLine(hDC, xPos, yPos, width, defaultColor, firstLine);
			if ( GetLineDrawSize(hDC, &strSize, firstLine) )
			{
				start = loc+2;
				yPos += strSize.cy;

				do // Do subsequent lines
				{
					loc = str.find("\r\n", start);
					if ( loc != npos )
					{
						std::string line = " ";
						if ( loc-start > 0 )
						{
							line = str.substr(start, loc-start);
							DrawStringLine(hDC, xPos, yPos, width, defaultColor, line);
						}

						start = loc+2;
						if ( GetLineDrawSize(hDC, &strSize, line) )
							yPos += strSize.cy;
						else
							break;
					}
					else
					{
						// Do last line
						std::string lastLine = str.substr(start, max-start);
						if ( GetLineDrawSize(hDC, &strSize, lastLine) )
						{
							DrawStringLine(hDC, xPos, yPos, width, defaultColor, lastLine);
							yPos += strSize.cy;
						}
						break;
					}
				} while ( start < max );
			}
		}
		else if ( GetLineDrawSize(hDC, &strSize, str) )
		{
			DrawStringLine(hDC, xPos, yPos, width, defaultColor, str);
			yPos += strSize.cy;
		}
	}
}
