# Content provenance and AI assistance

2026-11-09 · Production constraint; platform-policy verification outstanding

The user plans solo development, wants human-made art and would like to avoid a Steam AI label while using as much AI assistance as rules permit. This is an important production preference. It is not a guarantee this project can claim a particular disclosure status.

## What is already in the prototype

The Godot prototype includes AI-authored gameplay code/tests, AI-drafted dialogue and item/story text, and procedural meshes and a coin sound authored through AI-assisted code. The internal design documents are also AI-assisted. These are development history, not a certification that any shipped material is exempt from disclosure.

Do not describe the existing prototype as wholly human-authored because its meshes are simple or because no image generator was used. Do not assume that manually editing generated output removes any disclosure requirement. Treat the prototype as a behavioural reference and audit reused material before production shipping.

## Preferred production boundary

Human-made art is user-confirmed. Human-authored release text is a conservative proposal pending policy verification, rather than an additional restriction the user has already imposed. The final assistance boundary should use the actual current rules.

| Area | Proposed practice |
| --- | --- |
| Art, models, textures, UI icons, animation, music, sound and voices | Human-created original work or licensed assets with known provenance. Check vendor disclosures and licences. Do not assume every marketplace pack is human-made. |
| Dialogue, item descriptions, localisation, trailers and store copy | Human-authored release content. Existing generated prototype text is a placeholder for that process. Preserve accurate records of tools and contributions; a rewrite is not a promise about platform classification. |
| Runtime generation | No runtime generative dialogue, art or voice in the proposed game. No cloud AI service is required to play. |
| Internal planning, research, review and production assistance | Useful candidate areas for AI assistance, with human decisions and checking. Their disclosure treatment still depends on the actual current platform wording. |
| Tests, tools, editor automation and code | Establish which uses Steam's current policy distinguishes from player-facing generated content. Do not assume that compiled code, Blueprint logic or AI-written tools are automatically included or excluded. Review the actual policy before selecting the production assistance boundary. |

Conventional procedural modelling, material graphs and lighting systems have distinct purposes from generative AI. For any particular tool, establish what was actually used rather than deciding provenance from the engine name or a marketing label.

## Verification required

The official [Steam content survey](https://partner.steamgames.com/doc/gettingstarted/contentsurvey) could not be fetched in this environment: the proxy rejected the destination. Consequently this document does **not** assert Steam's current treatment of AI-assisted code, internal tools, edited generated content or AI disclosure labels. Do not substitute a remembered older version of the policy or third-party commentary for the current survey wording.

When access is available, record the page/survey version or dated capture, the relevant wording and how it applies to this game's actual shipped and development material. Resolve ambiguous uses through Steamworks support rather than designing around an assumed loophole. Complete the survey truthfully. A preference to avoid a label cannot override required disclosure.

If allowed assistance is narrower than expected, choose deliberately between limiting that assistance, changing production content/workflow and accepting any required disclosure. Until that check, the GDD and engine recommendation are planning decisions; neither is an assurance about release eligibility or labels.

## Lightweight asset/content record

For each shipped asset or content bundle, record its author/source, licence, relevant tool use, human edits, distribution rights and any vendor AI statement. Track reusable prototype code and text alongside art. Keep receipts and licence files. This is especially useful for a solo project mixing custom art with packs.

The next concrete action is official policy verification and a production content audit before importing the prototype's dialogue or choosing a code-generation workflow for the Unreal slice. No new generated player-facing assets were created as part of this GDD task.
