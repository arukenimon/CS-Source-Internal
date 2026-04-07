# CS-Source-Internal

Internal DLL cheat for **Counter-Strike: Source**, written in C++.
Renders a D3D9 overlay by hooking `EndScene` and manipulates player input via a `CreateMove` hook.
Server-side features are implemented by writing directly to server-module memory via `server.dll` offsets.

---

## Preview

![Screenshot](Screenshot.png)

---

## How it works

The DLL is injected into `hl2.exe` at runtime. On attach it spins up a thread that:

1. **Hooks `IDirect3DDevice9::EndScene`** — called every rendered frame; used to draw the ESP overlay and run per-frame features (god mode, fast knife, rapid fire, speedhack).
2. **Hooks `IBaseClientDLL::CreateMove`** — called every game tick before input is sent to the server; used to silently redirect aim angles when the aimbot is active.

All hooking is done with **Microsoft Detours**.

---

## Features

### ESP Overlay
| Feature | Detail |
|---|---|
| **Bone ESP** | Full skeleton drawn on every valid enemy (and optionally teammates) by reading the bone matrix. Bone pairs are connected with lines. |
| **ESP Box** | 2D bounding box derived from foot origin and head bone, height-proportional width. |
| **Snaplines** | Line from screen bottom-centre to each enemy's feet. |
| **Head Circle** | Dynamic-radius circle around bone 14. Radius is projected from a point above the head in world space so it scales naturally with distance. |
| **Entity Index** | Entity slot number drawn above each visible player's head. |
| **Team ESP** | Optionally renders teammate entities in green alongside enemies in red. |

### Aimbot
| Feature | Detail |
|---|---|
| **Target selection** | Two modes: **Distance** (closest enemy in 3D world space) and **Crosshair** (closest head to screen centre within FOV radius). |
| **FOV Circle** | Optional screen-space circle showing the active acquisition radius. FOV preset selectable from 180 / 250 / 360 / 450. |
| **Silent aim** | Final pitch/yaw are written inside `hookedCreateMove` so the engine sends corrected angles without visibly moving the crosshair. |
| **Hold to activate** | Runs only while `LSHIFT` is held and the Aimbot toggle is ON. |

### Weapon
| Feature | Detail |
|---|---|
| **Fast Knife** | Zeros `m_flNextPrimaryAttack`, `m_flNextSecondaryAttack`, and `m_flTimeWeaponIdle` on the server-side knife entity every frame. Verified against `weapon_knife` classname before writing. |
| **Rapid Fire (USP)** | Same timer zeroing for the USP, with the additional `m_flLastFire` field (`0x5A4`) that the `CWeaponUSP` derived class checks before allowing a shot. Verified against `weapon_usp` classname before writing. |

### Movement
| Feature | Detail |
|---|---|
| **Speedhack** | Scales the local player's server-side XY velocity every frame. Five presets: 1.5× / 2.0× / 2.5× / 3.0× / 20×. Only boosts when speed is below the base maximum (300 u/s) to avoid fighting the engine cap. |

### Misc
| Feature | Detail |
|---|---|
| **Telekill** | Finds the nearest living enemy and writes the player's origin to the server-side entity position, teleporting directly on top of the target. Hold `F`. |
| **God Mode** | Writes `100` to the server-side health field every frame. |

---

## In-game Menu

Toggled at any time with `INSERT`. Rendered as a semi-transparent overlay panel in the top-left corner.  
Items are grouped into category separators (ESP / Aimbot / Weapon / Movement / Misc).

| Key | Action |
|---|---|
| `INSERT` | Open / close menu |
| `↑` / `↓` | Move selection (skips category headers) |
| `ENTER` | Toggle selected ON / OFF |
| `←` / `→` | Cycle through options for Aim Mode, FOV Radius, Speed preset |
| `NUMPAD 0` | Unhook everything and unload the DLL cleanly |

---

## Memory Layout (CS:S)

### Client-side (entity base from `client.dll + 0x004E5B14`)
| Field | Offset |
|---|---|
| Health | `+0x8C` |
| Team | `+0x94` |
| Dormant | `+0x60` |
| Origin XYZ | `+0x258` |
| Eye height | `+0xE8` |
| Velocity XYZ | `+0x218` |
| Bone matrix pointer | `+0x570` |
| Active weapon handle | `+0xD78` |
| My weapons array | `+0xCC0` |

### Engine / globals
| Field | Address |
|---|---|
| View matrix pointer | `client.dll + 0x004FF224` → `+0x50` |
| View angles pointer | `engine.dll + 0x000A5904` |

### Server-side (local player via `server.dll + 0x004F615C`)
| Field | Offset |
|---|---|
| Health | `+0xE4` |
| Origin XYZ | `+0x308` |
| Velocity XYZ | `+0x218` |

### Server-side weapon (`CBaseCombatWeapon`)
| Field | Offset |
|---|---|
| `m_flNextPrimaryAttack` | `+0x46C` |
| `m_flNextSecondaryAttack` | `+0x470` |
| `m_flTimeWeaponIdle` | `+0x474` |
| Classname string pointer | `+0x5C` |

### Server-side weapon (`CWeaponUSP` only)
| Field | Offset |
|---|---|
| `m_flLastFire` | `+0x5A4` |

### Entity list
| Symbol | Address / detail |
|---|---|
| `CGlobalEntityList` base | `server.dll + 0x004F6148` |
| `m_EntPtrArray` | base `+ 0x4` = `0x004F614C` |
| `CEntInfo` stride | `0x10` bytes; `m_pEntity` at offset 0 |
| Slot index from handle | `handle & 0xFFF` |

> Offsets verified on CS:S build **5135** (Steam). Reverify with Cheat Engine after any update.

---

## Project Structure

```
Internal/
├── Addresses.h   — struct Addresses: all hardcoded offsets and module bases
├── Math.h        — Vector3, ViewMatrix, ViewAngles, Velocity; WorldToScreen, GetViewMatrix, GetViewAngles
├── Entity.h      — class Entity (client-side reads); g_pEntityList / g_pServerTools
├── Render.h      — D3D draw helpers, struct MenuItem, DrawESPMenu
├── Features.h    — RunAimbot, RunTelekill, RunFastKnife, RunRapidFire, RunGodMode, RunSpeedhack
├── Hooks.h       — hookedCreateMove, InitEntityList, InitServerTools, hookCreateMove
├── Weapons.h     — WEAPON_CLASSNAME_OFFSET and classname string defines
├── Header.h      — master include (chains all headers above)
└── main.cpp      — DLL entry, EndScene hook, menu state, per-frame feature dispatch
```

---

## Build

**Requirements**
- Visual Studio with x86 (MSVC) build tools
- DirectX SDK (June 2010 or compatible)
- Microsoft Detours

**Steps**
1. Clone the repo
2. Open `Internal.slnx` in Visual Studio
3. Set configuration to **Release / x86**
4. Build — output is a 32-bit `.dll`
5. Inject into `hl2.exe` with any standard DLL injector

---

## Disclaimer

This project exists for **educational purposes** — understanding D3D hooking, memory reading, virtual function table patching, and Source engine internals.  
Using cheats in online games violates the Terms of Service and will result in a ban.  
Do not use this in any live online environment.
