# AV/False-Positive Guidance

This repository builds a standard ReShade add-on DLL (`.addon`) using MSVC on `windows-latest` without packers or obfuscation.
If a scanner flags the binary, it's typically a heuristic false positive common to game-mod overlays.

Mitigations included:
- Deterministic Release builds (no custom packers)
- Proper version resource (`src/version.rc`) for reputation signals
- Public CI logs (GitHub Actions)

If a detection appears:
1. Rebuild from source using this repo.
2. Submit the artifact URL to the AV vendor as a false positive.
