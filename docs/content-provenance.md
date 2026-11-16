# Content provenance and AI assistance

2026-11-15 · AI-generated artwork permitted; source and tool records required

The user prioritises making the game quickly, well and with rigor. AI-generated artwork is permitted alongside original and licensed assets. Select assets for visual consistency, gameplay readability, production effort and usable rights. The earlier human-made-art preference is superseded.

The earlier preference concerning a Steam AI label needs revisiting against the chosen release content and current platform requirements. It should not block prototyping with generated artwork. No particular disclosure status has been established.

## What is already in the prototype

The Godot prototype includes AI-authored gameplay code/tests, AI-drafted dialogue and item/story text, and procedural meshes and a coin sound authored through AI-assisted code. The internal design documents are also AI-assisted. These are development history, not a certification that any shipped material is exempt from disclosure.

Do not describe the existing prototype as wholly human-authored because its meshes are simple or because no image generator was used. Do not assume that manually editing generated output removes any disclosure requirement. Treat the prototype as a behavioural reference and audit reused material before production shipping.

## Production boundary

AI-generated visual artwork is user-authorised. The earlier human-authored release-text proposal was a planning choice, rather than a user requirement. Audio, voice and runtime generation remain separate scope decisions.

| Area | Proposed practice |
| --- | --- |
| Visual art, models, textures and UI icons | Original, licensed or AI-generated assets are permitted. Record source/tool, usage terms and edits. Review style consistency, readability and suitability in the running game. Generated 3D assets still need scale, pivots, collision, material and performance checks. |
| Dialogue, item descriptions, localisation, trailers and store copy | Review prototype text before reuse. Decide the production authorship workflow when this content is scheduled. Record tools and contributions accurately. |
| Animation, music, sound and voices | Choose sources and generation tools when these deliverables enter scope. Record usage rights and contributions. |
| Runtime generation | No runtime generative dialogue, art or voice in the proposed game. No cloud AI service is required to play. |
| Internal planning, research, review and production assistance | Useful candidate areas for AI assistance, with human decisions and checking. Their disclosure treatment still depends on the actual current platform wording. |
| Tests, tools, editor automation and code | Use agent assistance for implementation and verification. Review changes, build them and exercise the affected behaviour. Check release disclosure wording against actual usage when preparing submission. |

Conventional procedural modelling, material graphs and lighting systems have distinct purposes from generative AI. For any particular tool, establish what was actually used rather than deciding provenance from the engine name or a marketing label.

## Verification required

The official [Steam content survey](https://partner.steamgames.com/doc/gettingstarted/contentsurvey) could not be fetched in this environment: the proxy rejected the destination. Consequently this document does **not** assert Steam's current treatment of AI-assisted code, internal tools, edited generated content or AI disclosure labels. Do not substitute a remembered older version of the policy or third-party commentary for the current survey wording.

When access is available, record the page/survey version or dated capture, the relevant wording and how it applies to this game's actual shipped and development material. Resolve ambiguous uses through Steamworks support rather than designing around an assumed loophole. Complete the survey truthfully. A preference to avoid a label cannot override required disclosure.

Verify applicable tool terms before adopting generated assets for release, and verify the platform's current survey wording during release preparation. Keep development moving with traceable assets and truthful records.

## Lightweight asset/content record

For each shipped asset or content bundle, record its author/source, licence, relevant tool use, human edits, distribution rights and any vendor AI statement. Track reusable prototype code and text alongside art. Keep receipts and licence files. This is especially useful for a solo project mixing custom art with packs.

For the next slice, establish one coherent visual reference, use a small reusable asset set and assess the result in a packaged build. Record generated artwork as it is introduced. No generated artwork has been added by this policy update.

## Unreal checkpoint asset records, 2026-11-15

| Bundle | Source and tools | Contributions and verification |
| --- | --- | --- |
| Approved brewing corner concept | Built-in image_gen, retained under `docs/art/concepts` with its prompt record | AI-generated visual reference selected by the user. |
| Apothecary model kit | AI-assisted Blender Python geometry, bpy 5.3.0, bundled Bfont mesh lettering and procedural material graphs | 23 meshes with editable Blender/FBX source and a dimensions/material/collision manifest under `art/shop-corner`. Unreal checks cover 47 authored convex hulls, presentation, shelf support and map bindings. |
| Five shop sound cues | AI-assisted standard-library Python synthesis under `Tools/make_shop_sounds.py` | Original mathematical waveforms and deterministic noise. The coin cue develops the earlier Godot checkout tone. PCM WAV sources and duration/rate/peak metadata are under `art/shop-audio`. Imported cues serve pickup, placement, brewing, coins and sale confirmation. |
| Customer presentations and animation | Epic First Person template content already in this project | Manny and Quinn simple meshes with the existing unarmed animation Blueprint. Native AI and shared item interaction provide behaviour. Preserve Epic's applicable content licence records for release. |

The user is handling hands-on gameplay, audible mix and presentation assessment. Release licence and platform-disclosure review remains part of release preparation.


## Physical planning and delivery polish, 2026-11-16

The native order book, paper tabs and sealed delivery reuse the existing original crate/material kit and engine cube geometry. World-space labels are authored C++ text. `ShopDayLighting` synthesizes a short deterministic wind/cricket cue from filtered noise and a pulsed sine wave at 24 kHz mono; it uses no samples, external media or runtime AI service. These additions are AI-assisted code and procedural presentation. Rendering, transaction and persistence checks are recorded in `validation.md`; audible mix and hands-on assessment remain with the user.
