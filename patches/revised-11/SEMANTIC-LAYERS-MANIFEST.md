# XR32-SEMANTIC-MASK-PIPELINE-01: deliverables

LAB ONLY. Not committed anywhere, not pushed, not published.

## Engine layers (base: revised-10 tree = SVN r12322 + 6 patches, manifest a8a05ed7, lab tag `base-r10` = 4104ef8)

Apply in this order: **02, 01, 03**. Each intermediate state was built fresh (makeobj + game, MinGW SDL2 32-bit).
02, 01 and 03 applied to base-r10 reproduce the frozen candidate tree byte for byte (0 differing files).

| Layer | SHA-256 | Diffstat | Depends on | Status |
|---|---|---|---|---|
| 02-XR32-SEMANTIC-FORMAT.diff | `a99823cc0596b8c938b87e7c20f64767cd9f933c5d2e48042e18e80355eb7983` | 4 files, +273 / -4 | base-r10 | LAB CANDIDATE |
| 01-MAKEOBJ-SEMANTIC-INPUT.diff | `b7ef3e2fa650ea7bdc6d97b24c120a2698d8acb60b148f27dba5866fb731075e` | 7 files, +439 | 02 | LAB CANDIDATE |
| 03-RENDERER-SEMANTIC-COMPOSITION.diff | `ea464a5774c252a812256af1febd6e583beebab5371132fbdb0f8218b90fd59a` | 2 files, +118 / -3 | 02 | LAB CANDIDATE |
| XR32-SEMANTIC-MASK-PIPELINE-ENGINE.diff (02+01+03) | `ec97197fca4b007d2585f3d5747aeea6f5892179e33d2f28da97a85e7455c05a` | 12 files, +829 / -6 | base-r10 | combined, for convenience |

- **02** keeps the runtime's supported version at 1, so a base+02 runtime treats a v2 node as UNSUPPORTED and draws the legacy image. **03** raises it to 2 together with the composition.
- Lab commits: 02 = `db46f3e9`, 01 = `b444bca6`, 03 = `23276dd1` (branch `layers`). The frozen tree is `2c7ec87b`.
- Every layer is marked NOT FOR COMMIT in its comments, as all XR32 lab code is.

## Tool layers

| Layer | Base | SHA-256 | Diffstat | Depends on | Status |
|---|---|---|---|---|---|
| 04-BLENDER-SEMANTIC-EXPORT.diff | simutrans-blender-kit `0bc64b747a0beb693a6a43f78e37f48f3b8166d4` (published main) | `c323c57d34cd1705f6d2ab840df905506f829a9f7b90577b9ef5abea43f4a9c3` | 5 files, +345 / -3 | nothing in the Kit; its output needs 01 | PROTOTYPE |
| 05-OBJECT-STUDIO-SEMANTIC-IO.diff | simutrans-object-studio `020ecacd7501b78ab0b4a9ad98aa2036e0120e87` | `a0e2c1153bb7e4c78f1102df5a7d3e2f635efcba5971eb39b26f624c0501c815` | 4 files, +314 / -1 | nothing in OS; its `.dat` line needs 01 | PROTOTYPE (reader/validator/writer; no UI) |

Both apply cleanly to their bases (`git apply`). The applied files equal the tested copies.

## Other

| File | SHA-256 |
|---|---|
| forum/FORUM-UPDATE-DRAFT.txt (UNSENT; only the post text) | `93fbe0f4c407dcf616d6e1a3aa214e454ade26b1a93d8f4901ecc9294d841b2e` |
