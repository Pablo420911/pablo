# Texture Directory — plny_tengu_set

Place **all PNG texture files** for the Plny Tengu set in this directory.
Each filename must exactly match what the corresponding JSON model references.

## Required files

| Filename | Used by model | Notes |
|---|---|---|
| `plny_tengu_helmet.png` | plny_tengu_helmet.json `#helmet` | Main dome, visor, cheek guards, crest, horns |
| `plny_tengu_wings.png` | plny_tengu_helmet.json `#wings` | Wing spine, upper/mid/lower panels |
| `plny_tengu_chestplate.png` | plny_tengu_chestplate.json | Flat 2-D icon |
| `plny_tengu_leggings.png` | plny_tengu_leggings.json | Flat 2-D icon |
| `plny_tengu_boots.png` | plny_tengu_boots.json | Flat 2-D icon |
| `plny_tengu_sword.png` | plny_tengu_sword.json | Pixel-art sword |
| `plny_tengu_bow.png` | plny_tengu_bow.json | Bow — idle / undrawn state |
| `plny_tengu_bow_pulling_0.png` | plny_tengu_bow_pulling_0.json | Bow — start of draw |
| `plny_tengu_bow_pulling_1.png` | plny_tengu_bow_pulling_1.json | Bow — mid draw |
| `plny_tengu_bow_pulling_2.png` | plny_tengu_bow_pulling_2.json | Bow — full draw |
| `plny_tengu_crossbow.png` | plny_tengu_crossbow.json | Crossbow — idle |
| `plny_tengu_crossbow_pulling_0.png` | plny_tengu_crossbow_pulling_0.json | Crossbow — loading start |
| `plny_tengu_crossbow_pulling_1.png` | plny_tengu_crossbow_pulling_1.json | Crossbow — loading mid |
| `plny_tengu_crossbow_pulling_2.png` | plny_tengu_crossbow_pulling_2.json | Crossbow — loaded |
| `plny_tengu_crossbow_arrow.png` | plny_tengu_crossbow_arrow.json | Crossbow charged with arrow |
| `plny_tengu_crossbow_firework.png` | plny_tengu_crossbow_firework.json | Crossbow charged with firework |
| `plny_tengu_shield.png` | plny_tengu_shield.json | Shield front artwork |
| `plny_tengu_fishing_rod.png` | plny_tengu_fishing_rod.json | Rod — uncast |
| `plny_tengu_fishing_rod_cast.png` | plny_tengu_fishing_rod_cast.json | Rod — line in water |
| `plny_tengu_staff.png` | plny_tengu_staff.json | Staff sprite |
| `plny_tengu_spear.png` | plny_tengu_spear.json | Spear sprite |
| `plny_tengu_trident.png` | plny_tengu_trident.json | Trident sprite |
| `plny_tengu_axe.png` | plny_tengu_axe.json | Axe sprite |
| `plny_tengu_shovel.png` | plny_tengu_shovel.json | Shovel sprite |
| `plny_tengu_pickaxe.png` | plny_tengu_pickaxe.json | Pickaxe sprite |
| `plny_tengu_hoe.png` | plny_tengu_hoe.json | Hoe sprite |
| `plny_tengu_crate.png` | plny_tengu_crate.json `#crate` | Crate body/lid/lock texture |
| `plny_tengu_key.png` | plny_tengu_key.json | Key sprite |

## UV layout notes for the helmet textures

### `plny_tengu_helmet.png` — recommended 64 × 64 px

The JSON model uses 0–16 UV units (Minecraft maps them proportionally to your
texture size).  Suggested region layout to match the UV values in
`plny_tengu_helmet.json`:

| UV region (in 0–16 units) | What it covers |
|---|---|
| `[0, 0, 12, 8]` | Helmet dome sides (north/south/east/west) |
| `[0, 0, 12, 12]` | Helmet dome top/bottom |
| `[0, 8, 10, 12]` | Visor front/back |
| `[12, 0, 14, 6]` | Cheek guard narrow faces |
| `[12, 0, 20, 6]` | Cheek guard wide faces |
| `[0, 12, 6, 16]` | Top crest all faces |
| `[6, 12, 9, 16]` | Left horn all faces |
| `[9, 12, 12, 16]` | Right horn all faces |

### `plny_tengu_wings.png` — recommended 64 × 64 px

| UV region | What it covers |
|---|---|
| `[7, 0, 9, 14]` / `[7, 0, 10, 14]` | Wing spine |
| `[0, 2, 7, 8]` | Left wing upper north/south |
| `[0, 8, 11, 12]` | Left wing mid |
| `[0, 12, 9, 16]` | Left wing lower |
| `[9, 2, 16, 8]` | Right wing upper |
| `[5, 8, 16, 12]` | Right wing mid |
| `[7, 12, 16, 16]` | Right wing lower |

> **Tip:** If your existing textures don't match these UV coordinates, open
> `plny_tengu_helmet.json` in **Blockbench** → *File → Open Model*, then
> use the **UV editor** to re-map each face to match your artwork without
> needing to re-paint the texture.

## Quick-start with your existing textures

If you already have a merged `wings.png` / `helmet.png` from the old pack:

1. Rename them to `plny_tengu_wings.png` / `plny_tengu_helmet.png`.
2. Drop them here.
3. Reload Oraxen: `/oraxen reload` (or restart).
4. Open `plny_tengu_helmet.json` in Blockbench to fine-tune UV regions.
