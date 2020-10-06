#include "pch.h"
#include "XCustomSearchband.h"

#define VF_SEARCHBAND_CLSNAME L"BYUNGHO_SEARCHBOX"

#define VF_SEARCH_USEDARKTHEME 16

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
BOOL CALLBACK XFindSearchband(HWND hwnd, LPARAM lParam);

LRESULT CALLBACK SearchbarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK SearchbarEditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

DWORD CheckParentWindowSize(LPVOID args);

BOOL XCusumSearchbandEnabled(HWND hwnd);
BOOL CALLBACK XFindCustomSearchBand(HWND hwnd, LPARAM lParam);

void VFSetPlaceholderTextW(HWND hwnd, LPCWSTR text);
void VFUnsetPlaceholderTextW(HWND hwnd);

void SetParentLocation(PRECT pParentRect,PXCUSTOMSEARCHBAND info,BOOL setTop);
void SetEditboxLocation(DWORD parentHeight,PXCUSTOMSEARCHBAND info);
BOOL IsSystemUsingDarkTheme();

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

	//for windows 10 +
	if (IsSystemUsingDarkTheme()) {
		searchInfo->dwFlags |= VF_SEARCH_USEDARKTHEME;
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

	if (searchInfo->dwFlags & VF_SEARCH_USEDARKTHEME) {
		wc.hbrBackground = (HBRUSH)GetStockObject(DKGRAY_BRUSH);
	}
	else {
		wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	}

	RegisterClassW(&wc);

	searchInfo->hSearchBox = CreateWindowExW(0, VF_SEARCHBAND_CLSNAME, NULL, WS_CHILD | WS_BORDER | WS_VISIBLE, 0, 0, 0, 0, searchInfo->hTargetHwnd, NULL, NULL, searchInfo);
	if (!searchInfo->hSearchBox) {
		goto escapeArea;
	}
	SetWindowLongPtr(searchInfo->hSearchBox, GWLP_USERDATA, (LONG_PTR)searchInfo);

	GetClientRect(searchInfo->hTargetHwnd, &rect);

	//hide original searchband if custom searchband is enabled
	if (XCusumSearchbandEnabled(searchInfo->hTargetHwnd)) {
		tmp = GetWindowLongPtrW(searchInfo->hOriginalBox, GWL_STYLE);
		tmp &= ~(WS_VISIBLE);
		SetWindowLongPtr(searchInfo->hOriginalBox, GWL_STYLE, tmp);
	}


	if (searchInfo->pPlaceHolderTextW) {
		VFSetPlaceholderTextW(searchInfo->hSearchEditBox, searchInfo->pPlaceHolderTextW);
	}
	SetParentLocation(&rect, searchInfo,TRUE);
	SetEditboxLocation(rect.bottom, searchInfo);

	ShowWindow(searchInfo->hSearchBox, SW_SHOWNORMAL);

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
			//SendMessageW(searchInfo->hSearchEditBox, EM_SETBKGNDCOLOR, 0, RGB(255, 255, 255));
		}; break;
		case VF_SEARCH_DISABLE: {
			searchInfo->dwFlagsSet |= VF_SEARCH_DISABLE;
			searchInfo->dwFlagsSet &= ~VF_SEARCH_ENABLE;
			SendMessageW(searchInfo->hSearchEditBox, EM_SETREADONLY, TRUE, 0);
			//SendMessageW(searchInfo->hSearchEditBox, EM_SETBKGNDCOLOR, 0, RGB(212, 212, 212));
		}; break;
		
		//temporary disabled
		//case VF_SEARCH_PLACEHOLDERTEXT: {
		//	if (exAttr) { //LPCWSTR
		//		if (searchInfo->pPlaceHolderTextW) {
		//			free(searchInfo->pPlaceHolderTextW);
		//		}

		//		dwValue = (DWORD)wcslen(exAttr);
		//		searchInfo->pPlaceHolderTextW = (LPWSTR)malloc(sizeof(WCHAR) * (dwValue + 1));
		//		if (searchInfo->pPlaceHolderTextW) {
		//			wcscpy_s(searchInfo->pPlaceHolderTextW, dwValue + 1, exAttr);
		//		}

		//		searchInfo->dwFlags |= 1;
		//		VFSetPlaceholderTextW(searchInfo->hSearchEditBox, searchInfo->pPlaceHolderTextW);
		//	}
		//	else {
		//		searchInfo->dwFlags &= ~1;
		//		VFUnsetPlaceholderTextW(searchInfo->hSearchEditBox);

		//		if (searchInfo->pPlaceHolderTextW) {
		//			free(searchInfo->pPlaceHolderTextW);
		//			searchInfo->pPlaceHolderTextW = NULL;
		//		}
		//	}
		//}; break;
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

BOOL CALLBACK XFindSearchband(HWND hwnd, LPARAM lParam) {
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
	HDC hdc = NULL;

	if (!info) {
		return E_FAIL;
	}

	switch (msg) {
		case WM_KEYDOWN: {
			if (wParam == VK_RETURN) {
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
		}; break;
		case WM_GETDLGCODE: {
			//allow enter key in file dialog
			if (wParam == VK_RETURN) {
				return (DLGC_WANTALLKEYS | CallWindowProcW(info->oldEditProc, hwnd, msg, wParam, lParam));
			}
		}; break;
		case WM_SETFOCUS: {
			tmp = GetWindowLongPtrW(info->hSearchEditBox, GWL_STYLE);
			if (tmp & ES_READONLY) {
				return 0;
			}
			if (info->pPlaceHolderTextW) {
				if (info->dwFlags & 1) {
					info->dwFlags &= ~1;
					VFUnsetPlaceholderTextW(info->hSearchEditBox);
				}
			}; break;
		case WM_KILLFOCUS: {
			if (info->pPlaceHolderTextW) {
				GetWindowText(info->hSearchEditBox, text, MAX_PATH);
				if (wcslen(text) == 0) {
					info->dwFlags |= 1; //Enable Placeholder Text
					VFSetPlaceholderTextW(info->hSearchEditBox, info->pPlaceHolderTextW);
				}
			}
			}; break;
		}
		case WM_MOUSEMOVE: {
			//SET disabled cursor if searchband is readonly mode
			if (info->dwFlagsSet & VF_SEARCH_DISABLE) {
				SetClassLongPtr(info->hSearchEditBox, -12, (LONG_PTR)LoadCursor(NULL, IDC_NO)); //GCL_HCURSOR
				return 0;
			}
			else {
				SetClassLongPtr(info->hSearchEditBox, -12, (LONG_PTR)LoadCursor(NULL, IDC_ARROW)); //GCL_HCURSOR
				return 0;
			}
		}; break;

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
	LPCREATESTRUCTW data = NULL;

	switch (msg) {
		case WM_CREATE: {
			data = (LPCREATESTRUCTW)lParam;
			if (data && data->lpCreateParams) {
				info = data->lpCreateParams;
				info->hSearchBox = hWnd;
				info->hSearchEditBox = CreateWindowExW(0, L"Edit", NULL, WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL , 0, 0, 0, 0, hWnd, NULL, NULL, NULL);
				info->oldEditProc = (WNDPROC)GetWindowLongPtr(info->hSearchEditBox, GWLP_WNDPROC);

				SetWindowLongPtr(info->hSearchEditBox, GWLP_WNDPROC, (LONG_PTR)SearchbarEditProc);
				SetWindowLongPtr(info->hSearchEditBox, GWLP_USERDATA, (LONG_PTR)info);

				GetClientRect(info->hTargetHwnd, &rect);

				SetParentLocation(&rect, info,FALSE);
				SetEditboxLocation(rect.bottom, info);

				MoveWindow(info->hSearchEditBox, 3, info->wEditYLocation, rect.right, info->wEditBoxHeight, FALSE);
			}
		}; break;
		case WM_NCPAINT: {
			if (info && info->dwFlagsSet & VF_SEARCH_USECUSTOMBORDER) {
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
		}; break;
		case WM_CTLCOLOREDIT: {
			if (info->dwFlags & VF_SEARCH_USEDARKTHEME) {
				hdc = (HDC)wParam;
				SetBkColor(hdc, RGB(64, 64, 64));
				SetTextColor(hdc, RGB(255, 255, 255));
				return (LRESULT)GetStockObject(DKGRAY_BRUSH);
			}
		}; break;
		//case WM_ERASEBKGND: {

		//	//if (info && info->dwFlagsSet & VF_SEARCH_DISABLE) {
		//	//	SetBkColor((HDC)wParam, RGB(192, 192, 192));
		//	//}
		//	//else {
		//	//	SetBkColor((HDC)wParam, RGB(255, 255, 255));
		//	//}

		//	////ExtTextOut((HDC)wParam, 0, 0, ETO_OPAQUE, &rect, 0, 0, 0);
		//	//return 1;
		//}; break;
		case WM_CTLCOLORSTATIC: {
			if (info->dwFlags & VF_SEARCH_USEDARKTHEME) {
				hdc = (HDC)wParam;
				SetBkColor(hdc, RGB(64, 64, 64));
				SetTextColor(hdc, RGB(255, 255, 255));
				return (LRESULT)GetStockObject(DKGRAY_BRUSH);
			}
			else {
				return (INT_PTR)GetStockObject(WHITE_BRUSH);
			}
		}; break;
	}

	return DefWindowProcW(hWnd, msg, wParam, lParam);
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

		if (parentWidth - rect.right > 2 || rect.right - parentWidth > 2){
			MoveWindow(info->hSearchBox, 0, info->wMainYLocation, parentWidth, (INT)(rect3.bottom - info->wMainYLocation + 2), TRUE);
			MoveWindow(info->hSearchEditBox, 3, info->wEditYLocation, parentWidth, info->wEditBoxHeight, FALSE);
			//debug only (exe)
			//if (info->wEditBoxHeight == 0) {
			//	//????
			//	GetClientRect(info->hOriginalBox, &rect3);
			//	SetEditboxLocation(rect3.bottom, info);
			//}
		}
		else {
			Sleep(1000);
		}

	}

}
//Check Custom Searchband is showed
BOOL XCusumSearchbandEnabled(HWND hwnd) {
	BOOL re = FALSE;

	EnumChildWindows(hwnd, (WNDENUMPROC)XFindCustomSearchBand, (LPARAM)&re);

	return re;
}
BOOL CALLBACK XFindCustomSearchBand(HWND hwnd, LPARAM lParam) {
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

void SetParentLocation(PRECT pParentRect,PXCUSTOMSEARCHBAND info, BOOL setTop) {
	RECT rect = { 0, };
	if (info->hOriginalBox) {
		GetClientRect(info->hOriginalBox, &rect);
		info->wMainYLocation = (WORD)(pParentRect->bottom - rect.bottom);
		pParentRect->bottom = rect.bottom;
	}

	if (setTop) {
		SetWindowPos(info->hSearchBox, HWND_TOP, 0, info->wMainYLocation, pParentRect->right, pParentRect->bottom, 0);

		//for height (original searchband is little small)
		if (info->hOriginalBox) {
			MoveWindow(info->hSearchBox, 0, info->wMainYLocation, rect.right, (INT)(rect.bottom - info->wMainYLocation + 2), TRUE);
		}
	}
}

void SetEditboxLocation(DWORD parentHeight,PXCUSTOMSEARCHBAND info) {
	if (!info->wEditBoxHeight) {
		if (parentHeight > 35) {
			info->wEditBoxHeight = 28;
		}
		else {
			info->wEditBoxHeight = parentHeight - 5;
		}
		info->wEditBoxHeight -= ((info->wMainYLocation * 2));

		info->wEditYLocation = (WORD)((parentHeight - info->wEditBoxHeight) / 2) - 1;

		if (!info->hEditboxFont) {
			info->hEditboxFont = CreateFont(info->wEditBoxHeight, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Arial");
		}
		SendMessage(info->hSearchEditBox, WM_SETFONT, (WPARAM)info->hEditboxFont, 1);
	}
}

BOOL IsSystemUsingDarkTheme() {
	BOOL re = FALSE;
	HKEY hKey = INVALID_HANDLE_VALUE;
	DWORD val = 1;
	DWORD bufferSize = sizeof(DWORD);

	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", 0, KEY_READ, &hKey) != 0) {
		goto escapeArea;
	}

	RegQueryValueEx(hKey, L"AppsUseLightTheme", NULL, NULL, (LPBYTE)&val, &bufferSize);

	if (val == 0) {
		re = TRUE;
	}

escapeArea:
	if (hKey != INVALID_HANDLE_VALUE) {
		RegCloseKey(hKey);
	}

	return re;
}