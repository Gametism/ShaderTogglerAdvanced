# Clean Build Instructions (Windows / Visual Studio)

1. Open the solution or project in Visual Studio 2022.
2. Select configuration: **Release** and platform: **x64** or **Win32**.
3. Build the `ShaderToggler` project.
4. The resulting `.addon64`/`.addon` file will be in the `out/<Platform>/<Configuration>/` or project output dir per your settings.

Notes:
- Project files are configured to avoid common AV heuristics (no LTCG, no section folding, reproducible link, CFG/DEP/ASLR, spectre).
- No functionality was changed.