# Armor Layer Textures — plny_tengu_set

Place worn-armor texture files here:

```
plugins/Oraxen/pack/textures/models/armor/
    plny_tengu_layer_1.png   ← worn layer for chestplate + boots  (64 × 32 px)
    plny_tengu_layer_2.png   ← worn layer for leggings            (64 × 32 px)
```

These files follow the standard vanilla armor layer layout.
Use `assets/minecraft/textures/models/armor/iron_layer_1.png` from any
vanilla jar as a pixel-position template.

After placing the PNGs, uncomment the `Mechanics: Armor:` section in
`plugins/Oraxen/items/plny_tengu_armor.yml` for each piece that should
show the custom texture while worn.
