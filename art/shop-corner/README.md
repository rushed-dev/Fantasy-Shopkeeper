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

The meshes and materials are under `/Game/Shop/Art`. `/Game/Shop/Native/BP_DawnTonic` extends the native generic pickup class and is the Dawn Tonic recipe's output class. Its mesh has unit scale and zero offset, so the base sits at the carry/placement origin. The loose demonstration item also receives the bottle mesh while retaining its existing gameplay identity.

In `L_Shop`, `Art_BrewingBench` replaces the six measured support cubes at the brewing table. It keeps the 100 cm working height. `Art_ApothecaryShelving` stands alongside, with six new sale slots at 60 and 100 cm. Existing player-authored sale slots are retained. The upper surface can receive future stock positions after reach testing. Furniture uses individual board/leg collision hulls to keep shelf openings accessible. Two soft local lights establish warm/cool material contrast in the greybox room.

The installer backs up the map under `Saved/BeforeShopArt` before its first map edit. Repeating installation reimports the kit, reapplies its authored materials, and updates recipe presentation. Existing art furniture/slot actor transforms are retained on subsequent runs.

The brewing vessel and ingredient containers replace the placed station's placeholder meshes. A mortar and pendant dress the bench. The customer uses the existing Unreal First Person template's Manny mannequin and unarmed animation assets. Its presentation child is `BP_CustomerCharacter`. The migration preserves the placed customer's order and price and backs up the map under `Saved/BeforeCustomerCharacter`.

`BP_ShopTill` uses the modelled till chest, drawer handle, three coin wells and cream sign. `Tools/install_till.py` places it on a scaled bench serving as the checkout counter, in front of the existing customer's waiting position. The Outliner folder is `Checkout`. The till's UMG panel supplies manual totals, tender, coin-tray correction and confirmation. The pre-till map is backed up under `Saved/BeforeTill`.

`Tools/install_shop_expansion.py` adds the timber-and-plaster interior, floor and ceiling tiles, window surrounds, herbs, plants, books, prices, supply crate and customer flow. It replaces twelve measured retail/preparation support cubes with four scaled benches at the existing working height and footprint. The floor has 66 reusable tiles, and the ceiling repeats the tile with greater thickness. The existing thirty sale slots retain their positions. Windows currently use opaque, cool luminous panes. Exterior scenery and transparent window views are future work.

The expansion installs `BP_MoonlitInk`, `DA_MoonlitInk` and `BP_InkCustomer`. The ink customer uses the existing Epic Quinn mannequin with the shared unarmed animation Blueprint. Order entries choose presentation classes, requested products, prices and tender. R cycles recipes at the vessel and ingredient packs at the supplier. E uses the shared interaction operation. G sets a held item down onto a clear flat surface. Brass rings mark stock and output positions through their existing interaction boxes. The supply crate sends a complete combined, Moonleaf-only or water-only parcel to the station through the guarded economy transaction. Timestamped backups are under `Saved/BeforeShopExpansion`.

Five original procedural sound cues are in `art/shop-audio`, with their waveform metadata in its manifest. `Tools/make_shop_sounds.py` authors mono 48 kHz, 16-bit PCM pickup, placement, brewing, coin and sale sounds through additive synthesis. The coin cue develops the damped 1100 Hz tone from the project's earlier Godot checkout. `Tools/install_shop_audio.py` imports them under `/Game/Shop/Audio`, sets Blueprint/recipe presentation references and saves till/crate bindings. The sounds use mathematical waveforms and deterministic noise. Audible mix and character presentation still need the user's hands-on assessment.

## Regeneration and verification

The modelling script supports Blender's background Python runner or Python 3.13 with the `bpy` package. The initial generation used bpy 5.3.0 installed in the repository's ignored `.tools/blender-python` directory.

From the repository root, using that local runtime, run

```powershell
$env:PYTHONPATH = "$PWD/.tools/blender-python"
python unreal/FantasyShopkeeper/Tools/model_shop_corner.py
```

Run import tools when Unreal Editor is closed. Run `Tools/install_shop_art.py` through UnrealEditor-Cmd with `-run=pythonscript`, `-EnablePlugins=PythonScriptPlugin,EditorScriptingUtilities`, `-unattended`, and `-nullrhi`. Run `Tools/install_till.py`, `Tools/install_shop_expansion.py` and `Tools/install_shop_audio.py` sequentially after importing the kit and generating the sounds. The expansion uses the navigation override below. Run `Tools/check_shop_art.py` to validate all 23 meshes, 47 convex collision hulls, both recipes and mannequin presentations, sound bindings, interior geometry, reachable shelf positions and support beneath every sale slot. The installer selects Unreal's FBX importer for the current process to preserve authored collision and import settings.

`Tools/install_customer_character.py` installs the customer Character Blueprint and navigation bounds, builds navigation, checks a complete route to the sales bench and saves the map. For commandlet execution, add `-ini:Engine:[/Script/NavigationSystem.NavigationSystemV1]:bWaitForAsyncLoadingBeforeBuildingNavigationAutomatically=False` to allow the synchronous navigation build after asset compilation. This override applies to the importing process. The saved navigation agent supports a 34 cm radius and 180 cm height, covering the player and customer capsules.

Run the nineteen gameplay regressions with `Tools/check.ps1 -SkipBuild`. The walking regression samples the animation Blueprint's movement state and the customer's foot pose while moving. The map workflow completes three visits, switches recipes with R, sets down/retrieves an item with G/E, buys supplies and exercises actual till controls. It verifies that Escape and the Back button restore game input. `Tools/render_shop_review.py`, launched through editor `-ExecCmds=py <script>` with `-RenderOffscreen`, produces static corner/interior review images under `Saved/ArtReview`. Those renders add temporary display bottles and use temporal warm-up frames. The level is saved by the import tools. Hands-on gameplay assessment belongs to the user.
