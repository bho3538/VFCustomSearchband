
#include "pch.h"
#include "XCustomSearchband.h"

typedef struct _XCUSTOMSEARCHBAND {
	DWORD dwSize;
	DWORD dwFlags;
	DWORD dwReserved; //reserved
	FLOAT dpi;
	HWND hTargetHwnd;
	HWND hSearchBox;
	HWND hOriginalBox;
	HINSTANCE hSearchboxM;
	LPWSTR pPlaceHolderTextW;
	WNDPROC g_OldProc;
	HANDLE hWatchSizeThread;
	CCUSTOMSEARCHCALLBACK callback;
	PVOID userData;
} XCUSTOMSEARCHBAND, *PXCUSTOMSEARCHBAND;

void VFSetPlaceholderTextW(HWND hwnd, LPCWSTR text);
void VFUnsetPlaceholderTextW(HWND hwnd);

__declspec(dllexport) PVOID VFInitializeCustomSearchBand(HWND targetHwnd, CCUSTOMSEARCHCALLBACK callback, PVOID userData, BOOL findSearchBand) {
	BOOL re = FALSE;
	PXCUSTOMSEARCHBAND searchInfo = (PXCUSTOMSEARCHBAND)malloc(sizeof(XCUSTOMSEARCHBAND));
	if (!searchInfo) {
		goto escapeArea;
	}

	ZeroMemory(searchInfo, sizeof(XCUSTOMSEARCHBAND));

	searchInfo->dwSize = sizeof(XCUSTOMSEARCHBAND);
	searchInfo->dwFlags = 0;
	searchInfo->hSearchboxM = LoadLibraryW(L"Msftedit.dll");
	searchInfo->callback = callback;
	searchInfo->userData = userData;

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
	searchInfo->hSearchBox = CreateWindowExW(0,MSFTEDIT_CLASS, NULL, WS_CHILD | WS_VISIBLE | WS_BORDER, 0, 0, 0, 0, searchInfo->hTargetHwnd, NULL, NULL, NULL);
	if (!searchInfo->hSearchBox) {
		goto escapeArea;
	}

	SendMessage(searchInfo->hSearchBox, WM_VSCROLL, SB_BOTTOM, 0);

	searchInfo->g_OldProc = (WNDPROC)GetWindowLongPtr(searchInfo->hSearchBox, GWLP_WNDPROC);
	SetWindowLongPtr(searchInfo->hSearchBox, GWLP_WNDPROC, (LONG_PTR)SearchbarProc);
	SetWindowLongPtr(searchInfo->hSearchBox, GWLP_USERDATA, (LONG_PTR)searchInfo);


	SetParent(searchInfo->hTargetHwnd, searchInfo->hSearchBox);
	
	GetClientRect(searchInfo->hTargetHwnd, &rect);

	//searchInfo->dpi= XGetCurrentWindowDPI(searchInfo->hSearchBox);
	searchInfo->dpi = 1.0f;
	//SetWindowPos(searchInfo->hSearchBox, HWND_TOP, 0, 0, (INT)(rect.right * searchInfo->dpi), (INT)(rect.bottom * searchInfo->dpi), 0);

	SetWindowPos(searchInfo->hSearchBox, HWND_TOP, 0, 0, rect.right, rect.bottom, 0);


	SendMessage(searchInfo->hSearchBox, EM_GETRECT, 0, (LPARAM)&rect);

	if (rect.bottom - rect.top > 28) {
		rect.top += 4;
	}
	rect.left += 4;
	SendMessage(searchInfo->hSearchBox, EM_SETRECT, 1, (LPARAM)&rect);


	ShowWindow(searchInfo->hSearchBox, SW_SHOWNORMAL);

	searchInfo->hWatchSizeThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CheckParentWindowSize, searchInfo, 0, NULL);

	if (XCusumSearchbandEnabled(searchInfo->hTargetHwnd)) {
		tmp = GetWindowLongPtrW(searchInfo->hOriginalBox, GWL_STYLE);
		tmp &= ~(WS_VISIBLE);
		SetWindowLongPtr(searchInfo->hOriginalBox, GWL_STYLE, tmp);
	}

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
			SendMessageW(searchInfo->hSearchBox, EM_SETREADONLY, FALSE, 0);
			SendMessageW(searchInfo->hSearchBox, EM_SETBKGNDCOLOR, 0, RGB(255, 255, 255));
		}; break;
		case VF_SEARCH_DISABLE: {
			SendMessageW(searchInfo->hSearchBox, EM_SETREADONLY, TRUE, 0);
			SendMessageW(searchInfo->hSearchBox, EM_SETBKGNDCOLOR, 0, RGB(212, 212, 212));
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
				VFSetPlaceholderTextW(searchInfo->hSearchBox, searchInfo->pPlaceHolderTextW);
			}
			else {
				searchInfo->dwFlags &= ~1;
				VFUnsetPlaceholderTextW(searchInfo->hSearchBox);

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
	FreeLibrary(searchInfo->hSearchboxM);

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
			if (wcscmp(className, L"RICHEDIT50W")) {
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

LRESULT CALLBACK SearchbarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	WCHAR text[MAX_PATH];
	PXCUSTOMSEARCHBAND info = (PXCUSTOMSEARCHBAND)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	LONG_PTR tmp = 0;

	if (!info) {
		return E_FAIL;
	}

	if (msg == WM_KEYDOWN && wParam == VK_RETURN) {
		tmp = GetWindowLongPtrW(info->hSearchBox, GWL_STYLE);
		if (!(tmp & ES_READONLY)) {
			//Get Current search text
			GetWindowText(info->hSearchBox, text, MAX_PATH);
			if (info->callback) {
				info->callback(text, info->userData);
			}
			else {
				MessageBoxW(NULL, text, L"Custom NSE Search", 0);
			}
			//Clear current search text
			SendMessageW(info->hSearchBox, WM_SETTEXT, (WPARAM)NULL, (LPARAM)L"");
		}
	}
	else if (msg == WM_GETDLGCODE && wParam == VK_RETURN) {
		//allow enter key in file dialog
		return (DLGC_WANTALLKEYS  | CallWindowProcW(info->g_OldProc, hwnd, msg, wParam, lParam));
	}
	else if (msg == WM_SETFOCUS) {
		tmp = GetWindowLongPtrW(info->hSearchBox, GWL_STYLE);
		if (tmp & ES_READONLY) {
			return 0;
		}
		if (info->pPlaceHolderTextW) {
			if (info->dwFlags & 1) {
				info->dwFlags &= ~1;
				VFUnsetPlaceholderTextW(info->hSearchBox);
			}
		}
	}
	else if (msg == WM_KILLFOCUS) {
		if (info->pPlaceHolderTextW) {
			GetWindowText(info->hSearchBox, text, MAX_PATH);
			if (wcslen(text) == 0) {
				info->dwFlags |= 1; //Enable Placeholder Text
				VFSetPlaceholderTextW(info->hSearchBox, info->pPlaceHolderTextW);
			}
		}
	}
	return CallWindowProcW(info->g_OldProc,hwnd, msg, wParam, lParam);
}

//Check parent original searchband is resized
DWORD CheckParentWindowSize(LPVOID args) {
	RECT rect = { 0, };
	RECT rect2 = { 0, };

	PXCUSTOMSEARCHBAND info = NULL;

	INT parentWidth = 0;

	if (!args) {
		return -1;
	}

	info = (PXCUSTOMSEARCHBAND)args;
	for (;;) {

		GetClientRect(info->hSearchBox,&rect);

		GetClientRect(info->hTargetHwnd, &rect2);

		parentWidth = (INT)(rect2.right * info->dpi);
		if (parentWidth - rect.right > 15 || rect.right - parentWidth > 15) {
			MoveWindow(info->hSearchBox, 0, 0, parentWidth, (INT)(rect2.bottom * info->dpi), TRUE);
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

	if (!wcscmp(text, L"RICHEDIT50W")) {
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