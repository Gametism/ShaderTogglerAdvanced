# Security / False-Positive Hygiene

This package contains only source code and project files. No prebuilt binaries are shipped.
To minimize antivirus false positives when you build locally or via CI, we recommend:

- Build **Release-Safe** configuration with:
  - No LTCG (no `/GL` or link-time code generation)
  - No identical COMDAT folding (`/OPT:ICF-`) and no reference optimization (`/OPT:REF-`)
  - Deterministic and reproducible linking (`/Brepro`)
  - Control Flow Guard (`/guard:cf`), DEP and ASLR enabled
  - Spectre mitigations enabled
  - Runtime library `/MD` (MultiThreadedDLL), keep PDBs
- Sign the resulting `.addon` with Authenticode if possible.
- Publish SHA-256 checksums alongside releases.
- If an AV flags the build, submit the exact hash to the vendor as a **false positive**.

These settings are already applied in the included `.vcxproj` files without altering functionality.