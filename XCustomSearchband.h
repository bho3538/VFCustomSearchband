#pragma once

#if __cplusplus
extern "C" {
#endif

#ifndef VF_SEARCH_OPTION
#define VF_SEARCH_OPTION
#define VF_SEARCH_ENABLE 1
#define VF_SEARCH_DISABLE 2
#define VF_SEARCH_PLACEHOLDERTEXT 4
#define VF_SEARCH_USECUSTOMBORDER 8
#endif
	typedef void(__stdcall *CCUSTOMSEARCHCALLBACK)(LPCWSTR, PVOID);

	__declspec(dllexport) PVOID VFInitializeCustomSearchBand(HWND targetHwnd, CCUSTOMSEARCHCALLBACK searchCallback, PVOID userData, BOOL findSearchBand);

	__declspec(dllexport) void VFSetOptionsSearchband(PVOID searchboxInfo,DWORD option,PVOID exAttr);

	__declspec(dllexport) BOOL VFShowCustomSearchBand(PVOID searchboxInfo);

	__declspec(dllexport) void VFCloseSearchBand(PVOID searchboxInfo);

	__declspec(dllexport) void VFCleanSearchboxInfo(PVOID searchboxInfo);

#if __cplusplus
}
#endif

