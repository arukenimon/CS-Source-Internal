// dllmain.cpp : Defines the entry point for the DLL application.
//#include "pch.h"

#include <iostream>

#include <d3d9.h>
#include <d3dx9.h>

#pragma comment(lib, "d3d9.lib")
#pragma comment(lib, "d3dx9.lib")

#include "detours.h"
#pragma comment(lib, "detours.lib")
#include "Header.h"

HINSTANCE DllHandle;

typedef HRESULT(__stdcall* endScene)(IDirect3DDevice9* pDevice);
endScene pEndScene;

LPD3DXFONT font;
ID3DXLine* pLine = nullptr;


int EntityCount = 300;
float fovRadius  = 100.0f;
bool g_menuOpen  = true;
bool g_espBones  = false;
bool g_fovCircle = false;
bool g_aimbot    = false;
bool g_teamEsp   = false;
int  g_menuSel   = 0;
D3DVIEWPORT9 vp;
HRESULT __stdcall hookedEndScene(IDirect3DDevice9* pDevice) {
	if (!font)
		D3DXCreateFont(pDevice, 14, 0, FW_BOLD, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &font);
	if (!pLine)
		D3DXCreateLine(pDevice, &pLine);

	// --- menu key handling (edge-triggered) ---
	static bool prevIns = false, prevUp = false, prevDown = false, prevEnter = false;
	bool curIns   = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
	bool curUp    = (GetAsyncKeyState(VK_UP)     & 0x8000) != 0;
	bool curDown  = (GetAsyncKeyState(VK_DOWN)   & 0x8000) != 0;
	bool curEnter = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;

	static MenuItem menuItems[] = {
		{ "Bones ESP",  &g_espBones  },
		{ "FOV Circle", &g_fovCircle },
		{ "Aimbot",     &g_aimbot    },
		{ "Team ESP",   &g_teamEsp   },
	};
	static const int menuItemCount = sizeof(menuItems) / sizeof(menuItems[0]);

	if (curIns && !prevIns) g_menuOpen = !g_menuOpen;
	if (g_menuOpen)
	{
		if (curUp    && !prevUp)    g_menuSel = (g_menuSel - 1 + menuItemCount) % menuItemCount;
		if (curDown  && !prevDown)  g_menuSel = (g_menuSel + 1) % menuItemCount;
		if (curEnter && !prevEnter) *menuItems[g_menuSel].value = !*menuItems[g_menuSel].value;
	}
	prevIns = curIns; prevUp = curUp; prevDown = curDown; prevEnter = curEnter;

	ViewMatrix viewMatrix = GetViewMatrix();

	pDevice->GetViewport(&vp);
	int screenW = (int)vp.Width;
	int screenH = (int)vp.Height;

	Entity localEnt(0);

	if (g_espBones)
	{
		for (int i = 1; i < 64; i++)
		{
			Entity entity(i);
			if (!entity.baseAddress) continue;
			if (IsBadReadPtr((void*)entity.baseAddress, sizeof(DWORD))) continue;
			int health = entity.GetHealth();
			if (health <= 1 || health > 100) continue;

			bool teammate = (localEnt.GetTeam() == entity.GetTeam());
			if (!g_teamEsp && teammate) continue;

			DrawBones(entity, viewMatrix, screenW, screenH, pLine, teammate ? GREEN : RED);
		}
	}

	if (g_aimbot && (GetAsyncKeyState(VK_LSHIFT) & 0x8000))
		RunAimbot(localEnt, viewMatrix, screenW, screenH, fovRadius);
	else
		g_aimActive = false;

	if (g_fovCircle)
		DrawCircle(screenW / 2.0f, screenH / 2.0f, fovRadius, 64, D3DCOLOR_ARGB(180, 255, 255, 255), pLine);

	if (g_menuOpen)
		DrawESPMenu(g_menuSel, menuItems, menuItemCount, font, pLine);

	return pEndScene(pDevice);
}

void hookEndScene() {
    IDirect3D9* pD3D = Direct3DCreate9(D3D_SDK_VERSION); // create IDirect3D9 object
    if (!pD3D)
        return;

    D3DPRESENT_PARAMETERS d3dparams = { 0 };
    d3dparams.SwapEffect = D3DSWAPEFFECT_DISCARD;
    d3dparams.hDeviceWindow = GetForegroundWindow();
    d3dparams.Windowed = true;

    IDirect3DDevice9* pDevice = nullptr;

    HRESULT result = pD3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, d3dparams.hDeviceWindow, D3DCREATE_SOFTWARE_VERTEXPROCESSING, &d3dparams, &pDevice);
    if (FAILED(result) || !pDevice) {
        pD3D->Release();
        return;
    }
    //if device creation worked out -> lets get the virtual table:
    void** vTable = *reinterpret_cast<void***>(pDevice);

    //now detour:

    pEndScene = (endScene)DetourFunction((PBYTE)vTable[42], (PBYTE)hookedEndScene);

    pDevice->Release();
    pD3D->Release();
}


DWORD __stdcall EjectThread(LPVOID lpParameter) {
    Sleep(100);
    FreeLibraryAndExitThread(DllHandle, 0);
    return 0;
}

DWORD WINAPI Menue(HINSTANCE hModule) {
    AllocConsole();
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout); //sets cout to be used with our newly created console

    hookEndScene();
    hookCreateMove();

    while (true) {
        Sleep(50);
        if (GetAsyncKeyState(VK_NUMPAD0)) {
            DetourRemove((PBYTE)pEndScene, (PBYTE)hookedEndScene);
            if (origCreateMove) DetourRemove((PBYTE)origCreateMove, (PBYTE)hookedCreateMove);
            break;
        }
        if (GetAsyncKeyState(VK_NUMPAD1) & 1) {
                EntityCount = EntityCount + 200;
            std::cout << "Count: " << EntityCount << std::endl;
        }
    }
    std::cout << "ight imma head out" << std::endl;
    Sleep(1000);
    fclose(fp);
    FreeConsole();
    CreateThread(0, 0, EjectThread, 0, 0, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        DllHandle = hModule;
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Menue, NULL, 0, NULL);
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}