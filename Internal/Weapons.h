#pragma once

// Offset to the classname string pointer within a CBaseCombatWeapon entity
// Dereference this offset on the weapon base address to get e.g. "weapon_knife", "weapon_ak47"
#define WEAPON_CLASSNAME_OFFSET 0x5C