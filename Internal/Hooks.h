#pragma once
#include <cstdio>
#include "Features.h"
#include "detours.h"

// CS:S m_angEyeAngles offsets on the local player entity
#define EYE_PITCH_OFFSET 0x12C
#define EYE_YAW_OFFSET   0x130

typedef void(__thiscall* CreateMoveFn)(void*, int, float, bool);
inline CreateMoveFn origCreateMove = nullptr;

inline void __fastcall hookedCreateMove(void* thisptr, void* /*edx*/, int seqnum, float frametime, bool active)
{
	origCreateMove(thisptr, seqnum, frametime, active);

	Entity local(0);
	if (!local.baseAddress || IsBadReadPtr((void*)local.baseAddress, sizeof(DWORD))) return;

	if (g_aimActive)
	{
		Addresses addr;
		DWORD ptr = *(DWORD*)(addr.engineBase + addr.ViewAnglesAddr);
		*(float*)(ptr + 0x0) = g_aimPitch;
		*(float*)(ptr + 0x4) = g_aimYaw;
	}
}

// Obtains IClientEntityList from client.dll; call once at startup
inline void InitEntityList()
{
	typedef void*(*CreateInterfaceFn)(const char*, int*);
	auto createIF = (CreateInterfaceFn)GetProcAddress(GetModuleHandleA("client.dll"), "CreateInterface");
	if (!createIF) { printf("[EntList] CreateInterface not found\n"); return; }
	g_pEntityList = createIF("VClientEntityList003", nullptr);
	printf("[EntList] IClientEntityList = 0x%X\n", (DWORD)g_pEntityList);
}

// Obtains IServerTools from server.dll; call once at startup
inline void InitServerTools()
{
	typedef void*(*CreateInterfaceFn)(const char*, int*);
	auto createIF = (CreateInterfaceFn)GetProcAddress(GetModuleHandleA("server.dll"), "CreateInterface");
	if (!createIF) { printf("[ServerTools] CreateInterface not found\n"); return; }
	g_pServerTools = createIF("VClientEntityList003", nullptr);
	printf("[ServerTools] IServerTools = 0x%X\n", (DWORD)g_pServerTools);
}

// Hooks IBaseClientDLL::CreateMove (vtable index 21) via Detours; call once at startup
inline void hookCreateMove()
{
	typedef void*(*CreateInterfaceFn)(const char*, int*);
	auto createIF = (CreateInterfaceFn)GetProcAddress(GetModuleHandleA("client.dll"), "CreateInterface");
	if (!createIF) { printf("[CreateMove] CreateInterface not found\n"); return; }

	// CS:S uses VClient017; try VClient014–VClient016 if this fails
	void* pClient = createIF("VClient017", nullptr);
	if (!pClient) { printf("[CreateMove] Interface not found\n"); return; }

	void** vTable = *(void***)pClient;
	// vtable index 21 = CreateMove for CS:S; try 22 if it doesn't work
	origCreateMove = (CreateMoveFn)DetourFunction((PBYTE)vTable[21], (PBYTE)hookedCreateMove);
	printf("[CreateMove] Hooked\n");
}
