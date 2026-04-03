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
bool g_snaplines = false;
bool g_espBox    = false;
bool g_showIndex = false;
int  g_menuSel   = 1;
int  g_aimMode   = AIM_CLOSEST;
int  g_fovPreset = 0;
bool g_telekill  = true;
bool g_speedhack  = false;
int  g_speedPreset = 0;
bool g_godMode     = false;
bool g_fastKnife   = false;
D3DVIEWPORT9 vp;
HRESULT __stdcall hookedEndScene(IDirect3DDevice9* pDevice) {
	if (!font)
		D3DXCreateFont(pDevice, 14, 0, FW_BOLD, 1, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Arial", &font);
	if (!pLine)
		D3DXCreateLine(pDevice, &pLine);

	// --- menu key handling (edge-triggered) ---
	static bool prevIns = false, prevUp = false, prevDown = false, prevEnter = false, prevLeft = false, prevRight = false;
	bool curIns   = (GetAsyncKeyState(VK_INSERT) & 0x8000) != 0;
	bool curUp    = (GetAsyncKeyState(VK_UP)     & 0x8000) != 0;
	bool curDown  = (GetAsyncKeyState(VK_DOWN)   & 0x8000) != 0;
	bool curEnter = (GetAsyncKeyState(VK_RETURN) & 0x8000) != 0;
	bool curLeft  = (GetAsyncKeyState(VK_LEFT)   & 0x8000) != 0;
	bool curRight = (GetAsyncKeyState(VK_RIGHT)  & 0x8000) != 0;

	static const char* aimModeLabels[]   = {  "Distance","Crosshair" };
	static const char* fovPresetLabels[] = { "180", "250", "360", "450" };
	static const float fovPresets[]      = { 180.0f, 250.0f, 360.0f, 450.0f };
	static const char* speedLabels[]     = { "1.5x", "2.0x", "2.5x", "3.0x", "20x" };
	static const float speedValues[]     = { 1.5f, 2.0f, 2.5f, 3.0f, 20.0f };
	static MenuItem menuItems[] = {
		{ "ESP",        nullptr,      nullptr,    0,    0,         0 },
		{ "Bones ESP",  &g_espBones,  nullptr,    0,    0,         0 },
		{ "ESP Box",    &g_espBox,    nullptr,    0,    0,         0 },
		{ "Snaplines",  &g_snaplines, nullptr,    0,    0,         0 },
		{ "Team ESP",   &g_teamEsp,   nullptr,    0,    0,         0 },
		{ "Index",      &g_showIndex, nullptr,    0,    0,         0 },
		{ "Aimbot",     nullptr,      nullptr,    0,    0,         0 },
		{ "Aimbot",     &g_aimbot,    nullptr,    0,    0,         0 },
		{ "Aim Mode",   nullptr,      nullptr,    0,    0,         0, &g_aimMode,    2, aimModeLabels  },
		{ "FOV Circle", &g_fovCircle, nullptr,    0,    0,         0 },
		{ "FOV Radius", nullptr,      nullptr,    0,    0,         0, &g_fovPreset,  4, fovPresetLabels },
		{ "Weapon",     nullptr,      nullptr,    0,    0,         0 },
		{ "Fast Knife", &g_fastKnife, nullptr,    0,    0,         0 },
		{ "Movement",   nullptr,      nullptr,    0,    0,         0 },
		{ "Speedhack",  &g_speedhack, nullptr,    0,    0,         0 },
		{ "Speed",      nullptr,      nullptr,    0,    0,         0, &g_speedPreset, 5, speedLabels    },
		{ "Misc",       nullptr,      nullptr,    0,    0,         0 },
		{ "Telekill",   &g_telekill,  nullptr,    0,    0,         0 },
		{ "God Mode",   &g_godMode,   nullptr,    0,    0,         0 },
	};
	static const int menuItemCount = sizeof(menuItems) / sizeof(menuItems[0]);

	if (curIns && !prevIns) g_menuOpen = !g_menuOpen;
	if (g_menuOpen)
	{
		if (curUp && !prevUp) {
			do { g_menuSel = (g_menuSel - 1 + menuItemCount) % menuItemCount; }
			while (!menuItems[g_menuSel].bvalue && !menuItems[g_menuSel].fvalue && !menuItems[g_menuSel].ivalue);
		}
		if (curDown && !prevDown) {
			do { g_menuSel = (g_menuSel + 1) % menuItemCount; }
			while (!menuItems[g_menuSel].bvalue && !menuItems[g_menuSel].fvalue && !menuItems[g_menuSel].ivalue);
		}

		MenuItem& item = menuItems[g_menuSel];
		if (item.bvalue && curEnter && !prevEnter) *item.bvalue = !*item.bvalue;
		if (item.fvalue)
		{
			if (curLeft  && !prevLeft)  *item.fvalue = max(item.fmin, *item.fvalue - item.fstep);
			if (curRight && !prevRight) *item.fvalue = min(item.fmax, *item.fvalue + item.fstep);
		}
		if (item.ivalue)
		{
			if (curLeft  && !prevLeft)  *item.ivalue = (*item.ivalue - 1 + item.imax) % item.imax;
			if (curRight && !prevRight) *item.ivalue = (*item.ivalue + 1) % item.imax;
		}
	}
	prevIns = curIns; prevUp = curUp; prevDown = curDown; prevEnter = curEnter; prevLeft = curLeft; prevRight = curRight;

	fovRadius = fovPresets[g_fovPreset];

	ViewMatrix viewMatrix = GetViewMatrix();

	pDevice->GetViewport(&vp);
	int screenW = (int)vp.Width;
	int screenH = (int)vp.Height;

	Entity localEnt(0);
	if (!localEnt.baseAddress || IsBadReadPtr((void*)localEnt.baseAddress, sizeof(DWORD)))
		return pEndScene(pDevice);

	if (g_espBones || g_snaplines || g_espBox || g_showIndex)
	{
		for (int i = 1; i < 64; i++)
		{
			Entity entity(i);
			if (!entity.baseAddress || entity.baseAddress < 0x10000) continue;
			if (IsBadReadPtr((void*)entity.baseAddress, sizeof(DWORD))) continue;
			if (entity.IsDormant()) continue;
			int health = entity.GetHealth();


			if (health <= 1 || health > 100) continue;

			bool teammate = (localEnt.GetTeam() == entity.GetTeam());
			if (!g_teamEsp && teammate) continue;

			D3DCOLOR color = teammate ? GREEN : RED;
			if (g_espBones)    DrawBones(entity, viewMatrix, screenW, screenH, pLine, color);
			if (g_espBox)      DrawESPBox(entity, viewMatrix, screenW, screenH, pLine, color);
			if (g_snaplines)   DrawSnapline(entity, viewMatrix, screenW, screenH, pLine, color);
			if (g_showIndex)   DrawEntityIndex(entity, i, viewMatrix, screenW, screenH, font);
			///DrawBoneNumbers(entity, viewMatrix, screenW, screenH, font);
		}
	}

	if(GetAsyncKeyState(VK_NUMPAD5) & 1) {
		SetServerPosition(480.362f, 2485.217f, -110.20211f);
	}
	if (GetAsyncKeyState(VK_NUMPAD6) & 1) {
		SetServerPosition(-529.8451f, -792.0601f, 132.0478f);
	}

	if (g_aimbot && (GetAsyncKeyState(VK_LSHIFT) & 0x8000))
		RunAimbot(localEnt, viewMatrix, screenW, screenH, fovRadius, (AimMode)g_aimMode);
	else
		g_aimActive = false;

	if (g_telekill && (GetAsyncKeyState('F') & 0x8000))
		RunTelekill(localEnt);

	if (g_godMode)
		RunGodMode();

	if (g_fastKnife)
		RunFastKnife(localEnt);

	g_speedMultiplier = g_speedhack ? speedValues[g_speedPreset] : 1.0f;

	if (g_speedhack) {
		if (g_speedMultiplier > 1.0f)
			RunSpeedhack(localEnt, g_speedMultiplier);
	}

	if (g_fovCircle)
		DrawCircle(screenW / 2.0f, screenH / 2.0f, fovRadius, 64, D3DCOLOR_ARGB(180, 255, 255, 255), pLine);

	if (g_menuOpen)
		DrawESPMenu(g_menuSel, menuItems, menuItemCount, font, pLine);

	Velocity vel = localEnt.GetVelocity();

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
	InitEntityList();
	InitServerTools();

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