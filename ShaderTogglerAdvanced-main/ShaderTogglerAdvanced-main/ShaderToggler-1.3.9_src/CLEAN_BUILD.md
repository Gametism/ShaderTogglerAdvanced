# Clean build guidance (source-only)

This repository contains only **source**. To reduce false positives from heuristic AV engines:
- Build with MSVC Release without LTCG and without /OPT:REF or /OPT:ICF.
- Link with `/Brepro /DYNAMICBASE /NXCOMPAT /INCREMENTAL:NO /guard:cf` (and `/SAFESEH` for Win32).
- Keep PDBs when distributing to allow vendors to symbolicate.
- Do **not** pack or obfuscate binaries.

These settings are already encoded in the `.vcxproj` files for Release.
