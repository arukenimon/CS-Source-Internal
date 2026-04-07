#pragma once

// Offset to the classname string pointer within a CBaseCombatWeapon entity
// *(const char**)(weaponBase + WEAPON_CLASSNAME_OFFSET) → e.g. "weapon_knife", "weapon_usp"
#define WEAPON_CLASSNAME_OFFSET 0x5C

#define WEAPON_CLASSNAME_KNIFE  "weapon_knife"
#define WEAPON_CLASSNAME_USP    "weapon_usp"