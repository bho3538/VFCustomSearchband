
#include "pch.h"
#include "XCustomSearchband.h"

typedef struct _XCUSTOMSEARCHBAND {
	DWORD dwSize;
	DWORD dwFlags;
	FLOAT dpi;
	HWND hTargetHwnd;
	HWND hSearchBox;
	HINSTANCE hSearchboxM;
	LPWSTR pPlaceHolderText; //not use yet
	WNDPROC g_OldProc;
	HANDLE hWatchSizeThread;
	CCUSTOMSEARCHCALLBACK callback;
	PVOID userData;
} XCUSTOMSEARCHBAND, *PXCUSTOMSEARCHBAND;



__declspec(dllexport) PVOID VFInitializeCustomSearchBand(HWND targetHwnd, CCUSTOMSEARCHCALLBACK callback, PVOID userData, BOOL findSearchBand) {
	BOOL re = FALSE;
	PXCUSTOMSEARCHBAND searchInfo = (PXCUSTOMSEARCHBAND)malloc(sizeof(XCUSTOMSEARCHBAND));
	if (!searchInfo) {
		goto escapeArea;
	}

	ZeroMemory(searchInfo, sizeof(XCUSTOMSEARCHBAND));

	searchInfo->dwSize = sizeof(XCUSTOMSEARCHBAND);
	searchInfo->dwFlags = 0;
	searchInfo->hTargetHwnd = targetHwnd;
	searchInfo->hSearchboxM = LoadLibraryW(L"Msftedit.dll");
	searchInfo->callback = callback;
	searchInfo->userData = userData;

	if (findSearchBand) {
		EnumChildWindows(targetHwnd, (WNDENUMPROC)XFindSearchband, (LPARAM)searchInfo);
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


	ShowWindow(searchInfo->hSearchBox, SW_SHOWNORMAL);

	searchInfo->hWatchSizeThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)CheckParentWindowSize, searchInfo, 0, NULL);

	re = TRUE;
escapeArea:

	return re;
}

__declspec(dllexport) void VFCloseSearchBand(PVOID searchboxInfo) {
	PXCUSTOMSEARCHBAND searchInfo = NULL;

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

	//free richedit control dll
	FreeLibrary(searchInfo->hSearchboxM);

	free(searchInfo);

escapeArea:
	return;
}

BOOL XFindSearchband(HWND hwnd, LPARAM lParam) {
	WCHAR className[MAX_PATH];
	PXCUSTOMSEARCHBAND searchInfo = (PXCUSTOMSEARCHBAND)lParam;

	if (!searchInfo) {
		return FALSE;
	}
	
	GetClassNameW(hwnd, className, MAX_PATH);

	//Searchbox class name (Windows Vista +)
	if (!wcscmp(className, L"UniversalSearchBand")) {
		searchInfo->hTargetHwnd = hwnd;
		return FALSE;
	}
	return TRUE;
}

LRESULT CALLBACK SearchbarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	WCHAR text[MAX_PATH];
	PXCUSTOMSEARCHBAND info = (PXCUSTOMSEARCHBAND)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	if (!info) {
		return E_FAIL;
	}

	if (msg == WM_KEYDOWN && wParam == VK_RETURN) {
		//Get Current search text
		GetWindowText(info->hSearchBox, text, MAX_PATH);
		if (info->callback) {
			info->callback(text,info->userData);
		}
		else {
			MessageBoxW(NULL, text, L"Custom NSE Search", 0);
		}
		//Clear current search text
		SendMessageW(info->hSearchBox, WM_SETTEXT, (WPARAM)NULL, (LPARAM)L"");
		
	}
	else if (msg == WM_GETDLGCODE && wParam == VK_RETURN) {
		//allow enter key in file dialog
		return (DLGC_WANTALLKEYS  | CallWindowProcW(info->g_OldProc, hwnd, msg, wParam, lParam));
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