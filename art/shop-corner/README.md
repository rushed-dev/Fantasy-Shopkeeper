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

`Art_ApothecaryShelving` has four eight-slot sections at 20, 60, 100 and 140 cm. These are the shop's only stock sections, providing 32 positions; the two retail tables and their former six stock sections have been removed. A section fills front-left to right, then back-left to right. Its first product determines the accepted type until empty. Customers share the item pickup operation and free a position before payment.

The whole checkout table is `BP_ShopTill`, with a small leather-bound ledger, goods mat, stitched change mat, ceramic payment dish and wooden-handled brass PAID stamp. Small paper slips record tender and counted change. Copper/silver/brass coins retain their 1/10/100 denominations; controlled stacks and mat lanes use the existing guarded transaction. The former floating ticket board and raised keypad are removed. Legacy asset names `SM_TillKey` and `SM_CoinPouch` now contain the stamp and dish to preserve Blueprint references. The kit has 34 meshes and 87 collision hulls (the counter's former upright ticket hull is removed).

The larger checkout counter is moved inward from the entrance wall and faces into the room, with a matching customer waypoint and a verified entrance route.

Two window wall modules replace measured greybox cubes and wall overlays. Their arched openings have separate translucent glass and timber frames. Original garden ground, fence and low-poly trees provide exterior views. Four visible brass pendants light brewing, preparation, checkout and the entrance, with cool daylight at both windows. Floor and ceiling retain 66 tiles each.

`Tools/install_shop_expansion.py` maintains the room, sections, physical checkout, customer presentations, recipes, supplier and customer flow. It preserves existing furniture transforms unless migrating the old counter or window geometry. Timestamped map backups are under `Saved/BeforeShopExpansion`. Old independent sale slots are replaced by sections, and the retired retail tables and their six stock sections are removed rather than regenerated. The station output remains independent.

The expansion installs `BP_MoonlitInk`, `DA_MoonlitInk` and `BP_InkCustomer`. Customers use the existing Epic Manny and Quinn meshes with the shared unarmed animation Blueprint. R cycles recipes and supplier packs. E performs world interaction. G sets a held item down on a clear flat surface.

Five original procedural sound cues are in `art/shop-audio`, with their waveform metadata in its manifest. `Tools/make_shop_sounds.py` authors mono 48 kHz, 16-bit PCM pickup, placement, brewing, coin and sale sounds through additive synthesis. The coin cue develops the damped 1100 Hz tone from the project's earlier Godot checkout. `Tools/install_shop_audio.py` imports them under `/Game/Shop/Audio`, sets Blueprint/recipe presentation references and saves till/crate bindings. The sounds use mathematical waveforms and deterministic noise. Audible mix and character presentation still need the user's hands-on assessment.

## Regeneration and verification

The modelling script supports Blender's background Python runner or Python 3.13 with the `bpy` package. The initial generation used bpy 5.3.0 installed in the repository's ignored `.tools/blender-python` directory.

From the repository root, using that local runtime, run

```powershell
$env:PYTHONPATH = "$PWD/.tools/blender-python"
python unreal/FantasyShopkeeper/Tools/model_shop_corner.py
```

Run import tools when Unreal Editor is closed. Use the [shared tooling runner](../../docs/local-tooling.md) with `-Action Editor -EditorArguments`, passing `-run=pythonscript`, `-script=<absolute script path>`, `-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities`, `-unattended` and `-nullrhi`.

The seven retained Python tools each support a repeatable production task.

1. `Tools/model_shop_corner.py` regenerates the mesh kit, editable Blender scene, manifest and studio previews.
2. `Tools/make_shop_sounds.py` regenerates five WAV cues and their manifest.
3. `Tools/install_shop_art.py` imports meshes/materials and binds brewing/bottle presentation.
4. `Tools/install_shop_expansion.py` maintains the saved shop, shelf sections, checkout, windows, lights and navigation.
5. `Tools/install_shop_audio.py` imports and binds the sounds.
6. `Tools/check_shop_art.py` validates dimensions, collision, materials, sound bindings, 32 supported stock positions, two window openings and four pendant fixtures.
7. `Tools/render_shop_review.py` produces static review images without saving temporary scene dressing.

Run the three installers in that order. The expansion commandlet additionally needs `-ini:Engine:[/Script/NavigationSystem.NavigationSystemV1]:bWaitForAsyncLoadingBeforeBuildingNavigationAutomatically=False` for synchronous navigation building. The override applies to that process. The navigation agent supports a 34 cm radius and 180 cm height. The manifest is the complete inventory of the current 34 meshes and their authored convex hulls.

Five completed one-off interaction, brewing, sale, mannequin and small-till migration scripts have been removed. Their durable Blueprint and recipe assets are committed. Current character and till presentation setup lives in the expansion installer.

Run the 38 gameplay regressions with `Tools/check.ps1`, or use `-SkipBuild` after rebuilding. The actual-map keyboard workflow completes two sales, orders evening supplies, advances to morning and exercises recipes, stocking, G/E set-down and retrieval, supply purchases, walking customers, physical coin dispensing and sale confirmation while retaining mouse look. Focused tests cover product locking, capacity, fill order, reassignment, reach, stale quotes and cash consistency.

Launch `Tools/render_shop_review.py` through editor `-ExecCmds=py <absolute script>` with `-RenderOffscreen`. It uses temporal warm-up frames and writes to `Saved/ArtReview`. Hands-on playtesting belongs to the user.

For a focused mesh revision, set `SHOP_ART_ASSETS=SM_CheckoutCounter,SM_TillKey,SM_CoinPouch` in the process environment before running `model_shop_corner.py` and `install_shop_art.py`. Modelling updates those FBXs plus the complete source scene and manifest; studio preview renders are skipped. Importing updates only those meshes using existing materials and does not rewrite Blueprints or the map. Unset the variable for a full regeneration. Runtime rendering is the review source for this partial workflow.

`ShopDayLighting` fades sunlight, sky illumination and the two exterior window fills into evening while leaving the pendants lit. Advancing to morning adds a short camera fade. Lighting follows explicit shop phases without a real-time trading deadline.

The entrance open/closed sign is a native `ShopDoorSign` actor using the shared wood material and engine cube, with cream world-space lettering. It uses the ordinary E interaction and updates from the authoritative shop phase. The counter has no number grid: customers place the real bottle on its goods mat and payment appears immediately.


The supplier crate now supports a native cream-paper order book with flat PACK, LESS, MORE and REST tabs. These use the existing cube and material kit, with text on the page rather than a floating planning panel. A smaller instance of the existing supply-crate mesh appears beside the entrance for paid morning deliveries, and disappears after E unpacks its contents. The map and the 34-mesh asset inventory are unchanged. The overnight wind/cricket cue is original deterministic synthesis in `ShopDayLighting`, with no imported media or additional tool script.


Checkout presentation now moves a collision-free visual copy from the customer's carried bottle pose to the reserved counter position over 0.45 seconds. The actual item retains its shared slot ownership and becomes visible when the copy settles; cancellation restores its visibility. Tender coins fall into controlled dish positions with staggered clinks, change coins settle on the mat, and the PAID stamp moves after a committed sale. Existing original bottle-placement, coin and sale sounds are reused with revised gains and denomination/return pitches. No new media, meshes or tool scripts were added. This is a kinematic object gesture, not bespoke arm/finger animation or free coin physics.

The native `ShopStorageUpgrade` cabinet sits between the brewing bench and supplier. It reuses the existing dark wood/brass materials and engine cubes; purchasing reveals its upper drawers. It requires no map migration, generated assets or additional tooling. The rendered workflow captures its evening offer as `storage.png`.
