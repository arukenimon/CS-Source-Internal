#pragma once
#include <cstdio>
#include <d3d9.h>
#include <d3dx9.h>
#include "Entity.h"

#define GREEN D3DCOLOR_ARGB(255,   0, 255,   0)
#define RED   D3DCOLOR_ARGB(255, 255,   0,   0)

struct MenuItem
{
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

inline void DrawString(const char* text, int x, int y, int a, int r, int g, int b, LPD3DXFONT font)
{
	RECT rect;
	SetRect(&rect, x, y, x + 200, y + 20);
	font->DrawTextA(NULL, text, -1, &rect, DT_NOCLIP, D3DCOLOR_ARGB(a, r, g, b));
}

inline void DrawLine(float x1, float y1, float x2, float y2, D3DCOLOR color, ID3DXLine* pLine)
{
	D3DXVECTOR2 points[2] = { { x1, y1 }, { x2, y2 } };
	pLine->Begin();
	pLine->Draw(points, 2, color);
	pLine->End();
}

inline void DrawCircle(float cx, float cy, float radius, int segments, D3DCOLOR color, ID3DXLine* pLine)
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

inline void DrawFilledRect(float x, float y, float w, float h, D3DCOLOR color, ID3DXLine* pLine)
{
	pLine->SetWidth(h);
	D3DXVECTOR2 pts[2] = { { x, y + h * 0.5f }, { x + w, y + h * 0.5f } };
	pLine->Begin();
	pLine->Draw(pts, 2, color);
	pLine->End();
	pLine->SetWidth(1.0f);
}

inline void DrawBoneNumbers(Entity& entity, ViewMatrix vm, int screenW, int screenH, LPD3DXFONT font)
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

inline void DrawEntityIndex(Entity& entity, int index, ViewMatrix vm, int screenW, int screenH, LPD3DXFONT font)
{
	Vector3 headWorld = entity.GetBonePosition(14);
	if (!isValid(headWorld)) return;

	Vector3 screenPos;
	if (!WorldToScreen(headWorld, screenPos, vm, screenW, screenH)) return;

	char buf[8];
	sprintf_s(buf, sizeof(buf), "%d", index);
	DrawString(buf, (int)screenPos.x - 4, (int)screenPos.y - 16, 255, 255, 255, 255, font);
}

inline void DrawHeadCircle(Entity& entity, ViewMatrix vm, int screenW, int screenH, ID3DXLine* pLine, D3DCOLOR color)
{
	Vector3 headWorld = entity.GetBonePosition(14);
	if (!isValid(headWorld)) return;

	Vector3 headScreen;
	if (!WorldToScreen(headWorld, headScreen, vm, screenW, screenH)) return;

	Vector3 topWorld  = { headWorld.x, headWorld.y, headWorld.z + 4.0f };
	Vector3 topScreen;
	float   radius = 8.0f;
	if (WorldToScreen(topWorld, topScreen, vm, screenW, screenH))
		radius = max(3.0f, fabsf(headScreen.y - topScreen.y));

	DrawCircle(headScreen.x, headScreen.y, radius, 20, color, pLine);
}

inline void DrawBones(Entity& entity, ViewMatrix vm, int screenW, int screenH, ID3DXLine* pLine, D3DCOLOR color)
{
	static const int bonePairs[][2] = {
		// spine
		{ 0, 9 }, { 9, 10 }, { 10, 11 }, { 11, 13 }, { 13, 14 },
		// pelvis
		{ 0, 2 },
		// left arm
		{ 11, 29 }, { 29, 30 }, { 30, 31 },
		// right arm
		{ 11, 16 }, { 16, 17 }, { 17, 18 },
		// left leg
		{ 0, 6 }, { 6, 7 }, { 7, 8 },
		// right leg
		{ 2, 3 }, { 3, 4 },
	};
	int pairCount = sizeof(bonePairs) / sizeof(bonePairs[0]);

	for (int i = 0; i < pairCount; i++)
	{
		Vector3 w1 = entity.GetBonePosition(bonePairs[i][0]);
		Vector3 w2 = entity.GetBonePosition(bonePairs[i][1]);

		if (!isValid(w1) || !isValid(w2)) continue;

		Vector3 s1, s2;
		if (!WorldToScreen(w1, s1, vm, screenW, screenH)) continue;
		if (!WorldToScreen(w2, s2, vm, screenW, screenH)) continue;

		DrawLine(s1.x, s1.y, s2.x, s2.y, color, pLine);
	}

	DrawHeadCircle(entity, vm, screenW, screenH, pLine, color);
}

inline void DrawSnapline(Entity& entity, ViewMatrix vm, int screenW, int screenH, ID3DXLine* pLine, D3DCOLOR color)
{
	Vector3 worldPos = entity.GetPosition();
	if (!isValid(worldPos)) return;

	Vector3 screenPos;
	if (!WorldToScreen(worldPos, screenPos, vm, screenW, screenH)) return;

	DrawLine(screenW / 2.0f, (float)screenH, screenPos.x, screenPos.y, color, pLine);
}

inline void DrawESPBox(Entity& entity, ViewMatrix vm, int screenW, int screenH, ID3DXLine* pLine, D3DCOLOR color)
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
	float x1    = cx - width * 0.5f;
	float x2    = cx + width * 0.5f;
	float y1    = headScreen.y - 4.0f;
	float y2    = footScreen.y;

	DrawLine(x1, y1, x2, y1, color, pLine); // top
	DrawLine(x1, y2, x2, y2, color, pLine); // bottom
	DrawLine(x1, y1, x1, y2, color, pLine); // left
	DrawLine(x2, y1, x2, y2, color, pLine); // right
}

inline void DrawESPMenu(int sel, MenuItem* items, int count, LPD3DXFONT font, ID3DXLine* pLine)
{
	const float x = 20.0f, y = 50.0f, w = 200.0f, rowH = 18.0f, pad = 3.0f;
	float totalH = rowH * (count + 2.0f);

	DrawFilledRect(x, y, w, totalH, D3DCOLOR_ARGB(180, 15, 15, 15), pLine);

	D3DCOLOR border = D3DCOLOR_ARGB(255, 80, 130, 255);
	DrawLine(x,     y,          x + w, y,          border, pLine);
	DrawLine(x,     y + totalH, x + w, y + totalH, border, pLine);
	DrawLine(x,     y,          x,     y + totalH, border, pLine);
	DrawLine(x + w, y,          x + w, y + totalH, border, pLine);

	DrawString("  REV hacks", (int)(x + 4), (int)(y + pad), 255, 120, 180, 255, font);
	DrawLine(x, y + rowH, x + w, y + rowH, border, pLine);

	for (int i = 0; i < count; i++)
	{
		float iy  = y + rowH + (i * rowH);
		bool  sep = !items[i].bvalue && !items[i].fvalue && !items[i].ivalue;

		if (!sep && i == sel)
			DrawFilledRect(x + 1, iy, w - 2, rowH, D3DCOLOR_ARGB(140, 60, 80, 200), pLine);

		char buf[64];
		int ca = 255, cr, cg, cb = 80;

		if (sep)
		{
			DrawFilledRect(x + 1, iy, w - 2, rowH, D3DCOLOR_ARGB(180, 15, 15, 15), pLine);
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
