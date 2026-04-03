#pragma once
#include "Math.h"

inline void* g_pEntityList  = nullptr;
inline void* g_pServerTools = nullptr;

class Entity
{
public:
	Addresses addresses;
	DWORD     baseAddress;

	Entity(int index)
	{
		baseAddress = *(DWORD*)(addresses.clientBase + addresses.entityList + (index * 0x4));
	}

	int GetHealth()
	{
		return *(int*)(baseAddress + addresses.health);
	}

	DWORD GetActiveWeapon()
	{
		if (!g_pEntityList) return 0;
		DWORD weaponHandle = *(DWORD*)(baseAddress + addresses.m_hActiveWeapon);
		if (!weaponHandle || weaponHandle == 0xFFFFFFFF) return 0;

		// IClientEntityList vtable[2] = GetClientEntityFromHandle(EHANDLE)
		typedef void*(__thiscall* GetEntityFromHandleFn)(void*, DWORD);
		auto fn = (*(GetEntityFromHandleFn**)g_pEntityList)[2];
		void* ent = fn(g_pEntityList, weaponHandle);

		if (!ent || IsBadReadPtr(ent, sizeof(DWORD))) return 0;
		return (DWORD)ent;
	}

	bool IsDormant()
	{
		Entity localEnt(0);
		float val = *(float*)(baseAddress + addresses.dormant);
		return val != *(float*)(localEnt.baseAddress + addresses.dormant);
	}

	Vector3 GetPosition()
	{
		Vector3 pos;
		pos.x = *(float*)(baseAddress + addresses.XYZ[0]);
		pos.y = *(float*)(baseAddress + addresses.XYZ[1]);
		pos.z = *(float*)(baseAddress + addresses.XYZ[2]);
		return pos;
	}

	int GetTeam()
	{
		return *(int*)(baseAddress + addresses.team);
	}

	Velocity GetVelocity()
	{
		Addresses addr;
		Velocity vel;
		memcpy(vel.m, (void*)(this->baseAddress + addr.VelocityXYZ), sizeof(float) * 3);
		return vel;
	}

	Vector3 GetEyePosition()
	{
		Vector3 pos = GetPosition();
		pos.z += *(float*)(baseAddress + addresses.eye);
		return pos;
	}

	Vector3 GetBonePosition(int boneIndex)
	{
		DWORD boneBase = *(DWORD*)(this->baseAddress + addresses.bonePtr[0]);
		if (!boneBase || IsBadReadPtr((void*)boneBase, sizeof(float) * 3))
			return { 0.f, 0.f, 0.f };

		DWORD bone = boneBase + (boneIndex * 0x30);

		Vector3 pos;
		pos.x = *(float*)(bone + 0x0C);
		pos.y = *(float*)(bone + 0x1C);
		pos.z = *(float*)(bone + 0x2C);
		return pos;
	}
};
