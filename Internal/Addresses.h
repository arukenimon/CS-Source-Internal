#pragma once
#include <Windows.h>

struct Addresses
{
	DWORD clientBase = (DWORD)GetModuleHandleA("client.dll");
	DWORD engineBase = (DWORD)GetModuleHandleA("engine.dll");
	DWORD serverBase = (DWORD)GetModuleHandleA("server.dll");
	DWORD entityList        = 0x004E5B14;
	DWORD viewMatrixOffset  = 0x50;
	DWORD engineOffset      = 0x004FF224;
	DWORD health            = 0x8C;
	DWORD XYZ[3]            = { 0x258, 0x25C, 0x260 };
	DWORD bonePtr[1]        = { 0x570 };
	DWORD team              = 0x94;
	DWORD dormant           = 0x60;
	DWORD eye               = 0xE8;
	DWORD ViewAnglesAddr    = 0x000A5904;
	DWORD VelocityXYZ       = 0x218; // + 0x04, + 0x08

	DWORD m_hActiveWeapon   = 0xD78;
	DWORD m_hMyWeapons      = 0xCC0; // local player only; array of 4 handles, each index into the entity list (handle & 0xFFF)

	DWORD knife_BaseAddress          = 0x004E5E6C;
	DWORD sv_m_flNextPrimaryAttack   = 0x46C;
	DWORD sv_m_flNextSecondaryAttack = 0x470;
	DWORD sv_m_flTimeWeaponIdle      = 0x474;

	DWORD serverHealthOffset = 0xE4;
	DWORD serverAddress      = 0x004F615C; // server-side local player entity pointer
	DWORD serverXYZ[3]       = { 0x308, 0x308 + 4, 0x308 + 8 };
	// gEntList base: server.dll + 0x004F6148; m_EntPtrArray starts at +0x4 = 0x004F614C
	// Each CEntInfo = 0x10 bytes; m_pEntity is at offset 0 within each entry
	DWORD serverEntList      = 0x004F614C;
};
