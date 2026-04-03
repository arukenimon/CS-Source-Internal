#pragma once
#include <cfloat>
#include <cstring>
#include "Entity.h"
#include "Weapons.h"

enum AimMode { AIM_CROSSHAIR = 1, AIM_CLOSEST = 0 };

inline bool  g_aimActive       = false;
inline float g_aimPitch        = 0.0f;
inline float g_aimYaw          = 0.0f;
inline float g_speedMultiplier = 1.0f;

inline void SetViewAngles(float pitch, float yaw)
{
	Addresses addr;
	DWORD ptr = *(DWORD*)(addr.engineBase + addr.ViewAnglesAddr);
	*(float*)(ptr + 0x0) = pitch;
	*(float*)(ptr + 0x4) = yaw;
}

inline void SetServerPosition(float x, float y, float z)
{
	Addresses addr;
	if (!addr.serverBase) return;
	DWORD ptr = *(DWORD*)(addr.serverBase + addr.serverAddress);
	if (!ptr || IsBadWritePtr((void*)(ptr + addr.serverXYZ[0]), sizeof(float) * 3)) return;
	*(float*)(ptr + addr.serverXYZ[0]) = x;
	*(float*)(ptr + addr.serverXYZ[1]) = y;
	*(float*)(ptr + addr.serverXYZ[2]) = z;
}

inline void RunAimbot(Entity& localEnt, ViewMatrix vm, int screenW, int screenH, float fovRadius, AimMode aimMode = AIM_CROSSHAIR, float smooth = 1.0f)
{
	float centerX = screenW / 2.0f;
	float centerY = screenH / 2.0f;

	float bestScore  = (aimMode == AIM_CROSSHAIR) ? fovRadius : FLT_MAX;
	int   bestTarget = -1;

	Vector3 localPos = localEnt.GetEyePosition();

	for (int i = 1; i < 64; i++)
	{
		Entity entity(i);
		if (!entity.baseAddress) continue;
		if (IsBadReadPtr((void*)entity.baseAddress, sizeof(DWORD))) continue;

		if (entity.IsDormant()) continue;

		int health = entity.GetHealth();
		if (health <= 1 || health > 100) continue;

		if (localEnt.GetTeam() == entity.GetTeam()) continue;

		Vector3 headPos = entity.GetBonePosition(14);
		if (!isValid(headPos)) continue;

		float score;
		if (aimMode == AIM_CROSSHAIR)
		{
			Vector3 screenPos;
			if (!WorldToScreen(headPos, screenPos, vm, screenW, screenH)) continue;
			float dx = screenPos.x - centerX;
			float dy = screenPos.y - centerY;
			score = sqrtf(dx * dx + dy * dy);
		}
		else // AIM_CLOSEST
		{
			float dx = headPos.x - localPos.x;
			float dy = headPos.y - localPos.y;
			float dz = headPos.z - localPos.z;
			score = sqrtf(dx * dx + dy * dy + dz * dz);
		}

		if (score < bestScore)
		{
			bestScore  = score;
			bestTarget = i;
		}
	}

	if (bestTarget == -1) return;

	Entity  target  = Entity(bestTarget);
	Vector3 headPos = target.GetBonePosition(14);

	float dx     = headPos.x - localPos.x;
	float dy     = headPos.y - localPos.y;
	float dz     = headPos.z - localPos.z;
	float dist2d = sqrtf(dx * dx + dy * dy);

	float targetPitch = -atanf(dz / dist2d) * (180.0f / D3DX_PI);
	float targetYaw   =  atan2f(dy, dx)     * (180.0f / D3DX_PI);

	ViewAngles current    = GetViewAngles();
	float      deltaPitch = NormalizeAngle(targetPitch - current.m[0]);
	float      deltaYaw   = NormalizeAngle(targetYaw   - current.m[1]);

	g_aimPitch = current.m[0] + deltaPitch / smooth;
	g_aimYaw   = current.m[1] + deltaYaw   / smooth;

	if (target.GetHealth() <= 1 || target.GetHealth() > 100)
	{
		g_aimActive = false;
		return;
	}

	g_aimActive = true;
}

inline void RunTelekill(Entity& localEnt)
{
	Vector3 localPos = localEnt.GetEyePosition();

	float bestDist   = FLT_MAX;
	int   bestTarget = -1;

	for (int i = 1; i < 64; i++)
	{
		Entity entity(i);
		if (!entity.baseAddress || entity.baseAddress < 0x10000) continue;
		if (IsBadReadPtr((void*)entity.baseAddress, sizeof(DWORD))) continue;

		if (entity.IsDormant()) continue;

		int health = entity.GetHealth();
		if (health <= 1 || health > 100) continue;

		if (localEnt.GetTeam() == entity.GetTeam()) continue;

		Vector3 pos = entity.GetPosition();
		if (!isValid(pos)) continue;

		float dx   = pos.x - localPos.x;
		float dy   = pos.y - localPos.y;
		float dz   = pos.z - localPos.z;
		float dist = sqrtf(dx * dx + dy * dy + dz * dz);

		if (dist < bestDist)
		{
			bestDist   = dist;
			bestTarget = i;
		}
	}

	if (bestTarget == -1) return;

	Entity  target    = Entity(bestTarget);
	Vector3 targetPos = target.GetPosition();
	SetServerPosition(targetPos.x, targetPos.y, targetPos.z);
}

inline void RunFastKnife(Entity& localEnt)
{
	Addresses addr;

	DWORD weaponHandle = *(DWORD*)(localEnt.baseAddress + addr.m_hActiveWeapon);
	if (!weaponHandle || weaponHandle == 0xFFFFFFFF) return;

	int weaponIndex = weaponHandle & 0xFFF;
	if (weaponIndex <= 0 || weaponIndex >= 2048) return;

	// Look up the weapon entity on the server via gEntList.m_EntPtrArray[weaponIndex].m_pEntity
	DWORD serverWeapon = *(DWORD*)(addr.serverBase + addr.serverEntList + weaponIndex * 0x10);
	if (!serverWeapon || IsBadReadPtr((void*)serverWeapon, sizeof(DWORD))) return;

	// Only apply to the knife
	const char* pClassName = *(const char**)(serverWeapon + WEAPON_CLASSNAME_OFFSET);
	if (!pClassName || IsBadReadPtr(pClassName, 1)) return;
	if (strcmp(pClassName, "weapon_knife") != 0) return;

	if (IsBadWritePtr((void*)(serverWeapon + addr.sv_m_flNextPrimaryAttack), sizeof(float) * 3)) return;
	*(float*)(serverWeapon + addr.sv_m_flNextPrimaryAttack)   = 0.0f;
	*(float*)(serverWeapon + addr.sv_m_flNextSecondaryAttack) = 0.0f;
	*(float*)(serverWeapon + addr.sv_m_flTimeWeaponIdle)      = 0.0f;
}

inline void RunGodMode()
{
	Addresses addr;
	if (!addr.serverBase) return;
	DWORD sptr = *(DWORD*)(addr.serverBase + addr.serverAddress);
	if (!sptr || IsBadWritePtr((void*)(sptr + addr.serverHealthOffset), sizeof(int))) return;
	*(int*)(sptr + addr.serverHealthOffset) = 100;
}

inline void RunSpeedhack(Entity& localEnt, float multiplier)
{
	if (!localEnt.baseAddress || IsBadReadPtr((void*)localEnt.baseAddress, sizeof(DWORD))) return;

	Addresses addr;
	if (!addr.serverBase) return;

	DWORD sptr = *(DWORD*)(addr.serverBase + addr.serverAddress);
	if (!sptr || IsBadWritePtr((void*)(sptr + addr.VelocityXYZ), sizeof(float) * 2)) return;

	float* vxPtr = (float*)(sptr + addr.VelocityXYZ);
	float* vyPtr = (float*)(sptr + addr.VelocityXYZ + 0x04);

	float vx    = *vxPtr;
	float vy    = *vyPtr;
	float speed = sqrtf(vx * vx + vy * vy);

	if (speed < 1.0f) return;

	const float baseMaxSpeed = 300.0f;
	if (speed < baseMaxSpeed)
	{
		float nx = vx / speed;
		float ny = vy / speed;
		*vxPtr   = nx * (speed * multiplier);
		*vyPtr   = ny * (speed * multiplier);
	}
}
