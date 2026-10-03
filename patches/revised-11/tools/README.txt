Tool layers of revised-11 (not part of the game build):

  04-BLENDER-SEMANTIC-EXPORT.diff     simutrans-blender-kit, base commit 0bc64b747a0beb693a6a43f78e37f48f3b8166d4
                                      (published main). Semantic pass + mask writer + .dat line. PROTOTYPE.
  05-OBJECT-STUDIO-SEMANTIC-IO.diff   simutrans-object-studio, base commit 020ecacd7501b78ab0b4a9ad98aa2036e0120e87.
                                      Mask reader/validator + .dat opt-in, no editor UI. PROTOTYPE.

Apply with: git apply <file>   (in a checkout of the base commit)
