#include "pch.h"
#include "XCustomSearchband.h"

#define VF_SEARCHBAND_CLSNAME L"BYUNGHO_SEARCHBOX"

typedef struct _XCUSTOMSEARCHBAND {
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwFlagsSet;
	DWORD wEditBoxHeight;
	WORD wEditYLocation;
	WORD wMainYLocation;
	FLOAT dpi;
	HWND hTargetHwnd;
	HWND hSearchBox;
	HWND hSearchEditBox;
	HWND hOriginalBox;
	HANDLE hWatchSizeThread;
	HFONT hEditboxFont;
	LPWSTR pPlaceHolderTextW;
	WNDPROC oldEditProc;
	CCUSTOMSEARCHCALLBACK callback;
	PVOID userData;
} XCUSTOMSEARCHBAND, *PXCUSTOMSEARCHBAND;


//Internal Function

//FLOAT XGetCurrentWindowDPI(HWND hwnd);
BOOL XFindSearchband(HWND hwnd, LPARAM lParam);

LRESULT CALLBACK SearchbarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SearchbarEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

DWORD CheckParentWindowSize(LPVOID args);

BOOL XCusumSearchbandEnabled(HWND hwnd);
BOOL XFindCustomSearchBand(HWND hwnd, LPARAM lParam);

void VFSetPlaceholderTextW(HWND hwnd, LPCWSTR text);
void VFUnsetPlaceholderTextW(HWND hwnd);

void SetParentLocation(PRECT pParentRect,PXCUSTOMSEARCHBAND info);
void SetEditboxLocation(DWORD parentHeight,PXCUSTOMSEARCHBAND info);

__declspec(dllexport) PVOID VFInitializeCustomSearchBand(HWND targetHwnd, CCUSTOMSEARCHCALLBACK callback, PVOID userData, BOOL findSearchBand) {
	BOOL re = FALSE;
	PXCUSTOMSEARCHBAND searchInfo = (PXCUSTOMSEARCHBAND)malloc(sizeof(XCUSTOMSEARCHBAND));
	if (!searchInfo) {
		goto escapeArea;
	}

	ZeroMemory(searchInfo, sizeof(XCUSTOMSEARCHBAND));

	searchInfo->dwSize = sizeof(XCUSTOMSEARCHBAND);
	searchInfo->dwFlags = 0;
	//searchInfo->hSearchboxM = LoadLibraryW(L"Msftedit.dll");
	searchInfo->callback = callback;
	searchInfo->userData = userData;
	searchInfo->dpi = 1.0f;

	if (findSearchBand) {
		EnumChildWindows(targetHwnd, (WNDENUMPROC)XFindSearchband, (LPARAM)searchInfo);
	}
	else {
		searchInfo->hTargetHwnd = targetHwnd;
	}

	if (!searchInfo->hTargetHwnd) {
		goto escapeArea;
	}

	re = TRUE;

escapeArea:

	if (!re) {
		VFCleanSearchboxInfo(searchInfo);
		searchInfo = NULL;
	}

	return searchInfo;
}

__declspec(dllexport) BOOL VFShowCustomSearchBand(PVOID searchboxInfo) {
	PXCUSTOMSEARCHBAND searchInfo = NULL;
	WNDCLASS wc = { 0,};
	RECT rect;
	BOOL re = FALSE;
	LONG_PTR tmp = 0;

	if (!searchboxInfo) {
		goto escapeArea;
	}

	searchInfo = searchboxInfo;
	if (searchInfo->dwSize != sizeof(XCUSTOMSEARCHBAND)) {
		goto escapeArea;
	}

	wc.lpfnWndProc = SearchbarProc;
	wc.lpszClassName = VF_SEARCHBAND_CLSNAME;
	wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);

	if (!RegisterClassW(&wc)) {
		goto escapeArea;
	}

	searchInfo->hSearchBox = CreateWindowExW(0, VF_SEARCHBAND_CLSNAME, NULL, WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 0, 0, searchInfo->hTargetHwnd, NULL, NULL, NULL);
	if (!searchInfo->hSearchBox) {
		goto escapeArea;
	}
	SetWindowLongPtr(searchInfo->hSearchBox, GWLP_USERDATA, (LONG_PTR)searchInfo);
	SetParent(searchInfo->hTargetHwnd, searchInfo->hSearchBox);

	searchInfo->hSearchEditBox = CreateWindowExW(0, L"Edit", NULL, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 0, 0, 0, 0, searchInfo->hSearchBox, NULL, NULL, NULL);
	searchInfo->oldEditProc = (WNDPROC)GetWindowLongPtr(searchInfo->hSearchEditBox, GWLP_WNDPROC);
	
	SetWindowLongPtr(searchInfo->hSearchEditBox, GWLP_WNDPROC, (LONG_PTR)SearchbarEditProc);
	SetWindowLongPtr(searchInfo->hSearchEditBox, GWLP_USERDATA, (LONG_PTR)searchInfo);
	
	GetClientRect(searchInfo->hTargetHwnd, &rect);


	if (XCusumSearchbandEnabled(searchInfo->hTargetHwnd)) {
		tmp = GetWindowLongPtrW(searchInfo->hOriginalBox, GWL_STYLE);
		tmp &= ~(WS_VISIBLE);
		SetWindowLongPtr(searchInfo->hOriginalBox, GWL_STYLE, tmp);
	}
	SetParentLocation(&rect, searchInfo);

	ShowWindow(searchInfo->hSearchBox, SW_SHOWNORMAL);

	ShowWindow(searchInfo->hSearchEditBox, SW_SHOWNORMAL);

	searchInfo->hWatchSizeThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CheckParentWindowSize, searchInfo, 0, NULL);


	re = TRUE;
escapeArea:

	return re;
}

__declspec(dllexport) void VFSetOptionsSearchband(PVOID searchboxInfo, DWORD option, PVOID exAttr) {
	PXCUSTOMSEARCHBAND searchInfo = NULL;
	DWORD dwValue = 0;

	if (!searchboxInfo) {
		goto escapeArea;
	}

	searchInfo = searchboxInfo;
	if (searchInfo->dwSize != sizeof(XCUSTOMSEARCHBAND)) {
		goto escapeArea;
	}

	switch (option) {
		case VF_SEARCH_ENABLE: {
			searchInfo->dwFlagsSet |= VF_SEARCH_ENABLE;
			searchInfo->dwFlagsSet &= ~VF_SEARCH_DISABLE;
			SendMessageW(searchInfo->hSearchEditBox, EM_SETREADONLY, FALSE, 0);
			SendMessageW(searchInfo->hSearchEditBox, EM_SETBKGNDCOLOR, 0, RGB(255, 255, 255));
		}; break;
		case VF_SEARCH_DISABLE: {
			searchInfo->dwFlagsSet |= VF_SEARCH_DISABLE;
			searchInfo->dwFlagsSet &= ~VF_SEARCH_ENABLE;
			SendMessageW(searchInfo->hSearchEditBox, EM_SETREADONLY, TRUE, 0);
			//SendMessageW(searchInfo->hSearchEditBox, EM_SETBKGNDCOLOR, 0, RGB(212, 212, 212));
		}; break;
		case VF_SEARCH_PLACEHOLDERTEXT: {
			if (exAttr) { //LPCWSTR
				if (searchInfo->pPlaceHolderTextW) {
					free(searchInfo->pPlaceHolderTextW);
				}

				dwValue = (DWORD)wcslen(exAttr);
				searchInfo->pPlaceHolderTextW = (LPWSTR)malloc(sizeof(WCHAR) * (dwValue + 1));
				if (searchInfo->pPlaceHolderTextW) {
					wcscpy_s(searchInfo->pPlaceHolderTextW, dwValue + 1, exAttr);
				}

				searchInfo->dwFlags |= 1;
				VFSetPlaceholderTextW(searchInfo->hSearchEditBox, searchInfo->pPlaceHolderTextW);
			}
			else {
				searchInfo->dwFlags &= ~1;
				VFUnsetPlaceholderTextW(searchInfo->hSearchEditBox);

				if (searchInfo->pPlaceHolderTextW) {
					free(searchInfo->pPlaceHolderTextW);
					searchInfo->pPlaceHolderTextW = NULL;
				}
			}
		}; break;
	}

escapeArea:
	return;
}

__declspec(dllexport) void VFCloseSearchBand(PVOID searchboxInfo) {
	PXCUSTOMSEARCHBAND searchInfo = NULL;
	LONG_PTR tmp = 0;

	if (!searchboxInfo) {
		goto escapeArea;
	}

	searchInfo = searchboxInfo;
	if (searchInfo->dwSize != sizeof(XCUSTOMSEARCHBAND)) {
		goto escapeArea;
	}

	if (searchInfo->hWatchSizeThread && searchInfo->hWatchSizeThread != INVALID_HANDLE_VALUE) {
		TerminateThread(searchInfo->hWatchSizeThread,0);
		CloseHandle(searchInfo->hWatchSizeThread);
	}

	DestroyWindow(searchInfo->hSearchBox);

	if (!XCusumSearchbandEnabled(searchInfo->hTargetHwnd)) {
		tmp = GetWindowLongPtrW(searchInfo->hOriginalBox, GWL_STYLE);
		tmp |= (WS_VISIBLE);
		if (!SetWindowLongPtr(searchInfo->hOriginalBox, GWL_STYLE, tmp)) {
			SendMessage(NULL, 0, 0, 0);
		}
	}

escapeArea:
	return;
}

__declspec(dllexport) void VFCleanSearchboxInfo(PVOID searchboxInfo) {
	PXCUSTOMSEARCHBAND searchInfo = NULL;
	DWORD lastErr = 0;

	if (!searchboxInfo) {
		goto escapeArea;
	}

	searchInfo = searchboxInfo;
	if (searchInfo->dwSize != sizeof(XCUSTOMSEARCHBAND)) {
		goto escapeArea;
	}

	if (searchInfo->pPlaceHolderTextW) {
		free(searchInfo->pPlaceHolderTextW);
	}

	//free richedit control dll
	DeleteObject(searchInfo->hEditboxFont);

	free(searchInfo);

escapeArea:
	return;
}

BOOL XFindSearchband(HWND hwnd, LPARAM lParam) {
	WCHAR className[MAX_PATH];
	PXCUSTOMSEARCHBAND searchInfo = (PXCUSTOMSEARCHBAND)lParam;
	DWORD tmp = 0;
	HWND tmpHwnd;

	if (!searchInfo) {
		return FALSE;
	}
	
	GetClassNameW(hwnd, className, MAX_PATH);

	//Searchbox class name (Windows Vista +)
	if (!wcscmp(className, L"UniversalSearchBand")) {
		searchInfo->hTargetHwnd = hwnd;
		searchInfo->hOriginalBox = NULL;
		tmpHwnd = GetWindow(hwnd, GW_CHILD);
		for (tmp = 0; tmp < 3; tmp++) {
			GetClassNameW(tmpHwnd, className, MAX_PATH);
			if (wcscmp(className,VF_SEARCHBAND_CLSNAME)) {
				searchInfo->hOriginalBox = tmpHwnd;
				break;
			}
			tmpHwnd = GetWindow(tmpHwnd, GW_HWNDNEXT);
		}
		
		//GetClassNameW(searchInfo->hOriginalBox, className, MAX_PATH);
		return FALSE;
	}

	return TRUE;
}

LRESULT CALLBACK SearchbarEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	WCHAR text[MAX_PATH];
	PXCUSTOMSEARCHBAND info = (PXCUSTOMSEARCHBAND)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	LONG_PTR tmp = 0;

	if (!info) {
		return E_FAIL;
	}

	if (msg == WM_KEYDOWN && wParam == VK_RETURN) {
		tmp = GetWindowLongPtrW(info->hSearchEditBox, GWL_STYLE);
		if (!(tmp & ES_READONLY)) {
			//Get Current search text
			GetWindowText(info->hSearchEditBox, text, MAX_PATH);
			if (info->callback) {
				info->callback(text, info->userData);
			}
			else {
				MessageBoxW(NULL, text, L"Custom NSE Search", 0);
			}
			//Clear current search text
			SendMessageW(info->hSearchEditBox, WM_SETTEXT, (WPARAM)NULL, (LPARAM)L"");
		}
	}
	else if (msg == WM_GETDLGCODE && wParam == VK_RETURN) {
		//allow enter key in file dialog
		return (DLGC_WANTALLKEYS | CallWindowProcW(info->oldEditProc, hwnd, msg, wParam, lParam));
	}
	else if (msg == WM_SETFOCUS) {
		tmp = GetWindowLongPtrW(info->hSearchEditBox, GWL_STYLE);
		if (tmp & ES_READONLY) {
			return 0;
		}
		if (info->pPlaceHolderTextW) {
			if (info->dwFlags & 1) {
				info->dwFlags &= ~1;
				VFUnsetPlaceholderTextW(info->hSearchEditBox);
			}
		}
	}
	else if (msg == WM_KILLFOCUS) {
		if (info->pPlaceHolderTextW) {
			GetWindowText(info->hSearchEditBox, text, MAX_PATH);
			if (wcslen(text) == 0) {
				info->dwFlags |= 1; //Enable Placeholder Text
				VFSetPlaceholderTextW(info->hSearchEditBox, info->pPlaceHolderTextW);
			}
		}
	}
		
	return CallWindowProcW(info->oldEditProc, hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK SearchbarProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	HDC hdc;
	RECT rect;
	HPEN pen;
	HGDIOBJ old;
	DWORD width, height = 0;
	PXCUSTOMSEARCHBAND info = (PXCUSTOMSEARCHBAND)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	if (msg == WM_NCPAINT && info->dwFlagsSet & VF_SEARCH_USECUSTOMBORDER) {
		GetWindowRect(hWnd, &rect);
		hdc = GetDCEx(hWnd, (HRGN)wParam, DCX_WINDOW | DCX_CACHE | DCX_INTERSECTRGN | DCX_LOCKWINDOWUPDATE);
		pen = CreatePen(PS_INSIDEFRAME, 2, RGB(200, 200, 200));
		old = SelectObject(hdc, pen);
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
		Rectangle(hdc, 0, 0, width, height);
		SelectObject(hdc, old);
		DeleteObject(pen);
		ReleaseDC(hWnd, hdc);

		return 0;
	}
	else if (msg == WM_ERASEBKGND) {
		GetClientRect(hWnd, &rect);
		if (info->dwFlagsSet & VF_SEARCH_DISABLE) {
			SetBkColor((HDC)wParam, RGB(230, 230, 230));
		}
		else {
			SetBkColor((HDC)wParam, RGB(255, 255, 255));
		}

		ExtTextOut((HDC)wParam, 0, 0, ETO_OPAQUE, &rect, 0, 0, 0);
		return 1;
	}
	else if (msg == WM_CTLCOLORSTATIC) {
		hdc = (HDC)wParam;

		SetBkColor(hdc, RGB(230, 230, 230));

		return (INT_PTR)GetStockObject(WHITE_BRUSH);
	}
	   
	return DefWindowProcW(hWnd,msg,wParam,lParam);
}

//Check parent original searchband is resized
DWORD CheckParentWindowSize(LPVOID args) {
	RECT rect = { 0, };  //check custom search width
	RECT rect2 = { 0, }; //check original search width
	RECT rect3 = { 0, }; //check original search height (if available)
	RECT rect4 = { 0, }; //check editcontrol is enabled

	PXCUSTOMSEARCHBAND info = NULL;

	INT parentWidth = 0;

	if (!args) {
		return -1;
	}

	info = (PXCUSTOMSEARCHBAND)args;
	for (;;) {
		GetClientRect(info->hSearchBox,&rect);

		GetClientRect(info->hTargetHwnd, &rect2);

		GetClientRect(info->hSearchEditBox, &rect4);

		if (info->hOriginalBox) {
			GetClientRect(info->hOriginalBox, &rect3);
		}
		else {
			rect3.bottom = rect2.bottom;
		}

		parentWidth = (INT)(rect2.right * info->dpi);


		if (parentWidth - rect.right > 2 || rect.right - parentWidth > 2 || rect4.right == 0) {
			MoveWindow(info->hSearchBox, 0, info->wMainYLocation, parentWidth, (INT)((rect3.bottom - info->wMainYLocation + 2) * info->dpi), TRUE);
			MoveWindow(info->hSearchEditBox, 3, info->wEditYLocation, parentWidth, info->wEditBoxHeight, TRUE);

			if (info->wEditBoxHeight == 0) {
				//????
				GetClientRect(info->hOriginalBox, &rect3);
				SetEditboxLocation(rect3.bottom, info);
			}
		}
		
		Sleep(1000);
	}

}
//Check Custom Searchband is showed
BOOL XCusumSearchbandEnabled(HWND hwnd) {
	BOOL re = FALSE;

	EnumChildWindows(hwnd, (WNDENUMPROC)XFindCustomSearchBand, (LPARAM)&re);

	return re;
}
BOOL XFindCustomSearchBand(HWND hwnd, LPARAM lParam) {
	WCHAR text[MAX_PATH];
	GetClassNameW(hwnd, text, MAX_PATH);

	if (!wcscmp(text, VF_SEARCHBAND_CLSNAME)) {
		*((PBOOL)lParam) = TRUE;
		return FALSE;
	}
	return TRUE;
}

void VFSetPlaceholderTextW(HWND hwnd, LPCWSTR text) {
	CHARFORMATW format = { sizeof(CHARFORMATW),0, };
	SetWindowText(hwnd, text);
	format.cbSize = sizeof(CHARFORMATW);
	format.dwMask = CFM_COLOR;
	format.crTextColor = RGB(180, 180, 180);
	SendMessageW(hwnd, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&format);
}

void VFUnsetPlaceholderTextW(HWND hwnd) {
	CHARFORMATW format = { sizeof(CHARFORMATW),0, };
	SetWindowText(hwnd, L"");
	format.dwMask = CFM_COLOR;
	format.crTextColor = RGB(0, 0, 0);
	SendMessageW(hwnd, EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&format);
}

void SetParentLocation(PRECT pParentRect,PXCUSTOMSEARCHBAND info) {
	RECT rect = { 0, };
	if (info->hOriginalBox) {
		GetClientRect(info->hOriginalBox, &rect);
		info->wMainYLocation = (WORD)(pParentRect->bottom - rect.bottom)	;
		pParentRect->bottom = rect.bottom;
	}

	SetWindowPos(info->hSearchBox, HWND_TOP, 0, info->wMainYLocation, pParentRect->right, pParentRect->bottom, 0);
}

void SetEditboxLocation(DWORD parentHeight,PXCUSTOMSEARCHBAND info) {
	if (!info->wEditBoxHeight) {
		if (parentHeight > 35) {
			info->wEditBoxHeight = 26;
		}
		else {
			info->wEditBoxHeight = parentHeight - 7;
		}
		info->wEditBoxHeight -= (info->wMainYLocation * 2);

		info->wEditYLocation = (WORD)((parentHeight - info->wEditBoxHeight) / 2);

		if (!info->hEditboxFont) {
			info->hEditboxFont = CreateFont(info->wEditBoxHeight, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
		}
		SendMessage(info->hSearchEditBox, WM_SETFONT, (WPARAM)info->hEditboxFont, 1);

		SetWindowPos(info->hSearchEditBox, HWND_TOP, 3, info->wEditYLocation, 0, info->wEditBoxHeight, 0);
	}
}