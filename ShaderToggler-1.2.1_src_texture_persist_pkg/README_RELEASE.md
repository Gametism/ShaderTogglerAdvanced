# How to build locally

1. Open `ShaderToggler.sln` (if present) or the `.vcxproj` in Visual Studio 2022.
2. Build `Release | x64` and `Release | Win32`.
3. The `.addon` (or `.dll`) will be in your MSVC output directories.

# GitHub CI builds

- Push to any branch → GitHub Actions runs the `Build ShaderToggler` workflow.
- Artifacts appear as `ShaderToggler_Build` with the built .addon files.
