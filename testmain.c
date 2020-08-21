#include "pch.h"

#include "XCustomSearchband.h"

int main() {
	MSG Msg;
	HWND explorer = (HWND)0x000202F8;
	PVOID aa = VFInitializeCustomSearchBand(explorer, NULL, NULL, TRUE);
	if (aa) {
		VFShowCustomSearchBand(aa);
		VFSetOptionsSearchband(aa, VF_SEARCH_PLACEHOLDERTEXT, (PVOID)L"Remote Search");
	}
	while (GetMessage(&Msg, NULL, 0, 0) > 0) {
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

}