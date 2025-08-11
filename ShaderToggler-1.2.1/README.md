## Texture toggling (compat mode)
This build includes persisted texture targets in `ShaderToggler.ini` and applies them when a group is activated.
On ReShade builds lacking `push_descriptors` support, texture override paths fall back to a safe no-crash mode.