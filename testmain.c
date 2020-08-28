#include "pch.h"

#include "XCustomSearchband.h"

#include <stdio.h>

int main() {
	LONGLONG a = 0;
	
	scanf_s("%Ix", &a);

	MSG Msg;
	HWND explorer = (HWND)a;
	PVOID aa = VFInitializeCustomSearchBand(explorer, NULL, NULL, 1);
	if (aa) {
		VFShowCustomSearchBand(aa);
		//VFSetOptionsSearchband(aa, VF_SEARCH_DISABLE, NULL);
		VFSetOptionsSearchband(aa, VF_SEARCH_PLACEHOLDERTEXT, (PVOID)L"Remote Search");
	}
	while (GetMessage(&Msg, NULL, 0, 0) > 0) {
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

}