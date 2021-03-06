#include "MainPlot.h"
#include "Chkdraft.h"

bool MainPlot::CreateThis(HWND hParent)
{
	if ( ClassWindow::RegisterWindowClass(0, NULL, LoadCursor(NULL, IDC_ARROW), (HBRUSH)(COLOR_APPWORKSPACE + 1), NULL, "Plot", NULL, false) &&
		ClassWindow::CreateClassWindow(WS_EX_TRANSPARENT, NULL, WS_CHILD | WS_VISIBLE, 0, 0, 162, 500, hParent, (HMENU)IDR_MAIN_PLOT) )
	{
		return true;
	}
	return false;
}

LRESULT MainPlot::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch ( msg )
	{
		case WM_CREATE:
			leftBar.CreateThis(hWnd);
			break;

		case WM_ERASEBKGND:
			if ( chkd.maps.curr == nullptr )
				return ClassWindow::WndProc(hWnd, msg, wParam, lParam);
			else
				return TRUE; // Prevent plot from drawing a background over maps
			break;

		default:
			return ClassWindow::WndProc(hWnd, msg, wParam, lParam);
			break;
	}
	return 0;
}
