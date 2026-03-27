#pragma once
#include <iostream>
#include <cstdio>

#include <d3d9.h>
#include <d3dx9.h>

#define GREEN D3DCOLOR_ARGB(255, 0, 255, 0)
#define RED D3DCOLOR_ARGB(255, 255, 0, 0)

enum AimMode { AIM_CROSSHAIR = 0, AIM_CLOSEST = 1 };

struct Vector3
{
	float x, y, z;
};
struct Addresses
{
	DWORD clientBase = (DWORD)GetModuleHandleA("client.dll");
	DWORD engineBase = (DWORD)GetModuleHandleA("engine.dll");
	DWORD serverBase = (DWORD)GetModuleHandleA("server.dll");
	DWORD baseOffset = 0x004E5B14;
	DWORD viewMatrixOffset = 0x50;
	DWORD engineOffset = 0x004FF224;
	DWORD health = 0x8C;
	DWORD XYZ[3] = { 0x258, 0x25C,0x260 };
	DWORD bonePtr[1] = { 0x570 };
	DWORD team = 0x94;
	DWORD dormant = 0xE8;
	DWORD eye = 0xE8;
	DWORD ViewAnglesAddr = 0x000A5904;



	DWORD serverAddress = 0x004F615C;
	DWORD serverXYZ[3] = { 0x308, 0x308+4, 0x308+8 };
};

struct ViewMatrix
{
	float m[16];

	float operator[](int i) const { return m[i]; }
};

// returns the view matrix from the engine module (16 floats = 64 bytes)
ViewMatrix GetViewMatrix()
{
	Addresses addr;
	DWORD ptr = *(DWORD*)(addr.clientBase + addr.engineOffset);
	ViewMatrix vm;
	memcpy(vm.m, (void*)(ptr + addr.viewMatrixOffset), sizeof(float) * 16);
	return vm;
}

struct ViewAngles {
	float m[2];
	float operator[](int i) const { return m[i]; }
};

ViewAngles GetViewAngles()
{
	Addresses addr;
	DWORD ptr = *(DWORD*)(addr.engineBase + addr.ViewAnglesAddr);
	ViewAngles va;
	memcpy(va.m, (void*)(ptr + 0x0), sizeof(float) * 2);
	return va;
}

class Entity
{
public:
	Addresses addresses;
	DWORD baseAddress;

	Entity(int index)
	{
		baseAddress = *(DWORD*)(addresses.clientBase + addresses.baseOffset + (index * 0x4));
	}

	int GetHealth()
	{
		return *(int*)(baseAddress + addresses.health);
	}

	int IsDormant()
	{
		return *(int*)(baseAddress + addresses.dormant); // this is a common pattern for checking if an entity is dormant (not active in the game world), returns true if dormant, false if active
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


	Vector3 GetEyePosition()
	{
		Vector3 pos = GetPosition();
		pos.z += *(float*)(baseAddress + addresses.eye); // typical eye height in CS:S
		return pos;
	}

	// This means that each bone struct is 0x10 bytes apart in memory, and the position of each bone is stored at offsets 0, 0x16, and 0x2C within that struct (just an example, the actual offsets depend on the game's data structures)
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





// projects a 3D world position to 2D screen coordinates
// returns false if the point is behind the camera
auto isValid = [](Vector3 v) {
	return !(v.x == 0 && v.y == 0 && v.z == 0) &&
		v.x > -10000 && v.x < 10000 &&
		v.y > -10000 && v.y < 10000 &&
		v.z > -10000 && v.z < 10000;
	};
bool WorldToScreen(Vector3 worldPos, Vector3& screenPos, ViewMatrix vm, int screenWidth, int screenHeight)
{
	float clipX = vm.m[0] * worldPos.x + vm.m[1] * worldPos.y + vm.m[2] * worldPos.z + vm.m[3];
	float clipY = vm.m[4] * worldPos.x + vm.m[5] * worldPos.y + vm.m[6] * worldPos.z + vm.m[7];
	float clipZ = vm.m[8] * worldPos.x + vm.m[9] * worldPos.y + vm.m[10] * worldPos.z + vm.m[11];
	float clipW = vm.m[12] * worldPos.x + vm.m[13] * worldPos.y + vm.m[14] * worldPos.z + vm.m[15];

	if (clipW < 0.01f)
		return false;

	float ndcX = clipX / clipW;
	float ndcY = clipY / clipW;
	float ndcZ = clipZ / clipW;

	// 🔴 THIS FIXES YOUR BUG
	if (ndcX < -1.0f || ndcX > 1.0f ||
		ndcY < -1.0f || ndcY > 1.0f)
		return false;

	screenPos.x = (screenWidth / 2.0f) * (1.0f + ndcX);
	screenPos.y = (screenHeight / 2.0f) * (1.0f - ndcY);
	screenPos.z = ndcZ;

	return true;
}

void DrawString(const char* text, int x, int y, int a, int r, int g, int b, LPD3DXFONT font)
{
	RECT rect;
	SetRect(&rect, x, y, x + 200, y + 20);
	font->DrawTextA(NULL, text, -1, &rect, DT_NOCLIP, D3DCOLOR_ARGB(a, r, g, b));
}

// draws the bone index number at each of the 40 bone positions on screen
void DrawBoneNumbers(Entity& entity, ViewMatrix vm, int screenW, int screenH, LPD3DXFONT font)
{
	for (int i = 0; i < 40; i++)
	{
		Vector3 bonePos = entity.GetBonePosition(i);
		Vector3 screenPos;
		if (WorldToScreen(bonePos, screenPos, vm, screenW, screenH))
		{
			char buf[8];
			sprintf_s(buf, sizeof(buf), "%d", i);
			DrawString(buf, (int)screenPos.x, (int)screenPos.y, 255, 255, 255, 255, font);
		}
	}
}

void DrawLine(float x1, float y1, float x2, float y2, D3DCOLOR color, ID3DXLine* pLine)
{
	D3DXVECTOR2 points[2] = { { x1, y1 }, { x2, y2 } };
	pLine->Begin();
	pLine->Draw(points, 2, color);
	pLine->End();
}

// draws a circle approximated with line segments at screen position (cx, cy)
void DrawCircle(float cx, float cy, float radius, int segments, D3DCOLOR color, ID3DXLine* pLine)
{
	for (int i = 0; i < segments; i++)
	{
		float a1 = (2.0f * D3DX_PI * i)       / segments;
		float a2 = (2.0f * D3DX_PI * (i + 1)) / segments;
		DrawLine(
			cx + radius * cosf(a1), cy + radius * sinf(a1),
			cx + radius * cosf(a2), cy + radius * sinf(a2),
			color, pLine);
	}
}

// draws a dynamic-radius circle around bone 14 (head)
// radius is derived by projecting a point 6 world units above the head
void DrawHeadCircle(Entity& entity, ViewMatrix vm, int screenW, int screenH, ID3DXLine* pLine, D3DCOLOR color)
{
	Vector3 headWorld = entity.GetBonePosition(14);
	if (!isValid(headWorld)) return;

	Vector3 headScreen;
	if (!WorldToScreen(headWorld, headScreen, vm, screenW, screenH)) return;

	// offset 6 units up in world space to derive a screen-space radius
	Vector3 topWorld  = { headWorld.x, headWorld.y, headWorld.z + 6.0f };
	Vector3 topScreen;
	float radius = 8.0f; // fallback if top point is off-screen
	if (WorldToScreen(topWorld, topScreen, vm, screenW, screenH))
		radius = max(3.0f, fabsf(headScreen.y - topScreen.y));

	DrawCircle(headScreen.x, headScreen.y, radius, 20, color, pLine);
}



// connects key bones to draw a skeleton outline
void DrawBones(Entity& entity, ViewMatrix vm, int screenW, int screenH, ID3DXLine* pLine, D3DCOLOR color)
{
	static const int bonePairs[][2] = {
		// spine
		{ 0, 9 }, { 9, 10 }, { 10, 11 }, { 11, 13 }, { 13, 14 },
		// pelvis
		{ 0, 2 },
		// left arm
		{ 11, 29 }, { 29, 30 }, {30, 31},
		// right arm
		{ 11, 16 }, { 16, 17 }, {17, 18},
		// left leg
		{ 0, 6 }, { 6, 7 }, { 7, 8 },
		// right leg
		{ 2, 3, }, { 3,4 },
	};

	int pairCount = sizeof(bonePairs) / sizeof(bonePairs[0]);

	for (int i = 0; i < pairCount; i++)
	{
		Vector3 w1 = entity.GetBonePosition(bonePairs[i][0]);
		Vector3 w2 = entity.GetBonePosition(bonePairs[i][1]);

		if (!isValid(w1) || !isValid(w2))
			continue;

		Vector3 s1, s2;

		if (!WorldToScreen(w1, s1, vm, screenW, screenH)) continue;
		if (!WorldToScreen(w2, s2, vm, screenW, screenH)) continue;

		DrawLine(s1.x, s1.y, s2.x, s2.y, color, pLine);
	}

	DrawHeadCircle(entity, vm, screenW, screenH, pLine, color);
}

void DrawSnapline(Entity& entity, ViewMatrix vm, int screenW, int screenH, ID3DXLine* pLine, D3DCOLOR color)
{
	Vector3 worldPos = entity.GetPosition();
	if (!isValid(worldPos)) return;

	Vector3 screenPos;
	if (!WorldToScreen(worldPos, screenPos, vm, screenW, screenH)) return;

	DrawLine(screenW / 2.0f, (float)screenH, screenPos.x, screenPos.y, color, pLine);
}

void DrawESPBox(Entity& entity, ViewMatrix vm, int screenW, int screenH, ID3DXLine* pLine, D3DCOLOR color)
{
	Vector3 footWorld = entity.GetPosition();
	Vector3 headWorld = entity.GetBonePosition(14);

	if (!isValid(footWorld) || !isValid(headWorld)) return;

	Vector3 footScreen, headScreen;
	if (!WorldToScreen(footWorld, footScreen, vm, screenW, screenH)) return;
	if (!WorldToScreen(headWorld, headScreen, vm, screenW, screenH)) return;

	float height = footScreen.y - headScreen.y;
	if (height <= 0) return;

	float width = height * 0.4f;
	float cx    = (footScreen.x + headScreen.x) * 0.5f;

	float x1 = cx - width * 0.5f;
	float x2 = cx + width * 0.5f;
	float y1 = headScreen.y - 4.0f; // small padding above head
	float y2 = footScreen.y;

	DrawLine(x1, y1, x2, y1, color, pLine); // top
	DrawLine(x1, y2, x2, y2, color, pLine); // bottom
	DrawLine(x1, y1, x1, y2, color, pLine); // left
	DrawLine(x2, y1, x2, y2, color, pLine); // right
}

void DrawFilledRect(float x, float y, float w, float h, D3DCOLOR color, ID3DXLine* pLine)
{
	pLine->SetWidth(h);
	D3DXVECTOR2 pts[2] = { { x, y + h * 0.5f }, { x + w, y + h * 0.5f } };
	pLine->Begin();
	pLine->Draw(pts, 2, color);
	pLine->End();
	pLine->SetWidth(1.0f);
}

struct MenuItem {
	const char*  label;
	bool*        bvalue;   // non-null for toggle items
	float*       fvalue;   // non-null for float slider items
	float        fstep;
	float        fmin;
	float        fmax;
	int*         ivalue;   // non-null for cycle items
	int          imax;     // number of options
	const char** ilabels;  // display name for each option index
};

void DrawESPMenu(int sel, MenuItem* items, int count, LPD3DXFONT font, ID3DXLine* pLine)
{
	const float x = 20.0f, y = 50.0f, w = 200.0f, rowH = 18.0f, pad = 3.0f;
	float totalH = rowH * (count + 2.0f);

	DrawFilledRect(x, y, w, totalH, D3DCOLOR_ARGB(180, 15, 15, 15), pLine); // This is the background rectangle for the menu, it uses a very dark color with some transparency (ARGB(180, 15, 15, 15)) to create a solid background that makes the menu items more readable against the game visuals. The rectangle is drawn at position (x, y) with width w and height totalH, which is calculated based on the number of menu items plus some extra space for the title and footer.

	D3DCOLOR border = D3DCOLOR_ARGB(255, 80, 130, 255);
	DrawLine(x,     y,          x + w, y,          border, pLine);
	DrawLine(x,     y + totalH, x + w, y + totalH, border, pLine);
	DrawLine(x,     y,          x,     y + totalH, border, pLine);
	DrawLine(x + w, y,          x + w, y + totalH, border, pLine);

	DrawString("  REV hacks", (int)(x + 4), (int)(y + pad), 255, 120, 180, 255, font);
	DrawLine(x, y + rowH, x + w, y + rowH, border, pLine);

	for (int i = 0; i < count; i++)
	{
		float iy = y + rowH + (i * rowH);
		bool sep = !items[i].bvalue && !items[i].fvalue && !items[i].ivalue;

		// This checks if the current item is selected (i == sel) and is not a separator (sep == false). If both conditions are true, it draws a filled rectangle behind the menu item to highlight it. The rectangle is slightly smaller than the full width of the menu (w - 2) to create a border effect, and it uses a semi-transparent color to indicate selection.
		if (!sep && i == sel)
			DrawFilledRect(x + 1, iy, w - 2, rowH, D3DCOLOR_ARGB(140, 60, 80, 200), pLine); // this is the highlight for the selected menu item, it is a color with some transparency (ARGB) that creates a blueish background behind the text of the selected item. The rectangle is drawn slightly smaller than the full width of the menu to create a border effect, and it is only drawn if the current item is not a separator (sep == false) to avoid highlighting separators.

		char buf[64];
		int ca = 255, cr, cg, cb = 80;
		// This checks if the current menu item is a separator (sep == true). If it is, it draws a filled rectangle with a different color to visually separate groups of menu items. The text for a separator is just the label with some padding, and it uses a lighter color (cr = 130, cg = 185, cb = 255) to distinguish it from regular items.
		if (sep)
		{
			DrawFilledRect(x + 1, iy, w - 2, rowH, D3DCOLOR_ARGB(180, 15, 15, 15), pLine); // This uses the color ARGB(180, 15, 15, 15) to draw a filled rectangle behind the separator text. The color has an alpha value of 180 for some transparency, and it has a dark hue (15 red, 15 green, 15 blue) to visually distinguish it from regular menu items. This rectangle is drawn only for separator items (where sep == true) to create a visual separation between groups of menu items.
			sprintf_s(buf, sizeof(buf), "  %s", items[i].label);
			cr = 130; cg = 185; cb = 255;
		}
		else if (items[i].bvalue)
		{
			bool on = *items[i].bvalue;
			sprintf_s(buf, sizeof(buf), " %s %-14s [%s]",
				(i == sel) ? ">" : " ", items[i].label, on ? "ON" : "OFF");
			cr = on ? 80 : 220;
			cg = on ? 220 : 80;
		}
		else if (items[i].ivalue)
		{
			sprintf_s(buf, sizeof(buf), " %s %-14s [%s]",
				(i == sel) ? ">" : " ", items[i].label, items[i].ilabels[*items[i].ivalue]);
			cr = 200; cg = 200;
		}
		else
		{
			sprintf_s(buf, sizeof(buf), " %s %-14s [%.0f]",
				(i == sel) ? ">" : " ", items[i].label, *items[i].fvalue);
			cr = 220; cg = 200;
		}
		DrawString(buf, (int)(x + 4), (int)(iy + pad), ca, cr, cg, cb, font);
	}

	float hy = y + rowH * (count + 1.0f);
	DrawString("  ENTER=Toggle  < >=Adjust", (int)(x + 4), (int)(hy + pad), 160, 160, 160, 160, font);
}


void SetViewAngles(float pitch, float yaw)
{
	Addresses addr;
	DWORD ptr = *(DWORD*)(addr.engineBase + addr.ViewAnglesAddr);
	*(float*)(ptr + 0x0) = pitch;
	*(float*)(ptr + 0x4) = yaw;
}

float NormalizeAngle(float angle)
{
	while (angle > 180.0f)  angle -= 360.0f;
	while (angle < -180.0f) angle += 360.0f;
	return angle;
}

typedef void(__thiscall* CreateMoveFn)(void*, int, float, bool);
inline CreateMoveFn origCreateMove = nullptr;
inline bool  g_aimActive = false;
inline float g_aimPitch  = 0.0f;
inline float g_aimYaw    = 0.0f;

void RunAimbot(Entity& localEnt, ViewMatrix vm, int screenW, int screenH, float fovRadius, AimMode aimMode = AIM_CROSSHAIR, float smooth = 1.0f)
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

		int health = entity.GetHealth();
		if (health <= 1 || health > 100) continue;

		if (localEnt.GetTeam() == entity.GetTeam()) continue;

		Vector3 headPos = entity.GetBonePosition(14);
		if (!isValid(headPos)) continue;

		Vector3 screenPos;
		if (!WorldToScreen(headPos, screenPos, vm, screenW, screenH)) continue;

		float score;
		if (aimMode == AIM_CROSSHAIR)
		{
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

	Entity target(bestTarget);
	Vector3 headPos  = target.GetBonePosition(14);

	float dx     = headPos.x - localPos.x;
	float dy     = headPos.y - localPos.y;
	float dz     = headPos.z - localPos.z;
	float dist2d = sqrtf(dx * dx + dy * dy);

	float targetPitch = -atanf(dz / dist2d) * (180.0f / D3DX_PI);
	float targetYaw   =  atan2f(dy, dx)     * (180.0f / D3DX_PI);

	ViewAngles current = GetViewAngles();
	float deltaPitch = NormalizeAngle(targetPitch - current.m[0]);
	float deltaYaw   = NormalizeAngle(targetYaw   - current.m[1]);

	g_aimPitch  = current.m[0] + deltaPitch / smooth;
	g_aimYaw    = current.m[1] + deltaYaw   / smooth;

	if (target.GetHealth() <= 1 || target.GetHealth() > 100) {
		g_aimActive = false;
		return;
	}

	g_aimActive = true;
}

void SetServerPosition(float x, float y, float z)
{
	Addresses addr;
	if (!addr.serverBase) return;
	DWORD ptr = *(DWORD*)(addr.serverBase + addr.serverAddress);
	if (!ptr || IsBadWritePtr((void*)(ptr + addr.serverXYZ[0]), sizeof(float) * 3)) return;
	*(float*)(ptr + addr.serverXYZ[0]) = x;
	*(float*)(ptr + addr.serverXYZ[1]) = y;
	*(float*)(ptr + addr.serverXYZ[2]) = z;
}

void RunTelekill(Entity& localEnt)
{
	Vector3 localPos = localEnt.GetEyePosition();

	float bestDist   = FLT_MAX;
	int   bestTarget = -1;

	for (int i = 1; i < 64; i++)
	{
		Entity entity(i);
		if (!entity.baseAddress || entity.baseAddress < 0x10000) continue;
		if (IsBadReadPtr((void*)entity.baseAddress, sizeof(DWORD))) continue;

		int health = entity.GetHealth();
		if (health <= 1 || health > 100) continue;

		if (localEnt.GetTeam() == entity.GetTeam()) continue;

		Vector3 pos = entity.GetPosition();
		if (!isValid(pos)) continue;

		float dx = pos.x - localPos.x;
		float dy = pos.y - localPos.y;
		float dz = pos.z - localPos.z;
		float dist = sqrtf(dx * dx + dy * dy + dz * dz);

		if (dist < bestDist)
		{
			bestDist   = dist;
			bestTarget = i;
		}
	}

	if (bestTarget == -1) return;

	Entity target(bestTarget);
	Vector3 targetPos = target.GetPosition();
	SetServerPosition(targetPos.x, targetPos.y, targetPos.z);
}

// CS:S m_angEyeAngles offsets on the local player entity
#define EYE_PITCH_OFFSET 0x12C
#define EYE_YAW_OFFSET   0x130

void __fastcall hookedCreateMove(void* thisptr, void* /*edx*/, int seqnum, float frametime, bool active)
{
	origCreateMove(thisptr, seqnum, frametime, active);
	if (!g_aimActive) return;


	Entity local(0);
	if (!local.baseAddress || IsBadReadPtr((void*)local.baseAddress, sizeof(DWORD))) return;
	Addresses addr;
	DWORD ptr = *(DWORD*)(addr.engineBase + addr.ViewAnglesAddr);
	*(float*)(ptr + 0x0) = g_aimPitch;
	*(float*)(ptr + 0x4) = g_aimYaw;
}

// This function locates the CreateMove function in the client.dll's vtable and hooks it using Detours
void hookCreateMove()
{
	typedef void*(*CreateInterfaceFn)(const char*, int*);
	auto createIF = (CreateInterfaceFn)GetProcAddress(GetModuleHandleA("client.dll"), "CreateInterface");
	if (!createIF) { printf("[CreateMove] CreateInterface not found\n"); return; }

	// CS:S uses VClient017; if hook fails try VClient014, VClient015, VClient016
	void* pClient = createIF("VClient017", nullptr);
	if (!pClient) { printf("[CreateMove] Interface not found\n"); return; }

	void** vTable = *(void***)pClient;
	// vtable index 21 = CreateMove for CS:S; try 22 if it doesn't work
	origCreateMove = (CreateMoveFn)DetourFunction((PBYTE)vTable[21], (PBYTE)hookedCreateMove);
	printf("[CreateMove] Hooked\n");
}
