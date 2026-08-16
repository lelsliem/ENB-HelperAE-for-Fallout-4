# ENBHelperF4 v1.5.2

The multiversion 1.11.x build: one DLL for every 1.11.x runtime.

## What changed

- **Multiversion 1.11.x.** `F4SEPlugin_Version` declares all five 1.11.x runtimes —
  1.11.137, 1.11.159, 1.11.169, 1.11.191, 1.11.221 — instead of only `RUNTIME_LATEST`
  (1.11.221). All game access goes through the Address Library, so a single binary
  resolves the right addresses per runtime.
- **1.10.980 / 1.10.984 are NOT covered by this DLL.** Real testing on 1.10.984 caught
  this: the Address Library uses two different ID schemes across the NG era, and the
  singleton IDs baked into CommonLibF4 only exist in one family per runtime
  (`Failed to find offset for Address Library ID! Invalid ID: 4798212`). The package
  ships a separate pre-NG DLL for 1.10.980 / 1.10.984.
- **Pre-NG excluded.** 1.10.163 has a different memory layout; it needs its own build.

## Requirements

- Fallout 4 Next-Gen / AE — runtime 1.11.137 through 1.11.221 for this DLL
  (1.10.980 / 1.10.984 use the pre-NG DLL in the package)
- F4SE matching your runtime
- Address Library for F4SE Plugins (NG)

## Checksums

- 1.11.x DLL sha256: dea89ee7440d8715a6727159b9b5d66f079128a66795f2dde3cea15338df03b1
- pre-NG (1.10.980/1.10.984) DLL sha256: bf59a1a1f1de415973b05c2201e2f8c006085600469f63132ea2e1c2d00fc464

## Honest note

The layout-dependent flag is set (the plugin dereferences game structs directly). The
fields it reads are stable across the 1.11.x era, and this build has been in-game tested
on 1.11.221. The pre-NG DLL has been structurally verified but should get a real 1.10.984
session before public release.
