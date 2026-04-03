# Polagony Tengu Set — Oraxen Resource Pack

A clean, production-quality custom item pack for Minecraft Java Edition,
built for [Oraxen](https://oraxen.com/). All items use the `plny_tengu_*`
namespace to avoid conflicts with legacy files or other plugins.

---

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Folder Structure](#folder-structure)
3. [Item List](#item-list)
4. [Installation](#installation)
5. [Helmet + Wings Technical Design](#helmet--wings-technical-design)
6. [Armor Stats Reference](#armor-stats-reference)
7. [Custom Worn-Armor Visuals](#custom-worn-armor-visuals)
8. [Bow & Crossbow Animations](#bow--crossbow-animations)
9. [Crate & Key Setup](#crate--key-setup)
10. [Compatibility Notes (1.18–1.21+ / ViaVersion)](#compatibility-notes)
11. [Oraxen + ItemsAdder Coexistence](#oraxen--itemsadder-coexistence)
12. [Migration Checklist (Old → New Files)](#migration-checklist)
13. [Troubleshooting](#troubleshooting)

---

## Architecture Overview

```
plugins/
└── Oraxen/
    ├── items/
    │   ├── plny_tengu_armor.yml       ← helmet+wings, chestplate, leggings, boots
    │   ├── plny_tengu_tools.yml       ← sword, bow, crossbow, shield, rod,
    │   │                                 staff, spear, trident, axe, shovel,
    │   │                                 pickaxe, hoe
    │   └── plny_tengu_misc.yml        ← crate, key
    └── pack/
        ├── models/
        │   └── custom/
        │       ├── plny_tengu_helmet.json         ← FULL 3D elements model (helmet+wings)
        │       ├── plny_tengu_chestplate.json
        │       ├── plny_tengu_leggings.json
        │       ├── plny_tengu_boots.json
        │       ├── plny_tengu_sword.json
        │       ├── plny_tengu_bow.json             ← includes pulling overrides
        │       ├── plny_tengu_bow_pulling_0.json
        │       ├── plny_tengu_bow_pulling_1.json
        │       ├── plny_tengu_bow_pulling_2.json
        │       ├── plny_tengu_crossbow.json        ← includes charged overrides
        │       ├── plny_tengu_crossbow_arrow.json
        │       ├── plny_tengu_crossbow_firework.json
        │       ├── plny_tengu_shield.json
        │       ├── plny_tengu_fishing_rod.json
        │       ├── plny_tengu_staff.json
        │       ├── plny_tengu_spear.json
        │       ├── plny_tengu_trident.json
        │       ├── plny_tengu_axe.json
        │       ├── plny_tengu_shovel.json
        │       ├── plny_tengu_pickaxe.json
        │       ├── plny_tengu_hoe.json
        │       ├── plny_tengu_crate.json
        │       └── plny_tengu_key.json
        └── textures/
            ├── custom/
            │   └── plny_tengu_set/
            │       ├── TEXTURES_README.md  ← texture layout guide
            │       ├── plny_tengu_helmet.png        (YOU PROVIDE)
            │       ├── plny_tengu_wings.png          (YOU PROVIDE)
            │       └── … (one PNG per item — see TEXTURES_README.md)
            └── models/
                └── armor/
                    ├── ARMOR_LAYERS_README.md
                    ├── plny_tengu_layer_1.png   (optional, for worn body textures)
                    └── plny_tengu_layer_2.png   (optional)
```

### Why this structure?

| Decision | Reason |
|----------|--------|
| `plny_tengu_*` namespace prefix | Prevents collisions with old `tengu_*` or `wings` item IDs |
| `generate_model: false` for every item | You supply pre-built JSON models; Oraxen never overwrites them |
| `NETHERITE_*` base materials for armor/weapons | Full netherite stats by default; custom attributes push further |
| Separate `plny_tengu_helmet.png` and `plny_tengu_wings.png` | Update either texture independently |
| `item/generated` for flat items (armor, crate, key) | Perfect inventory render, no distortion |
| `item/handheld` for tools/weapons | Correct grip rotation in first/third-person view |
| `item/bow` / `item/crossbow` parent for ranged weapons | Inherits vanilla hand animations automatically |

---

## Item List

| Oraxen ID               | Base Material        | Type        |
|-------------------------|----------------------|-------------|
| `plny_tengu_helmet`     | NETHERITE_HELMET     | Armor       |
| `plny_tengu_chestplate` | NETHERITE_CHESTPLATE | Armor       |
| `plny_tengu_leggings`   | NETHERITE_LEGGINGS   | Armor       |
| `plny_tengu_boots`      | NETHERITE_BOOTS      | Armor       |
| `plny_tengu_sword`      | NETHERITE_SWORD      | Weapon      |
| `plny_tengu_bow`        | BOW                  | Ranged      |
| `plny_tengu_crossbow`   | CROSSBOW             | Ranged      |
| `plny_tengu_shield`     | SHIELD               | Defense     |
| `plny_tengu_fishing_rod`| FISHING_ROD          | Tool        |
| `plny_tengu_staff`      | NETHERITE_HOE        | Weapon/Tool |
| `plny_tengu_spear`      | NETHERITE_SWORD      | Weapon      |
| `plny_tengu_trident`    | TRIDENT              | Weapon      |
| `plny_tengu_axe`        | NETHERITE_AXE        | Weapon/Tool |
| `plny_tengu_shovel`     | NETHERITE_SHOVEL     | Tool        |
| `plny_tengu_pickaxe`    | NETHERITE_PICKAXE    | Tool        |
| `plny_tengu_hoe`        | NETHERITE_HOE        | Tool        |
| `plny_tengu_crate`      | CHEST                | Misc        |
| `plny_tengu_key`        | TRIPWIRE_HOOK        | Misc        |

---

## Installation

1. **Copy files** — place the entire `plugins/Oraxen/` subtree into your
   server's existing `plugins/Oraxen/` folder.
2. **Add textures** — place your PNG files in
   `plugins/Oraxen/pack/textures/custom/plny_tengu_set/`
   (see [TEXTURES_README.md](plugins/Oraxen/pack/textures/custom/plny_tengu_set/TEXTURES_README.md)).
3. **Reload** — run `/oraxen reload` or restart the server.
4. **Give items** — `/o give <player> plny_tengu_helmet`

---

## Helmet + Wings Technical Design

### Why a full `elements`-based model (not `item/generated`)?

`item/generated` renders a flat 2D sprite. It cannot represent a 3D helmet
sitting on the player's head, and it cannot attach wing geometry behind the
body. A full `elements`-based model (all geometry defined in the JSON `elements`
array) solves both problems and is the correct, production approach.

### How the wings end up on the player's back

Minecraft's coordinate system for head-slot items:

```
      +Y (up)
       │
       │     +Z (back of head → behind body)
       │   ╱
       │ ╱
       └──────── +X (player's right)

Z = 0  : player face (front)
Z = 16 : back of head
Z > 16 : behind the player's body  ← wings live here
```

The wing elements in `plny_tengu_helmet.json` are placed at **Z = 14–17**
and **Y = −6 to +8**. When the helmet is worn (head slot), Minecraft renders
the item with this orientation — the wings appear at the player's upper-back
and shoulder area, exactly like cosmetic back-wings.

### UV mapping

- `#helmet` → `custom/plny_tengu_set/plny_tengu_helmet.png` (64 × 64)
- `#wings` → `custom/plny_tengu_set/plny_tengu_wings.png` (64 × 64)
- All UV values in the JSON are **pixel coordinates** in the 0–64 range.
- To repaint or remap, open `plny_tengu_helmet.json` in **Blockbench**
  (`File → Open Model`), adjust UVs visually, then re-export as
  *Java Block/Item Model*.

### Display transform cheatsheet

| Context | Effect |
|---------|--------|
| `head` | Worn on player's head. Identity transform — correct for all helmet items. |
| `gui` | Inventory slot. Rotated 30°/225° at 0.35× scale so the full model fits. |
| `firstperson_righthand` | Held in right hand. 0.4× scale with slight tilt. |
| `ground` | Dropped on floor. 0.25× scale. |
| `fixed` | Item frame. 90° rotation so it faces the viewer. |

---

## Armor Stats Reference

Designed to exceed vanilla netherite as an endgame reward.

| Piece        | Armor | Toughness | Bonus |
|--------------|-------|-----------|-------|
| Helmet       | 5     | 4         | +4 max HP, +20% knockback resist |
| Chestplate   | 8     | 4         | +6 max HP |
| Leggings     | 6     | 4         | +5% movement speed |
| Boots        | 3     | 4         | +10% speed, +10% knockback resist |
| **Full set** | **22**| **16**    | Exceeds full vanilla netherite (20 / 12) |

All pieces are `Unbreakable: true`. Remove that line for normal durability.

**Enchant glint:** Not forced by default. The red/orange gradient already
creates a strong visual identity. Add `Enchantments: - DURABILITY:1` with
`HIDE_ENCHANTS` if you want the glint without visible enchantment text.

---

## Custom Worn-Armor Visuals

By default, chestplate / leggings / boots show the vanilla netherite texture
when **worn on the player body** (the item's custom model only shows in hand
and inventory). To use custom body textures:

1. Create `plny_tengu_layer_1.png` (64 × 32) and
   `plny_tengu_layer_2.png` (64 × 32) matching the vanilla armor UV layout.
2. Place them in `plugins/Oraxen/pack/textures/models/armor/`.
3. Uncomment the `Mechanics: Armor: texture: plny_tengu` section in
   `plny_tengu_armor.yml` for each affected piece.

**1.21.4+ only alternative:** Minecraft's `minecraft:equippable` item
component supports a `model` field for custom equipment models. Oraxen 2.x
exposes this via `Components: equippable: ...`. Use only when targeting
Paper 1.21.4+ exclusively; it cannot be loaded by older clients via
ViaVersion.

---

## Bow & Crossbow Animations

`plny_tengu_bow.json` uses vanilla override predicates:

```json
"overrides": [
  {"predicate": {"pulling": 1},              "model": "custom/plny_tengu_bow_pulling_0"},
  {"predicate": {"pulling": 1, "pull": 0.65},"model": "custom/plny_tengu_bow_pulling_1"},
  {"predicate": {"pulling": 1, "pull": 0.9}, "model": "custom/plny_tengu_bow_pulling_2"}
]
```

Provide three draw-state textures (`plny_tengu_bow_pulling_0/1/2.png`)
showing progressively more tension.

`plny_tengu_crossbow.json` switches to:
- `plny_tengu_crossbow_arrow.json` when an arrow is loaded.
- `plny_tengu_crossbow_firework.json` when a firework is loaded.

---

## Crate & Key Setup

Both items are **pure custom visual items** in Oraxen. Actual crate-opening
logic must be handled by a separate crate plugin (e.g. AdvancedCrates,
GoldenCrates, CrateReloaded, CrazyEnvoy).

**Finding the CMD value:**
```
/oraxen debug plny_tengu_crate
```

**Giving items:**
```
/o give <player> plny_tengu_crate
/o give <player> plny_tengu_key
```

---

## Compatibility Notes

### Version support

| Version    | Status   | Notes |
|------------|----------|-------|
| 1.18.x     | ✅ Full  | All features |
| 1.19.x     | ✅ Full  | All features |
| 1.20.0–4   | ✅ Full  | All features |
| 1.20.5–6   | ✅ Full  | Paper attributes renamed; Oraxen maps automatically |
| 1.21.x     | ✅ Full  | Primary target |

### ViaVersion

- Models use the −16 to +32 element range, supported by all Java clients
  since 1.14.
- Do **not** use `minecraft:equippable` or `minecraft:item_model` (1.21.4
  features) if you also support older clients via ViaVersion.
- The `overrides`-based bow/crossbow approach works on all Java client
  versions.

### What to avoid

- `generate_model: true` when you have a hand-crafted JSON — it overwrites
  your model.
- Texture paths with uppercase letters, spaces, or backslashes.
- Reusing CMD values Oraxen already assigned to other items.
- Texture files referenced in JSON that don't yet exist — causes
  purple/black missing-texture appearance.

---

## Oraxen + ItemsAdder Coexistence

Running both on the same server is **not recommended** because both try to
serve a resource pack. Only one pack can be active per client connection.

### If you must run both

1. Disable pack hosting in one plugin (in ItemsAdder:
   `resource-pack → hosting: disabled`).
2. Reserve non-overlapping CMD ranges (e.g. 1–999 for ItemsAdder,
   1000+ for Oraxen).
3. Never name an ItemsAdder namespace `oraxen` or vice versa.
4. Merge both packs into a single ZIP using a tool like
   [Merge Resource Packs](https://github.com/nicholasgasior/mc-pack-merger)
   and host it on an external CDN.

---

## Migration Checklist

### Before you start

- [ ] Back up old `plugins/Oraxen/items/` and `plugins/Oraxen/pack/` to a ZIP.
- [ ] List all old item IDs that players currently hold in-game.
- [ ] Confirm which old JSON/YAML files were broken.

### Remove old files

- [ ] Delete any item YAML entries using the old IDs:
      `tengu_helmet`, `wings`, `tengu_chestplate`, `tengu_leggings`,
      `tengu_boots`, `tengu_key`, `tengu_chest`.
- [ ] Delete old model JSON files with broken texture paths.
- [ ] Delete the old merged helmet+wings flat-sprite PNG (if it was
      `item/generated`).

### Install new files

- [ ] Copy `plugins/Oraxen/items/plny_tengu_armor.yml`
- [ ] Copy `plugins/Oraxen/items/plny_tengu_tools.yml`
- [ ] Copy `plugins/Oraxen/items/plny_tengu_misc.yml`
- [ ] Copy all 23 `plugins/Oraxen/pack/models/custom/plny_tengu_*.json` files
- [ ] Place your PNG textures in
      `plugins/Oraxen/pack/textures/custom/plny_tengu_set/`

### Verify

- [ ] `/oraxen reload` — check console for errors.
- [ ] `/o give <player> plny_tengu_helmet` — equip and confirm wings appear.
- [ ] Test each item in the item list.
- [ ] Replace any old items in player inventories with new `plny_tengu_*` IDs.

---

## Troubleshooting

### Item not found (`/o give` returns "unknown item")

1. Confirm the YAML file is in `plugins/Oraxen/items/` (not a subfolder).
2. Validate YAML indentation at [yamllint.com](https://www.yamllint.com/).
3. Look for `[Oraxen] Loaded items/plny_tengu_armor.yml` in the console.

### Invisible item (held or in inventory)

1. Check console for `[Oraxen] Failed to load model` errors.
2. Confirm the `model:` path in YAML exactly matches the JSON filename.
   - YAML: `model: custom/plny_tengu_helmet`
   - File: `pack/models/custom/plny_tengu_helmet.json` ✓

### Purple / black "missing texture"

The PNG file does not exist at the path referenced in the JSON.

- JSON has: `"helmet": "custom/plny_tengu_set/plny_tengu_helmet"`
- File must exist at: `pack/textures/custom/plny_tengu_set/plny_tengu_helmet.png`
- Path must be all lowercase with no spaces.
- After adding the PNG, run `/oraxen reload`.

### Pack not updating on the client

1. `/oraxen reload` on the server.
2. Client: disconnect, clear the cached server resource pack, reconnect.
3. Or increment `pack-format` in `Oraxen/settings.yml` to force a cache miss.

### Wearable item not equipping in head slot

- Base material must be a helmet type (e.g. `NETHERITE_HELMET`).
- The `head` display transform in the JSON must be present (identity values
  are correct: rotation `[0,0,0]`, translation `[0,0,0]`, scale `[1,1,1]`).

### Wings not visible when helmet is worn

1. Confirm `plny_tengu_wings.png` exists in the texture folder.
2. Confirm wing elements in the JSON have Z coordinates > 12 (they must
   be behind the head to appear at the player's back).
3. Open the model in Blockbench → enable *Display* → *Head* preview to
   verify element positions visually before placing on a server.

### Oraxen and ItemsAdder sending conflicting packs

See [Oraxen + ItemsAdder Coexistence](#oraxen--itemsadder-coexistence) above.
Disable pack hosting in one of the two plugins.
