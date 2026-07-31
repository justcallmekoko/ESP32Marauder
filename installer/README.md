# Installer manifest

`targets.json` is the canonical identity registry for the 22 stable firmware build targets. It maps each build flag to a stable target ID, aliases, release asset suffix, browser-facing chip family, and esptool chip name.

The stable release workflow runs `tools/installer_manifest.py` after every matrix build. The generator reads Arduino's emitted `flash_args` or `flash_args.txt`; it does not guess offsets, flash size, mode, frequency, or segment paths. It copies each actual flash segment into a target-specific release asset, calculates its size and SHA-256 digest, and emits:

- an update plan containing only the application image and preserving user data;
- a factory plan containing every emitted flash segment and requiring erase;
- a per-target manifest used only as an intermediate CI artifact.

The release job refuses to create `firmware-manifest.json` unless every registry target is present exactly once and every target agrees on stable channel, version, and full source commit. The final manifest and all referenced binaries are attached to the draft stable release.

Nightly releases intentionally do not publish installer manifests at launch.

## Local validation

```sh
python3 tools/installer_manifest.py --validate-registry
python3 -m unittest tools/test_installer_manifest.py -v
```
