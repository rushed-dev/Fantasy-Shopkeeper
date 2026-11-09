# Engine decision — production recommendation

2026-11-09 · Recommendation, not an executed migration

**Recommend Unreal as the leading production choice**, validated by one bounded visual/workflow evaluation before porting the game. Keep the Godot prototype as the playable behavioural reference. The user has chosen stylised, richly lit 3D, owns a Ryzen 7 9800X3D / 32 GB / RTX 5070 Ti development PC, and has moderate Blueprint experience with limited C++. Those facts make Unreal a credible solo workflow, rather than a choice based solely on screenshots.

The evaluation should determine whether Unreal delivers the desired room and crafting feel at a sustainable authoring cost. If it fails that practical test, improve the Godot version using Forward+ and the same art assets. Do not maintain two full games indefinitely.

## What the existing prototype establishes

The repository uses Godot 4.7.2, GDScript, GL Compatibility and procedural placeholder meshes. The source/tests together are roughly 3,100 lines at the time of assessment. Most of the value is the tested design: physical ownership, manual checkout, forgiving pacing, day transitions, saves and Fen's outcome. It is a small implementation to replace compared with a production game, but the replacement still needs real work.

Compatibility is not Godot's highest-fidelity renderer. Official Godot documentation describes Forward+, lightmapping, GI, reflections, volumetric fog and other modern rendering features. The current flat placeholder room is not evidence that Godot cannot produce attractive 3D. Conversely, choosing Unreal does not supply a coherent art direction, attractive character models or finished materials automatically.

No Unreal editor is installed in the inspected cloud environment, and no Unreal performance or productivity benchmark has been run. This recommendation combines the user's goals/experience, repository inspection and general engine capabilities. It is not a measured bake-off.

## Comparison for this game

| Need | Godot | Unreal | Practical consequence |
| --- | --- | --- | --- |
| Stylised room with rich light and materials | Forward+ supports a credible polished approach; baking/probes can suit a contained shop. Current Compatibility scene needs an art/rendering pass. | Mature lighting, material, effects and presentation tools; Lumen is an attractive candidate for a changing, luminous shop. | Unreal has the stronger fit for the chosen production presentation, provided it reaches the target performance. |
| Bottles, liquids and magical light | Custom materials and lights can serve a stylised treatment. | Material Editor and Niagara offer a strong workflow for authored effects. | Test transparent glass, coloured liquids, labels, reflections and many nearby emissive bottles early in both approaches. |
| Solo iteration | Existing project is compact and quick to run; GDScript and text scene files suit review and automation. | Heavier editor, asset/build pipeline and shader/cook steps. Existing Blueprint familiarity reduces the learning cost. | Measure actual edit/play/package loops on the user's machine, not engine launch time alone. |
| Interaction and business state | Already implemented and regression-tested here. | Rebuild interactions and transaction rules in engine-specific components and tests. | Neither engine choice changes the need for single-owner stock, atomic purchases and safe restores. |
| AI-assisted collaboration | Most current logic and tests are text files, making proposed changes easy to inspect. | Blueprint graphs and many content assets are binary/editor-authored. Text C++/data can help, but requires a working toolchain and review. | Do not promise that the current unattended coding workflow will transfer unchanged. |
| Art sourcing | General meshes/materials work with the importer; conversion and shader work remain. | Fab/Unreal-ready packs and sample workflows can shorten setup. Quality and licence/provenance still vary. | A curated, human-made kit is likely more realistic for a solo project than building every asset from scratch. |
| Developer hardware | This PC can comfortably begin a small Godot 3D project. | This PC is suitable for beginning a contained Unreal project. 32 GB is a workable starting point; larger projects/multitasking may benefit from more. | No hardware purchase is needed before the evaluation. Do not mistake the high-end development GPU for a minimum player target. |
| Player hardware | Rendering path can be selected to support different budgets. | Advanced rendering choices can increase GPU and memory costs; scalable alternatives must be deliberate. | Choose actual player targets and test packaged builds. Engine name alone cannot establish a minimum specification. |
| Licensing | Godot's engine licence is MIT, with required notices and third-party obligations. | Unreal uses a proprietary EULA with product-specific commercial terms. | Verify the applicable current game terms before committing. This assessment does not quote unverified royalty thresholds or seat fees. |

The chosen style can work in either engine. Unreal's advantage is the production route to that style and its fit with the developer's experience. Godot's advantage is the lighter, already working development loop. The answer is not that speed no longer matters once the concept is serious; solo authoring speed remains a release risk.

## Proposed Unreal workflow

Start **Blueprint-first** for the room, interaction feel, data-driven recipes and the first brewing test. Use small functions/components, stable item IDs and clear transaction entry points. Keep UI presentation separate from actions that consume ingredients, transfer ownership, receive payment or award story goods.

Do not begin with a large C++ framework. Add a small C++ boundary only when a concrete requirement—testing, ownership rules, performance, tooling or repeated complexity—makes its benefit clear and the developer can maintain it. Limited C++ experience should not become a dependency on opaque generated code. Current Steam AI rules must also be verified before deciding the production code-assistance policy.

Use source-controlled, reviewable recipe/customer/story data where practical. Blueprint graphs should be small enough to inspect in the editor; agree naming and folder conventions. Assess Git LFS or another suitable large-asset workflow before importing art packs. Binary assets are not reviewed or merged like the current GDScript files.

The practical editor workstation is the user's PC. The current Linux cloud session is proven for Godot tests; it is not an established Unreal build/editor environment. Choose and pin an appropriate supported Unreal version after checking required plugins/assets and the official toolchain requirements. Establish local packaging and test execution before promising cloud automation.

## Bounded evaluation

Use a proposed budget of three to five focused development days, then review. This is an investigation budget, not an estimate for the port or a commitment to the finished game's schedule. Preserve the existing Godot checkout/build; create the candidate as a separate project if the evaluation is authorised. No project or asset migration has started in this documentation task.

Build one shop corner containing a window, counter, shelf, bottles, coin tray and brewing bench. Use the [selected art direction](art-direction.md), a small coherent human-made/licensed asset kit, fixed camera comparisons and one morning/evening lighting change. Do not use the best Unreal showcase assets against raw Godot primitives.

The playable portion should let the player pick up a bottle, place it on a shelf, perform one short ingredient-to-bottle process and complete a simple sale. Add one Fen-style product effect in the room. Test a changing light and readable glass rather than assuming advanced rendering will handle them cheaply. No full customer cast, dialogue framework or campaign is needed for this decision.

Record:

- Packaged framerate, frame-time behaviour and memory use at stated resolution/settings. Provisional aim: 60 fps at 1080p on a chosen mainstream player configuration; the actual target GPU remains to be selected. Measure an appropriate lower-spec machine, not just the RTX 5070 Ti.
- Clarity of labels, shelf aiming, bottle silhouettes, change tray and magic effects under both light setups. Test opaque/stylised alternatives if transparency dominates cost.
- Cold-start/shader wait, normal edit/play turnaround, incremental compile if C++ is used, and a full Windows package. Capture actual timings rather than assumed relative speed.
- Hands-on time to add a second recipe, alter a customer/order condition and fix a transaction bug. Judge how much can be maintained comfortably through Blueprints.
- Asset import/conversion effort, style consistency, licences and provenance. Capture what still needs custom modelling, animation, texture work and audio.

Proceed with Unreal if the room meets the intended look, interaction remains satisfying, packaging works, performance has a plausible scalable path and the authoring loop is comfortable. If rendering looks good but the workflow is prohibitive, simplify and retest one specific issue before deciding. If Godot is revisited, compare a Forward+ scene using the same kit and capture rules.

Lumen, Nanite and complex shader features are candidates, not compulsory pillars of the design. A small shop may benefit more from controlled meshes, materials and lighting than from enabling every high-end feature.

## What migration would retain and replace

**Retain:** design decisions, story structure, product vocabulary, recipe/data schemas where suitable, transaction invariants, test scenarios and playtest evidence.

**Rebuild:** GDScript, Godot nodes/scenes, rendering/material resources, UI, input integration, actors, navigation, engine save integration and executable packaging. The existing tests need equivalent tests or playable assertions in Unreal. Do not describe them as already validating a port.

Prototype save files do not automatically become Unreal saves. Preserve the Godot checkpoint and decide whether a production importer is worthwhile; never silently overwrite those files. Introduce brewing into a coherent slice before porting every old menu and incidental prototype detail.

## Evidence and outstanding verification

Inspected locally on 2026-11-09: project.godot selects Compatibility; the playable project pins Godot 4.7.2; no Unreal editor was found. Read official Godot sources:

- [Godot feature documentation source](https://github.com/godotengine/godot-docs/blob/master/about/list_of_features.rst): renderers, GI and reflections. This is the documentation's development branch, not a benchmark or a guarantee of identical features in every engine version.
- [Godot engine licence](https://github.com/godotengine/godot/blob/master/LICENSE.txt): MIT terms. Individual third-party components/assets still need their own notices/licences.

These official pages are the intended next checks, but were blocked by the environment's destination policy during this assessment and were **not read successfully**:

- [Godot renderer comparison](https://docs.godotengine.org/en/stable/tutorials/rendering/renderers.html).
- [Unreal Lumen technical details](https://dev.epicgames.com/documentation/en-us/unreal-engine/lumen-technical-details-in-unreal-engine).
- [Unreal hardware/software requirements](https://dev.epicgames.com/documentation/en-us/unreal-engine/hardware-and-software-specifications-for-unreal-engine).
- [Unreal licensing](https://www.unrealengine.com/en-US/license).
- [Steam content survey](https://partner.steamgames.com/doc/gettingstarted/contentsurvey), discussed in [content provenance](content-provenance.md).

Documentation-domain additions were saved to the environment draft for review; saving a draft did not apply them. Current licence/disclosure details, chosen Unreal version, target player hardware and measured performance remain open. The absence of those checks is not evidence that Unreal is unsuitable, nor permission to assume particular legal or platform terms.
