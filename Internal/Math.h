#pragma once
#include <d3dx9.h>
#include "Addresses.h"

struct Vector3
{
	float x, y, z;
};

struct ViewMatrix
{
	float m[16];
	float operator[](int i) const { return m[i]; }
};

struct ViewAngles
{
	float m[2];
	float operator[](int i) const { return m[i]; }
};

struct Velocity
{
	float m[3];
	float operator[](int i) const { return m[i]; }
};

inline bool isValid(Vector3 v)
{
	return !(v.x == 0 && v.y == 0 && v.z == 0) &&
		v.x > -10000 && v.x < 10000 &&
		v.y > -10000 && v.y < 10000 &&
		v.z > -10000 && v.z < 10000;
}

inline float NormalizeAngle(float angle)
{
	while (angle >  180.0f) angle -= 360.0f;
	while (angle < -180.0f) angle += 360.0f;
	return angle;
}

inline ViewMatrix GetViewMatrix()
{
	Addresses addr;
	DWORD ptr = *(DWORD*)(addr.clientBase + addr.engineOffset);
	ViewMatrix vm;
	memcpy(vm.m, (void*)(ptr + addr.viewMatrixOffset), sizeof(float) * 16);
	return vm;
}

inline ViewAngles GetViewAngles()
{
	Addresses addr;
	DWORD ptr = *(DWORD*)(addr.engineBase + addr.ViewAnglesAddr);
	ViewAngles va;
	memcpy(va.m, (void*)(ptr + 0x0), sizeof(float) * 2);
	return va;
}

inline bool WorldToScreen(Vector3 worldPos, Vector3& screenPos, ViewMatrix vm, int screenWidth, int screenHeight)
{
	float clipX = vm.m[0]  * worldPos.x + vm.m[1]  * worldPos.y + vm.m[2]  * worldPos.z + vm.m[3];
	float clipY = vm.m[4]  * worldPos.x + vm.m[5]  * worldPos.y + vm.m[6]  * worldPos.z + vm.m[7];
	float clipZ = vm.m[8]  * worldPos.x + vm.m[9]  * worldPos.y + vm.m[10] * worldPos.z + vm.m[11];
	float clipW = vm.m[12] * worldPos.x + vm.m[13] * worldPos.y + vm.m[14] * worldPos.z + vm.m[15];

	if (clipW < 0.01f)
		return false;

	float ndcX = clipX / clipW;
	float ndcY = clipY / clipW;
	float ndcZ = clipZ / clipW;

	if (ndcX < -1.0f || ndcX > 1.0f ||
		ndcY < -1.0f || ndcY > 1.0f)
		return false;

	screenPos.x = (screenWidth  / 2.0f) * (1.0f + ndcX);
	screenPos.y = (screenHeight / 2.0f) * (1.0f - ndcY);
	screenPos.z = ndcZ;

	return true;
}
