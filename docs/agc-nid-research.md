# Agc / AgcDriver NID research note

Unresolved PLT-import dataset for the PS5 `Agc` and `AgcDriver` libraries, collected from real
runtime captures so upstream (KytyPS5, shadPS4, and public stub-table projects) has the raw data set.

**Import identity:** every capture shows the guest importing these under
`Agc_v1][Agc_v1.1` and `AgcDriver_v1][AgcDriver_v1.1` — the PS5 ABI identities, *not* the PS4-era
Gen5 identities Kyty already registers graphics-command-buffer functions under (see
[libAgcDriver.cpp](../src/libs/libAgcDriver.cpp), which serves the same NID-space only under the
`Graphics5`/`Graphics5Driver` library names that no PS5 guest imports).

## Per-game unresolved counts

| Game | Agc NIDs | AgcDriver NIDs | Total |
|---|---|---|---|
| Stray | 67 | 17 | 84 |
| PAW Patrol | 67 | 17 | 84 |
| Hollow Knight Silksong | 44 | 12 | 56 |
| **Union (distinct)** | **76** | **22** | **98** |

## Mapping status summary

- `Agc` (76): **59** named by the [PS5 3.20 stub tables](https://github.com/DNNDHH/PS5-3.20_Libs/blob/main/libSceAgc.c);
  a further **1** (`7Wa3aeJgeVU`) named only from Kyty's own source (`AgcBranchPatchSetThenTarget`);
  **4** more have unverified name leads in an anonymous [Pastebin dump](https://pastebin.com/13BKREct)
  (`sceAgcAcbAtomicGds_0900`, `sceAgcUpdateInterpolantMapping_0100`, `sceAgcBranchPatchSetElseTarget_0300`,
  `sceAgcDcbAtomicGds_0900`); the remaining **12** are unmapped in every public source checked.
- `AgcDriver` (22): **16** named by [libSceAgcDriver.c](https://github.com/DNNDHH/PS5-3.20_Libs/blob/main/libSceAgcDriver.c);
  **6** unmapped.
- shadPS4 registers neither PS5 `Agc` library (its graphics NID work is PS4-era Gnm/GnmDriver), so there
  is nothing to port from there — same conclusion as the NP backend fill (commit `6b7686c9`).
- Cheapest concrete unblock: re-register Kyty's six existing `Gen5` functions under the guest's
  `Agc_v1][Agc_v1.1` identity — exact alias, no new behavior needed.

## lib Agc — 76 unresolved NIDs

| NID | Name | Source | Games importing |
|---|---|---|---|
| `+u6dKSLWM2o` | `sceAgcDcbStallCommandBufferParserGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `0ZOG0jc9nRg` | `sceAgcAcbRewindGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `0o3VDdtA6nM` | `sceAgcDcbSetIndexIndirectArgs` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `1-gUn1PI4Sw` | `sceAgcDcbAtomicMem` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `1tB0xkLNjcw` | `sceAgcDcbAtomicGdsGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `2ccJz9LQI+w` | `sceAgcDcbDmaDataGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `6nths4DHNrs` | `—` | no public mapping found | Hollow Knight Silksong |
| `7Wa3aeJgeVU` | `AgcBranchPatchSetThenTarget` | Kyty source (libAgcDriver.cpp, registered under Gen5/Graphics5 identity) | Stray, PAW Patrol |
| `7toV+elXqNM` | `sceAgcCbCondWrite` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `8Kly1JrJUlw` | `—` | no public mapping found | Hollow Knight Silksong |
| `9S4noWrUI0s` | `sceAgcDcbSetBaseDispatchIndirectArgsGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `AAeX-U5-P3M` | `—` | no public mapping found | Stray, PAW Patrol, Hollow Knight Silksong |
| `AFIh8SQkYlQ` | `sceAgcDcbSetIndexIndirectArgsGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `DwICrVxerkY` | `sceAgcAcbRewind` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `F8NLhWvFemI` | `sceAgcAcbQueueEndOfShaderActionGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `FcgdDM3MB+k` | `—` | no public mapping found | Stray, PAW Patrol, Hollow Knight Silksong |
| `FneFypEDRgY` | `—` | no public mapping found | Stray, PAW Patrol |
| `FuVbkyKlf+s` | `sceAgcCbCondWriteGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `G0jrLdvEqDw` | `sceAgcDcbMemSemaphore` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `GBCh3zCihoU` | `sceAgcDcbSetCxRegistersIndirectGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `GPbUp9jXQa8` | `sceAgcAcbWaitUntilSafeForRendering` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `GXBlM-ekzrI` | `sceAgcBranchPatchSetCompareAddress` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `ICkECTBxrMw` | `—` | no public mapping found | Hollow Knight Silksong |
| `J8YCgfKAMQs` | `sceAgcQueueEndOfPipeActionPatchGcrCntl` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `JOWmDrl+j20` | `sceAgcCbSetUcRegisterRangeDirectGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `K2mciNVxUCE` | `sceAgcSetNop` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `KjPeVduz6jU` | `sceAgcDcbPrimeUtcl2GetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `M0ttm8h7SKA` | `sceAgcAcbDmaDataGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `MDLD5Ly94Xk` | `sceAgcCbSetUcRegisterRangeDirect` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `MMlmJAL7N5w` | `sceAgcDcbSetBaseDrawIndirectArgsGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `OQTgEXyihvA` | `—` | no public mapping found | Hollow Knight Silksong |
| `P1CugZ99Uzc` | `sceAgcDcbEndOcclusionQueryGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `PxKWV2fVAps` | `sceAgcAcbDispatchIndirectGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `QhPDD513V0w` | `sceAgcDcbSetShRegisterDirectGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `RTpj-tIlvZc` | `—` | no public mapping found | Hollow Knight Silksong |
| `SwI6QxqwAC0` | `—` | no public mapping found | Hollow Knight Silksong |
| `T9fjQIINoeE` | `sceAgcQueueEndOfPipeActionPatchType` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `TGEZzUWLbrc` | `sceAgcCbSetUcRegistersDirectGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `UQGTw4xRlcM` | `sceAgcDcbSetUcRegistersIndirectGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `XKKuA6VkSRc` | `sceAgcAcbAtomicMem` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `XN+Iuu7XsM8` | `sceAgcDcbSetZPassPredicationEnableGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `Y-5vneiBtzk` | `sceAgcAcbEventWriteGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `aP1Ki9G3++4` | `sceAgcDcbSetUcRegisterDirectGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `ca4KPvp0qLQ` | `sceAgcDcbSetIndexSizeGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `da1Sm8-QDoU` | `sceAgcAcbAtomicMemGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `eCjKaqeeQ5s` | `sceAgcAcbPrimeUtcl2GetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `eWaWyFegzgQ` | `sceAgcAsyncRewindPatchSetRewindState` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `ebixW91gpPw` | `sceAgcAcbSetFlip` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `gQkqkLttcpw` | `sceAgcAcbAtomicGds_0900` | Pastebin "New Agc Nids" lead (unverified) | Stray, PAW Patrol, Hollow Knight Silksong |
| `hFQ9pUxoLQ4` | `—` | no public mapping found | Hollow Knight Silksong |
| `hcIxS8pmXF4` | `sceAgcAcbAtomicGdsGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `j4emHHndCPY` | `sceAgcDcbSetIndexBufferGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `jt3pl7EN17o` | `sceAgcDcbPrimeUtcl2` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `k0E7vkgqAuE` | `sceAgcUpdateInterpolantMapping_0100` | Pastebin "New Agc Nids" lead (unverified) | Stray, PAW Patrol |
| `mStuvI0zOtc` | `sceAgcDcbDrawIndexIndirectGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `mljzuGDZRQ4` | `sceAgcDcbSetIndexCountGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `ms1xVoZ-Vwc` | `sceAgcDcbBeginOcclusionQueryGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `n485EBnIWmk` | `sceAgcWaitRegMemPatchCompareFunction` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `nNlUtdDDvZ0` | `sceAgcDcbSetShRegistersIndirectGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `opR1JeJZCBU` | `sceAgcAcbSetWorkloadComplete` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `oz6zQq1JwCE` | `sceAgcDcbAtomicMemGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `pYoKs3lPy88` | `sceAgcDcbDrawIndirectMultiGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `pxx-GoOSdw4` | `—` | no public mapping found | Hollow Knight Silksong |
| `q4VuU-QsLOE` | `sceAgcAcbMemSemaphore` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `r98I08t+LOg` | `sceAgcDcbDrawIndexIndirectMultiGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `rP5xLdOf26k` | `sceAgcBranchPatchSetElseTarget_0300` | Pastebin "New Agc Nids" lead (unverified) | Stray, PAW Patrol |
| `rUuVjyR+Rd4` | `sceAgcDcbGetLodStatsGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `rVOmPz2RBlg` | `sceAgcAcbSetWorkloadsActive` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `szG7hz2yEhA` | `sceAgcAcbPrimeUtcl2` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `uZW-mqsxkrM` | `sceAgcCbBranchGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `vLrBL8DQiz8` | `sceAgcDcbSetPredicationDisableGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `vieBRwlh1Lw` | `—` | no public mapping found | Hollow Knight Silksong |
| `yUBESvCCJ4I` | `sceAgcCbSetShRegistersDirectGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `yheJGN-ay+A` | `sceAgcDcbSetBoolPredicationEnableGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |
| `zARR5aCmkoY` | `sceAgcDcbAtomicGds_0900` | Pastebin "New Agc Nids" lead (unverified) | Stray, PAW Patrol |
| `zg6u-N6Otxs` | `sceAgcDcbQueueEndOfShaderActionGetSize` | PS5 3.20 stub table (libSceAgc.c) | Stray, PAW Patrol |

## lib AgcDriver — 22 unresolved NIDs

| NID | Name | Source | Games importing |
|---|---|---|---|
| `+TN0oRTBxJQ` | `sceAgcDriverIsTraceInProgress` | PS5 3.20 stub table (libSceAgcDriver.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `+iAOE3jCnkk` | `—` | no public mapping found | Hollow Knight Silksong |
| `5l3IfCFJxBs` | `sceAgcDriverFindResourcesPublic` | PS5 3.20 stub table (libSceAgcDriver.c) | Stray, PAW Patrol |
| `8F5j7U-Bq6s` | `—` | no public mapping found | Hollow Knight Silksong |
| `FOwvmNlFLjM` | `sceAgcDriverRequestCaptureStop` | PS5 3.20 stub table (libSceAgcDriver.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `ICaGtkEIXTk` | `—` | no public mapping found | Hollow Knight Silksong |
| `LepGrgk77sM` | `sceAgcDriverGetOwnerName` | PS5 3.20 stub table (libSceAgcDriver.c) | Stray, PAW Patrol |
| `M9yBzRKkjPc` | `sceAgcDriverGetResourceName` | PS5 3.20 stub table (libSceAgcDriver.c) | Stray, PAW Patrol |
| `NghWEUXp1qM` | `sceAgcDriverGetResourceBaseAddressAndSizeInBytes` | PS5 3.20 stub table (libSceAgcDriver.c) | Stray, PAW Patrol |
| `SAfhzJPcjuk` | `sceAgcDriverRequestCaptureStart` | PS5 3.20 stub table (libSceAgcDriver.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `SCoAN5fYlUM` | `sceAgcDriverUnregisterAllResourcesForOwner` | PS5 3.20 stub table (libSceAgcDriver.c) | Stray, PAW Patrol |
| `VOMSpd9+vxU` | `sceAgcDriverSetResourceUserData` | PS5 3.20 stub table (libSceAgcDriver.c) | Stray, PAW Patrol |
| `WHIOMbb+iIU` | `—` | no public mapping found | Hollow Knight Silksong |
| `Xq5WmbwPTnQ` | `sceAgcDriverTriggerCapture` | PS5 3.20 stub table (libSceAgcDriver.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `chJWZcNSzjk` | `—` | no public mapping found | Hollow Knight Silksong |
| `emP3ckeS2uo` | `sceAgcDriverRegisterGdsResource` | PS5 3.20 stub table (libSceAgcDriver.c) | Stray, PAW Patrol |
| `ls4jfY576lw` | `sceAgcDriverGetResourceUserData` | PS5 3.20 stub table (libSceAgcDriver.c) | Stray, PAW Patrol |
| `mXn+K9E-wOA` | `sceAgcDriverGetResourceShaderGuid` | PS5 3.20 stub table (libSceAgcDriver.c) | Stray, PAW Patrol |
| `n5ElQVYsU1A` | `sceAgcDriverUnregisterWorkloadStream` | PS5 3.20 stub table (libSceAgcDriver.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `qspAL8bgcBY` | `sceAgcDriverIsSubmitValidationEnabled` | PS5 3.20 stub table (libSceAgcDriver.c) | Stray, PAW Patrol, Hollow Knight Silksong |
| `rI9lNAXPMIw` | `sceAgcDriverGetResourceType` | PS5 3.20 stub table (libSceAgcDriver.c) | Stray, PAW Patrol |
| `rJUyMrDdxJg` | `—` | no public mapping found | Stray, PAW Patrol, Hollow Knight Silksong |

## Provenance

- Runtime captures: untracked local logs (`stray_run1_printf.log`, `paw_run1_printf.log`,
  `silksong_run2_printf.log`, line pattern `unresolved PLT import patched to stub ... NID[lib_v1][lib_v1.1][Func]`).
- Name tables: PS5-3.20_Libs raw stub files (196 `Agc` entries, 95 `AgcDriver` entries), parsed
  `sprx_dlsym(__handle, "<NID>", &__ptr_<name>)`; Kyty source scan of `src/libs/` confirms zero of these
  98 NIDs is registered under any guest-visible library identity today.
- Negative sources checked for the unmapped 18: ps5-payload-dev SDK stubs, shadPS4 `src/core/libraries/`,
  Kyty upstream (InoriRus/Kyty), GitHub code search on the literal NID strings.

*Generated from the verified per-NID lookups in this working session; every table row above was
mechanically derived from the parsed tables, not transcribed by hand.*
