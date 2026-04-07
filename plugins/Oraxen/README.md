# Plny Tengu Set — Oraxen Resource Pack

Complete Oraxen-ready custom item pack for the Plny Tengu set.
Rebuilt from scratch with clean naming, valid JSON, and valid YAML.

---

## 1. Architecture Overview

```
plugins/Oraxen/
├── items/
│   ├── plny_tengu_armor.yml   ← helmet, chestplate, leggings, boots, shield
│   ├── plny_tengu_tools.yml   ← sword, bow, crossbow, fishing_rod,
│   │                             staff, spear, trident, axe,
│   │                             shovel, pickaxe, hoe
│   └── plny_tengu_misc.yml    ← crate, key
└── pack/
    ├── models/custom/
    │   ├── plny_tengu_helmet.json              ← 3D helmet + back wings
    │   ├── plny_tengu_chestplate.json
    │   ├── plny_tengu_leggings.json
    │   ├── plny_tengu_boots.json
    │   ├── plny_tengu_shield.json              ← with blocking state
    │   ├── plny_tengu_shield_blocking.json
    │   ├── plny_tengu_sword.json
    │   ├── plny_tengu_bow.json                 ← with pull-state overrides
    │   ├── plny_tengu_bow_pulling_0.json
    │   ├── plny_tengu_bow_pulling_1.json
    │   ├── plny_tengu_bow_pulling_2.json
    │   ├── plny_tengu_crossbow.json            ← pulling + charged states
    │   ├── plny_tengu_crossbow_pulling_0.json
    │   ├── plny_tengu_crossbow_pulling_1.json
    │   ├── plny_tengu_crossbow_pulling_2.json
    │   ├── plny_tengu_crossbow_arrow.json
    │   ├── plny_tengu_crossbow_firework.json
    │   ├── plny_tengu_fishing_rod.json         ← with cast state
    │   ├── plny_tengu_fishing_rod_cast.json
    │   ├── plny_tengu_staff.json
    │   ├── plny_tengu_spear.json
    │   ├── plny_tengu_trident.json
    │   ├── plny_tengu_axe.json
    │   ├── plny_tengu_shovel.json
    │   ├── plny_tengu_pickaxe.json
    │   ├── plny_tengu_hoe.json
    │   ├── plny_tengu_crate.json               ← 3D chest model
    │   └── plny_tengu_key.json
    └── textures/custom/plny_tengu_set/
        ├── README.md                          ← texture placement guide
        └── <your PNG files go here>
```

### Naming convention

All item IDs and file names are prefixed with `plny_tengu_`.
This avoids conflicts with:
- Old Polagony/Oraxen files (`tengu_helmet`, `wings`, `tengu_chest`, etc.)
- Any ItemsAdder pack using generic names
- Other Oraxen plugins that register common item IDs

### Custom Model Data (CMD) assignment

| CMD   | Item ID                 | Base material       |
|-------|-------------------------|---------------------|
| 10001 | plny_tengu_helmet       | NETHERITE_HELMET    |
| 10002 | plny_tengu_chestplate   | NETHERITE_CHESTPLATE|
| 10003 | plny_tengu_leggings     | NETHERITE_LEGGINGS  |
| 10004 | plny_tengu_boots        | NETHERITE_BOOTS     |
| 10005 | plny_tengu_shield       | SHIELD              |
| 10006 | plny_tengu_sword        | NETHERITE_SWORD     |
| 10007 | plny_tengu_bow          | BOW                 |
| 10008 | plny_tengu_crossbow     | CROSSBOW            |
| 10009 | plny_tengu_fishing_rod  | FISHING_ROD         |
| 10010 | plny_tengu_staff        | BLAZE_ROD           |
| 10011 | plny_tengu_spear        | IRON_SWORD          |
| 10012 | plny_tengu_trident      | TRIDENT             |
| 10013 | plny_tengu_axe          | NETHERITE_AXE       |
| 10014 | plny_tengu_shovel       | NETHERITE_SHOVEL    |
| 10015 | plny_tengu_pickaxe      | NETHERITE_PICKAXE   |
| 10016 | plny_tengu_hoe          | NETHERITE_HOE       |
| 10017 | plny_tengu_crate        | PAPER               |
| 10018 | plny_tengu_key          | PAPER               |

---

## 2. Merged Helmet + Wings — Design Rationale

### Why full `elements` and not `item/generated`?

`item/generated` renders a flat 2-D sprite — unusable for a 3-D wearable helmet.
Using a raw `elements` array (no parent, or no flat-sprite parent) gives:
- Full 3-D geometry visible in inventory, on the ground, in hand, and worn
- Correct `display.head` transform applied when equipped
- Independent UV control per face

### How the wings appear on the player's back

In Minecraft's item model coordinate space for HEAD-slot display:

- **Low Z (≈ 0–2)** = the **front** of the model = player's face direction
- **High Z (≈ 13–16+)** = the **back** of the model = player's **back**

The wing elements (`wing_spine`, `left_wing_*`, `right_wing_*`) are positioned at
**Z = 13 – 20** in the model.  When the helmet is worn in the HEAD slot, those
elements appear behind and below the player's head, i.e. on the back and shoulders.
No second item, no /hat, no cosmetic slot plugin is required.

### Display transform tuning

The `head` display transform in `plny_tengu_helmet.json` starts at `scale [1,1,1]`.
If the helmet appears too large or too small on the player's head in-game:

1. Open `plny_tengu_helmet.json` in **Blockbench** (`File → Open`).
2. Switch to *Display* mode.
3. Select the **Head** display and adjust **scale** and **translation**.
4. A scale of **`[0.625, 0.625, 0.625]`** makes the 16-unit model ≈ 10 px wide,
   fitting closely around the player's 8-px-wide head.
5. Use negative Y translation (e.g. `[0, -1, 0]`) to lower it slightly if it floats.

### Inventory / hand rendering

- **GUI (inventory):** isometric `rotation [30, 225, 0]`, `scale [0.625, 0.625, 0.625]`
- **Third-person hand:** `scale [0.375, 0.375, 0.375]` — small enough to not clip
- **First-person hand:** `scale [0.4, 0.4, 0.4]`

---

## 3. Armor Stats

The set uses NETHERITE_* base materials.  Vanilla netherite stats apply by default.
The `AttributeModifiers` in each YAML entry add **on top** of the vanilla values:

| Piece       | Vanilla armor | Added | Total armor | Total toughness |
|-------------|:---:|:---:|:---:|:---:|
| Helmet      | 3   | +2  | **5**  | 4 |
| Chestplate  | 8   | +2  | **10** | 4 |
| Leggings    | 6   | +2  | **8**  | 4 |
| Boots       | 3   | +2  | **5**  | 4 |
| **Total**   | 20  | +8  | **28** | 16 |

Enchant glint: NETHERITE items carry the enchant-glint possibility.
You can force glint with the Oraxen `enchantments` key or `enchanted: true`
(version-dependent) if you want it to always glow without an enchantment.

---

## 4. Compatibility Strategy (Java 1.18–1.21.x + ViaVersion)

### What works everywhere (1.14–1.21.x)
- `CustomModelData` (CMD) overrides in item models
- `item/generated` and `item/handheld` parents
- Full `elements` arrays (no parent) for 3-D models
- `display` transforms (all slots)
- `overrides` array with `pulling` / `pull` / `charged` / `blocking` / `cast` predicates

### 1.21.4+ change to be aware of
Minecraft 1.21.4 introduced the **Item Model Definition** system
(`assets/<namespace>/items/<id>.json`).  Oraxen 2.x handles this transparently —
the CMD system continues to work.  If you're on a cutting-edge Oraxen 2.x build,
check whether it generates `items/*.json` for you; if so, do **not** manually add
them or you'll get duplicate overrides.

### ViaVersion
ViaVersion translates packets between versions.  Item model data (CMD) is
carried in NBT/components and is version-agnostic at the server level.
The resource pack is served to **the connecting client's version** so:
- Clients on 1.19 will load the pack as a 1.19 pack (CMD works fine)
- Clients on 1.21.4 will load it as a 1.21.4 pack

To support both, avoid features that are *only* in 1.21.4's new item model format.
The models in this pack use only the legacy override system, which is safe.

### What to avoid
- Do **not** use `item/trident` as a parent for custom trident textures — it cannot
  accept external `layer0` texture substitution cleanly.  `item/handheld` is used
  instead and works identically for display purposes.
- Do **not** rely on OptiFine CIT or shader features for cross-version compatibility.
- Do **not** use 1.21.4-only `"model"` component YAML keys if targeting older clients.

---

## 5. Worn Armor Appearance (Equip Slot Visual)

> **Important:** Custom Model Data controls the **item icon** (inventory + hand).
> The **worn armor texture** (what you see on the player's body when equipped)
> is controlled by a separate system:
>
> - **1.20+ with Oraxen 2.x:** Use armor trim textures or Oraxen's
>   `armor_layer` feature (if available in your build).
> - **1.19 and below:** Requires OptiFine CIT resource pack or a shader.
> - **Helmet in HEAD slot:** The item's 3-D model IS shown on the head —
>   so the helmet + wings model will render correctly when worn without
>   any additional armor layer setup.
> - **Chestplate/Leggings/Boots:** The body armor texture is separate from
>   the item icon and requires the `armor_layer` approach for custom visuals.

---

## 6. Crate & Key

Both use `material: PAPER` which has no vanilla click behaviour.
Wire them up to a crate plugin (e.g. **EpicCrates**, **CrateReloaded**,
**BetterCrates**) by configuring that plugin to recognise:
- material `PAPER` + `custom-model-data: 10017` as a crate item
- material `PAPER` + `custom-model-data: 10018` as the matching key

Most crate plugins support CMD matching out of the box.

Give items to players with:
```
/oraxen give <player> plny_tengu_crate 1
/oraxen give <player> plny_tengu_key 1
```

---

## 7. Avoiding Oraxen / ItemsAdder Conflicts

If both Oraxen and ItemsAdder are installed:

1. **Separate namespaces** — all Plny Tengu items are in the `plny_tengu_*`
   namespace; ensure ItemsAdder items use a different prefix.
2. **Separate CMD ranges** — ItemsAdder typically uses low CMD values (1–999).
   This pack uses 10001–10018.  Verify with `/ia dump` or `config.yml` to
   see ItemsAdder's CMD range and ensure no overlap.
3. **One pack host** — only one plugin should serve the resource pack.
   Disable Oraxen's built-in pack sender if ItemsAdder serves the pack, or
   vice versa.  In Oraxen `settings.yml`: `Pack → dispatch_pack_on_join: false`.
   In ItemsAdder `config.yml`: `resource-pack → hosting → enabled: false`.
   Then use a merge tool (Oraxen's merge, or a manual `assets/` merge) so both
   sets of textures live in one pack.
4. **Texture namespace clash** — Oraxen stores textures in `assets/minecraft/`.
   ItemsAdder uses a custom namespace (e.g. `assets/ia/`).  They will not clash
   as long as you do not manually copy textures into each other's namespace paths.

---

## 8. Migration Checklist — from old broken pack to new

- [ ] **Back up** your current `plugins/Oraxen/` directory to a safe location.
- [ ] **Delete** the following legacy files/directories to avoid ID conflicts:
  - `plugins/Oraxen/items/` files referencing `tengu_helmet`, `wings`,
    `tengu_chestplate`, `tengu_leggings`, `tengu_boots`,
    `tengu_key`, `tengu_chest` (and any cosmetic variant YAML)
  - `plugins/Oraxen/pack/models/` files with the old names
  - `plugins/Oraxen/pack/textures/` directories using the old paths
- [ ] **Copy** this repository's `plugins/Oraxen/` folder into your server.
- [ ] **Place your textures** in `plugins/Oraxen/pack/textures/custom/plny_tengu_set/`
  using the exact filenames listed in the texture README.
- [ ] **Rename** existing texture files if needed:
  - `wings.png` → `plny_tengu_wings.png`
  - `tengu_helmet.png` → `plny_tengu_helmet.png`
  - (and so on for each piece)
- [ ] Open `plny_tengu_helmet.json` in **Blockbench** and verify that the
  `#helmet` and `#wings` texture variables map to the correct regions of
  your textures.  Adjust UV values as needed.
- [ ] Restart the server (full restart, not `/reload`).
- [ ] Run `/oraxen reload` once the server is up to force pack regeneration.
- [ ] Equip `plny_tengu_helmet` (`/oraxen give <you> plny_tengu_helmet 1`)
  and verify the wings appear on your back.
- [ ] Check each tool/weapon in hand for correct texture.
- [ ] If the pack update is not received by clients: disable and re-enable
  the resource pack in Oraxen's `settings.yml` (`Pack → send_pack: true`)
  and reconnect.

---

## 9. Troubleshooting

### Item not found (`/oraxen give` returns unknown item)
- Check the YAML key spelling — must be `plny_tengu_<name>` exactly.
- Ensure the YAML file is in `plugins/Oraxen/items/` (not a subdirectory).
- Check Oraxen's startup log for YAML parse errors in `plny_tengu_armor.yml` etc.

### Invisible item in inventory / on head
- The model path in YAML (`Pack.model`) must match the JSON filename without
  extension: `model: custom/plny_tengu_helmet` → `pack/models/custom/plny_tengu_helmet.json`.
- Verify the JSON file is valid: paste it into [jsonlint.com](https://jsonlint.com).
- Check Oraxen logs for `Cannot load model` errors.

### Purple/black (missing texture)
- The texture PNG is missing from `textures/custom/plny_tengu_set/`.
- The texture path in the JSON (`"helmet": "custom/plny_tengu_set/plny_tengu_helmet"`)
  does not match the actual file path.  Paths are case-sensitive.
- Oraxen might not have included the texture in the generated pack — check
  `plugins/Oraxen/pack/.generated_pack/` or run `/oraxen pack` to inspect.

### Pack not updating on clients
1. Force a server-side pack rebuild: delete `plugins/Oraxen/pack/.generated_pack/`
   and restart.
2. Force clients to re-download: change the pack version in `settings.yml`
   (increment `Pack → pack_version`) and reconnect.
3. If using BungeeCord/Velocity: ensure the pack is served at the proxy level
   or that each backend server sends the pack on join.

### Wearable item not equipping in head slot
- The base `material` must be a helmet-compatible type.
  `NETHERITE_HELMET`, `DIAMOND_HELMET`, `LEATHER_HELMET`, `GOLDEN_HELMET`,
  `IRON_HELMET`, `CHAINMAIL_HELMET`, `TURTLE_HELMET`, or any item with
  `equippable` component (1.21.2+) will equip in the head slot.
- Shift-clicking the item into the armour slot or using `/equipment set` should work.
  If neither works, check Oraxen's `Mechanics` section for any conflicting
  `prevent_usage_in_slot` mechanic.

### Oraxen and ItemsAdder sending conflicting packs
- Only one plugin should be the pack host.  See Section 7 above.
- If players receive two pack prompts, set `dispatch_pack_on_join: false` in
  whichever plugin is NOT hosting, then manually merge the pack assets.
