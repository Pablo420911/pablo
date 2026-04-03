# Texture Placement Guide — plny_tengu_set

Place your PNG texture files in this directory:
`plugins/Oraxen/pack/textures/custom/plny_tengu_set/`

## Required Textures

| File name                          | Used by model                   | Recommended size |
|------------------------------------|----------------------------------|-----------------|
| `plny_tengu_helmet.png`            | plny_tengu_helmet.json (#helmet) | 64 × 64         |
| `plny_tengu_wings.png`             | plny_tengu_helmet.json (#wings)  | 64 × 64         |
| `plny_tengu_chestplate.png`        | plny_tengu_chestplate.json       | 64 × 64         |
| `plny_tengu_leggings.png`          | plny_tengu_leggings.json         | 64 × 64         |
| `plny_tengu_boots.png`             | plny_tengu_boots.json            | 64 × 64         |
| `plny_tengu_sword.png`             | plny_tengu_sword.json            | 16 × 64         |
| `plny_tengu_bow.png`               | plny_tengu_bow.json              | 32 × 32         |
| `plny_tengu_bow_pulling_0.png`     | plny_tengu_bow_pulling_0.json    | 32 × 32         |
| `plny_tengu_bow_pulling_1.png`     | plny_tengu_bow_pulling_1.json    | 32 × 32         |
| `plny_tengu_bow_pulling_2.png`     | plny_tengu_bow_pulling_2.json    | 32 × 32         |
| `plny_tengu_crossbow.png`          | plny_tengu_crossbow.json         | 32 × 32         |
| `plny_tengu_crossbow_arrow.png`    | plny_tengu_crossbow_arrow.json   | 32 × 32         |
| `plny_tengu_crossbow_firework.png` | plny_tengu_crossbow_firework.json| 32 × 32         |
| `plny_tengu_shield.png`            | plny_tengu_shield.json           | 32 × 64         |
| `plny_tengu_fishing_rod.png`       | plny_tengu_fishing_rod.json      | 16 × 64         |
| `plny_tengu_staff.png`             | plny_tengu_staff.json            | 16 × 64         |
| `plny_tengu_spear.png`             | plny_tengu_spear.json            | 16 × 64         |
| `plny_tengu_trident.png`           | plny_tengu_trident.json          | 16 × 64         |
| `plny_tengu_axe.png`               | plny_tengu_axe.json              | 32 × 32         |
| `plny_tengu_shovel.png`            | plny_tengu_shovel.json           | 16 × 32         |
| `plny_tengu_pickaxe.png`           | plny_tengu_pickaxe.json          | 32 × 32         |
| `plny_tengu_hoe.png`               | plny_tengu_hoe.json              | 32 × 32         |
| `plny_tengu_crate.png`             | plny_tengu_crate.json            | 32 × 32         |
| `plny_tengu_key.png`               | plny_tengu_key.json              | 16 × 16         |

## Helmet UV Atlas Layout (64 × 64)

The helmet uses **two separate textures** (`plny_tengu_helmet.png` and
`plny_tengu_wings.png`), each 64 × 64 pixels.

### plny_tengu_helmet.png (pixel layout guide)
```
  X →  0        8       16       24       32       40       48       56       64
Y ↓
  0  [top-face-L] [top-face-R]  [  horns-area  ]  [   extra   ]
  8  [E ] [front] [L  ] [back]  [extra faces   ]
 16  [brim-front ] [brim-back ] [brim-sides     ]
 24  [   extra   ] [   extra  ] [   extra       ]
 32  [face-plate-N][face-plate-S][face-plate-E] [face-plate-W]
 40  [beak                    ] [horn-L ] [horn-R]
 48  [horn-L-sides            ] [horn-R-sides    ]
 56  [  reserved / empty      ] [   reserved     ]
 64
```

### plny_tengu_wings.png (pixel layout guide)
```
  X →  0        8       16       24       32       40       48       56       64
Y ↓
  0  [spine-N][spine-S][spine-E][spine-W]  [  reserved  ]
  8  [spine-up][spine-dn]  [  reserved                  ]
 13  [L-wing-upper-N  ] [L-wing-upper-S  ]  [R-wing-upper-N ] [R-wing-upper-S ]
 19  [L-wing-upper-E] [L-wing-upper-W]  [R-wing-upper-E] [R-wing-upper-W]
 25  [L-wing-upper-UP] [L-wing-upper-DN] [R-wing-upper-UP] [R-wing-upper-DN]
 31  [L-lower-N         ] [L-lower-S         ] [R-lower-N ] [R-lower-S ]
 39  [L-lower-E][L-lower-W] [L-tip-N] ... [R-lower-E][R-lower-W] [R-tip-N]
 47  [L-lower-UP] [L-lower-DN] [R-lower-UP] [R-lower-DN]
 53  [  reserved                                          ]
 64
```
> These are approximate guides. Open `plny_tengu_helmet.json` in Blockbench
> to see the exact UV rectangles and paint your texture to match them.

## Optional: Worn Armor Textures

To have custom visuals when chestplate / leggings / boots are **worn on the
player's body** (not just in inventory), add these files:

```
plugins/Oraxen/pack/textures/models/armor/
    plny_tengu_layer_1.png   ← chestplate + boots layer  (64 × 32 px)
    plny_tengu_layer_2.png   ← leggings layer            (64 × 32 px)
```

Then uncomment the `Mechanics: Armor: texture: plny_tengu` lines in
`plny_tengu_armor.yml` for the chestplate, leggings, and boots items.

> **Important:** These layer files follow the exact same layout as vanilla
> `armor/iron_layer_1.png` / `armor/iron_layer_2.png`. Use the vanilla
> template as a pixel guide.
