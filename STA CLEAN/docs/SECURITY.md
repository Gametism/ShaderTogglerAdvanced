# Security & Virus Scanners

Builds produced by this repository are standard Visual C++ DLLs, renamed to `.addon`/`.addon64` for ReShade to load.
Occasionally, some antivirus engines flag game overlays or hooking tools as suspicious. This is a **false positive**.

If you encounter a detection:
- Verify the artifact hash matches the GitHub Actions artifact hash.
- Build locally from source to reproduce.
- Report the false positive to your AV vendor.
