# Apothecary model kit

Created on 2026-11-15 by Codex using authored Blender Python geometry, following the approved [generated corner concept](../../docs/art/concepts/brewing-shelving-corner-v1.png). This kit uses original procedural meshes, Blender's bundled Bfont for mesh lettering, and authored procedural material settings. No third-party marketplace assets or external textures were incorporated. The approved concept was generated with the built-in image_gen tool. The modelling script and its output are AI-assisted production work.

## Assets

| Asset | Dimensions in centimetres | Triangles | Convex collision hulls |
| --- | --- | --- | --- |
| Dawn Tonic | 14 × 10.46 × 25 | 4,208 | 2 |
| Moonlit Ink | 16 × 13.06 × 25 | 3,840 | 2 |
| Apothecary bench | 220 × 99.7 × 100 | 4,564 | 10 |
| Apothecary shelving | 124 × 48 × 196 | 4,576 | 10 |
| Brewing vessel | 58.13 × 42 × 42 | 2,636 | 1 |
| Moonleaf jar | 20 × 20.1 × 27.5 | 1,720 | 1 |
| Spring Water jug | 28.12 × 20 × 32 | 1,672 | 1 |
| Mortar and pestle | 23.02 × 22 × 27.33 | 1,240 | 1 |
| Brass pendant | 48 × 48 × 58 | 868 | 1 |
| Apothecary till | 56 × 34.8 × 36 | 1,964 | 2 |
| Supply crate | 70 × 51.01 × 67 | 5,944 | 2 |
| Oak floor tile | 199.6 × 99.7 × 3 | 1,880 | 1 |
| Plaster wall panel | 100 × 12 × 300 | 752 | 1 |
| Timber post | 13 × 16 × 300 | 188 | 1 |
| Arched window | 164 × 33 × 203.5 | 1,945 | 2 |
| Dawn recipe book | 43 × 33 × 3.86 | 2,496 | 1 |
| Drying Moonleaf | 27.29 × 27.3 × 65.04 | 508 | 1 |
| Potted fern | 58.96 × 59.87 × 73.63 | 1,576 | 1 |
| Dawn price card | 30 × 10 × 16 | 1,632 | 1 |
| Ink price card | 30 × 10 × 16 | 1,708 | 1 |
| Ink recipe book | 43 × 33 × 3.66 | 2,428 | 1 |
| Door surround | 230 × 25 × 222 | 564 | 3 |
| Brass stock ring | 18.6 × 18.6 × 0.52 | 576 | 0 |

Pivots are at floor/bottle-base centre. The bottle faces -Y in the Blender studio arrangement. FBX geometry compensates for Unreal's handedness conversion so the imported fronts face the aisle at identity rotation. Each FBX exports its asset at identity transform in centimetres. `shop-corner.blend` contains the editable meshes, collision objects and studio arrangement. `shop-kit-preview.png` is a Blender studio render. The manifest records material slots, dimensions and collision counts.

Unreal materials provide warm wood grain, sage paint, brass, cork, cream paper, dark label lettering, translucent amber glass and an opaque inner liquid volume. The label is mesh lettering and a geometric botanical sun emblem. The glass is a stylised surface treatment, with no fluid simulation.

The corrected cork uses the same flattened cross-section as the neck. Its inserted radius stays below 1.9 cm inside a minimum 2 cm opening, and the wider head starts above the glass lip. The modelling script checks these clearances. `bottle-fit-preview.png` shows the fit close up.

## Unreal installation

The meshes and materials are under `/Game/Shop/Art`. `/Game/Shop/Native/BP_DawnTonic` extends the native generic pickup class and is the Dawn Tonic recipe's output class. Its mesh has unit scale and zero offset, so the base sits at the carry/placement origin. The loose demonstration bottle is a Dawn Tonic product.

`Art_ApothecaryShelving` has four eight-slot sections at 20, 60, 100 and 140 cm. Six more sections divide the two retail tables. A section fills front-left to right, then back-left to right. Its first product determines the accepted type until empty. Customers share the item pickup operation and free a position before payment.

The whole checkout table is `BP_ShopTill`, with native world-space controls. The kit includes the counter, key, open leather pouch, and copper/silver/brass coins stamped 1/10/100. Payment coins appear in the pouch and change coins in three tray lanes. Each denomination displays up to 48 coins and always shows its exact count. Counting and returning coins use the authoritative till transaction. Confirmation credits cash once.

Two window wall modules replace measured greybox cubes and wall overlays. Their arched openings have separate translucent glass and timber frames. Original garden ground, fence and low-poly trees provide exterior views. Four visible brass pendants light brewing, preparation, checkout and the entrance, with cool daylight at both windows. Floor and ceiling retain 66 tiles each.

`Tools/install_shop_expansion.py` maintains the room, sections, physical checkout, customer presentations, recipes, supplier and customer flow. It preserves existing furniture transforms unless migrating the old counter or window geometry. Timestamped map backups are under `Saved/BeforeShopExpansion`. Old independent sale slots are replaced by sections. The station output remains independent.

The expansion installs `BP_MoonlitInk`, `DA_MoonlitInk` and `BP_InkCustomer`. Customers use the existing Epic Manny and Quinn meshes with the shared unarmed animation Blueprint. R cycles recipes and supplier packs. E performs world interaction. G sets a held item down on a clear flat surface.

Five original procedural sound cues are in `art/shop-audio`, with their waveform metadata in its manifest. `Tools/make_shop_sounds.py` authors mono 48 kHz, 16-bit PCM pickup, placement, brewing, coin and sale sounds through additive synthesis. The coin cue develops the damped 1100 Hz tone from the project's earlier Godot checkout. `Tools/install_shop_audio.py` imports them under `/Game/Shop/Audio`, sets Blueprint/recipe presentation references and saves till/crate bindings. The sounds use mathematical waveforms and deterministic noise. Audible mix and character presentation still need the user's hands-on assessment.

## Regeneration and verification

The modelling script supports Blender's background Python runner or Python 3.13 with the `bpy` package. The initial generation used bpy 5.3.0 installed in the repository's ignored `.tools/blender-python` directory.

From the repository root, using that local runtime, run

```powershell
$env:PYTHONPATH = "$PWD/.tools/blender-python"
python unreal/FantasyShopkeeper/Tools/model_shop_corner.py
```

Run import tools when Unreal Editor is closed. Use `UnrealEditor-Cmd` with the project path, `-run=pythonscript`, `-script=<absolute script path>`, `-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities`, `-unattended` and `-nullrhi`.

The seven retained Python tools each support a repeatable production task.

1. `Tools/model_shop_corner.py` regenerates the mesh kit, editable Blender scene, manifest and studio previews.
2. `Tools/make_shop_sounds.py` regenerates five WAV cues and their manifest.
3. `Tools/install_shop_art.py` imports meshes/materials and binds brewing/bottle presentation.
4. `Tools/install_shop_expansion.py` maintains the saved shop, shelf sections, checkout, windows, lights and navigation.
5. `Tools/install_shop_audio.py` imports and binds the sounds.
6. `Tools/check_shop_art.py` validates dimensions, collision, materials, sound bindings, 80 supported stock positions, two window openings and four pendant fixtures.
7. `Tools/render_shop_review.py` produces static review images without saving temporary scene dressing.

Run the three installers in that order. The expansion commandlet additionally needs `-ini:Engine:[/Script/NavigationSystem.NavigationSystemV1]:bWaitForAsyncLoadingBeforeBuildingNavigationAutomatically=False` for synchronous navigation building. The override applies to that process. The navigation agent supports a 34 cm radius and 180 cm height. The manifest is the complete inventory of the current 34 meshes and their authored convex hulls.

Five completed one-off interaction, brewing, sale, mannequin and small-till migration scripts have been removed. Their durable Blueprint and recipe assets are committed. Current character and till presentation setup lives in the expansion installer.

Run the 22 gameplay regressions with `Tools/check.ps1`, or use `-SkipBuild` after rebuilding. The actual-map keyboard workflow completes three visits and exercises recipes, stocking, G/E set-down and retrieval, supply purchases, walking customers, physical coin dispensing and sale confirmation while retaining mouse look. Focused tests cover product locking, capacity, fill order, reassignment, reach, stale quotes and cash consistency.

Launch `Tools/render_shop_review.py` through editor `-ExecCmds=py <absolute script>` with `-RenderOffscreen`. It uses temporal warm-up frames and writes to `Saved/ArtReview`. Hands-on playtesting belongs to the user.
