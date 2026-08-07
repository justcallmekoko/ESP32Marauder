# Installer manifest

`targets.json` is the canonical installer identity registry for the 22 stable firmware build targets. It maps each build flag to a stable target ID, aliases, release asset suffix, browser-facing chip family, and esptool chip name.

The normal release workflow, `.github/workflows/build_parallel.yml`, remains unchanged and continues producing its existing downloadable application binaries. Installer automation lives only in the additive `.github/workflows/build_installer_manifests.yml` workflow.

The installer workflow exports its build matrix directly from the normal workflow and validates every build flag against `targets.json`. Matrix parsing fails closed on unsupported syntax, missing fields, duplicates, or target drift. This keeps the existing normal-release matrix authoritative without requiring it to consume installer-specific configuration.

Each installer build reads Arduino CLI's expanded upload recipe and concrete build properties; it does not guess offsets, flash size, mode, frequency, or segment paths. It copies each actual flash segment into a target-specific asset, calculates its size and SHA-256 digest, and emits:

- an update plan containing only the application image and preserving user data;
- a factory plan containing every emitted flash segment and requiring erase;
- a per-target manifest used only as an intermediate CI artifact.

The combine job refuses to create `firmware-manifest.json` unless every registry target is present exactly once and every target agrees on stable channel, version, and full source commit.

## Triggers and release relationship

- Pull requests and relevant pushes to `develop` rebuild and validate the complete installer bundle as short-lived Actions artifacts.
- Manual dispatch is artifact-only; it does not create, publish, or modify a release.
- Publishing a non-prerelease GitHub release triggers an independent rebuild from that release tag. Only after all 22 targets and the combined manifest pass does the workflow attach the additive installer assets to the already-published release.
- The installer workflow never invokes or replaces the normal release workflow, and it never renames or removes the normal downloadable binaries.
- Every additive installer binary uses the `esp32_marauder_installer_` namespace so release attachment cannot collide with or overwrite a normal binary filename.

Nightly releases intentionally do not publish installer manifests at launch.

The isolation has one deliberate cost: stable release publication performs a second 22-target build so installer assets can include authoritative bootloader, partition, OTA-data, and application segments without changing the normal release jobs.

## Local validation

```sh
python3 tools/installer_manifest.py --validate-registry
python3 tools/installer_manifest.py --export-build-matrix
python3 -m unittest tools/test_installer_manifest.py -v
```
