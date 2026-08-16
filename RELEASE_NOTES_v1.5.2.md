# ENBHelperF4 v1.5.2

The multiversion Next-Gen build: one DLL for every Next-Gen runtime.

## What changed

- **Multiversion Next-Gen.** `F4SEPlugin_Version` now declares all seven Next-Gen runtimes
  — 1.10.980, 1.10.984, 1.11.137, 1.11.159, 1.11.169, 1.11.191, 1.11.221 — instead of only
  `RUNTIME_LATEST` (1.11.221). All game access already goes through the Address Library, so a
  single binary resolves the right addresses per runtime.
- **Pre-NG excluded.** 1.10.163 has a different memory layout; it is not covered by this DLL
  and needs its own build.

## Requirements

- Fallout 4 Next-Gen / AE (any runtime 1.10.980 → 1.11.221)
- F4SE matching your runtime
- Address Library for F4SE Plugins (NG)

## Checksums

- DLL sha256: 4a7b8bc7f0514ab886a0e1f477dc54c749d1af2b3e6a3fb7cddebf1bf0f0369a

## Honest note

The layout-dependent flag is set (the plugin dereferences game structs directly). The fields
it reads are stable across the NG era, but this build has only been in-game tested on
1.11.221. Smoke-test on 1.10.980 / 1.10.984 before publishing.
