#pragma once

#if __cplusplus
extern "C" {
#endif

	typedef void(__stdcall *CCUSTOMSEARCHCALLBACK)(LPCWSTR, PVOID);


	__declspec(dllexport) PVOID VFInitializeCustomSearchBand(HWND targetHwnd, CCUSTOMSEARCHCALLBACK searchCallback, PVOID userData, BOOL findSearchBand);
	__declspec(dllexport) BOOL VFShowCustomSearchBand(PVOID searchboxInfo);

	__declspec(dllexport) void VFCloseSearchBand(PVOID searchboxInfo);

	__declspec(dllexport) void VFCleanSearchboxInfo(PVOID searchboxInfo);

#if __cplusplus
}
#endif

//Internal Function

//FLOAT XGetCurrentWindowDPI(HWND hwnd);
BOOL XFindSearchband(HWND hwnd, LPARAM lParam);

LRESULT CALLBACK SearchbarProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

DWORD CheckParentWindowSize(LPVOID args);
