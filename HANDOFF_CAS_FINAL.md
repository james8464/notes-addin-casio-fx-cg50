# handoff: finish CAS.g3a project

## context

repo: `/Users/james/Developer/CASIO`

branch: `main`

latest commits:

- `2110230d0 Prevent named mechanics arguments being misread`
- `e9e5615d6 Add missing SUVAT velocity acceleration route`
- `857523cdb Restore stock KhiCAS and improve CAS command support`

user goal:

finish `CAS.g3a` as the main a-level maths app, including paper 3 mechanics support, while keeping original KhiCAS behaviour stable. `khicasen.g3a` must remain identical to official upstream/source output. `CAS.g3a` can be modified.

## current problem

on calculator/emulator:

- custom added CAS functions still do not appear coloured in the console/editor
- F6 help on command sheet from the command menu does not show for custom functions

these were thought to be implemented in commit `857523cdb`, but user testing says it still does not work.

## hard rules

- use caveman mode
- do not patch `.g3a` binaries directly as final output
- source-build `.g3a`
- keep every `.g3a` under `2,097,152` bytes
- `CAS.g3a` is currently very tight: last build was `2,096,692 bytes`, only `460 bytes` spare
- do not weaken `CAS.g3a` pure maths unless needed
- stats/probability out of scope
- keep `khicasen.g3a` identical to upstream/source version
- keep old `SUVAT.py` and `SUVATprogram.mpy` in `calculator_files`

## important files

- custom working engine: `khicas/upstream/giac90_1addin/cascas_working.cc`
- custom command declarations / colouring likely area: `khicas/upstream/giac90_1addin/main.cc`
- command catalog + F6 help likely area: `khicas/upstream/giac90_1addin/catalogen.cpp`
- help pack builder: `tools/build/build_help_pack.py`
- generated help pack copied to calculator: `calculator_files/CAS.PAK`
- CAS docs: `docs/CAS_README.md`
- compile command: `./compile`

## known good state

last verification passed:

```sh
python3 tests/check_cas_p3_mechanics.py
python3 tests/check_help_examples.py
python3 tools/checks/check_help_quality.py
python3 tools/checks/check_catalog_scope.py
python3 tools/checks/check_removed_features.py
./compile cas
python3 tools/checks/check_g3a_size.py calculator_files/CAS.g3a
python3 tools/checks/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a
```

results:

- `OK CAS Paper 3 mechanics cases=34`
- `OK help examples=86`
- `OK help quality functions=65`
- `OK catalog scope kept_visible=64 removed=82`
- `OK removed features blocked=82`
- `CAS.g3a: 2096692 bytes`

## likely root cause areas

### syntax colouring

look in `main.cc` around the custom function-name detection. Current added functions include mechanics commands:

```txt
suvat force connected pulley lift varacc moment work power impulse momentum energy friction resolve incline projectile weight
```

and pure/custom functions already added elsewhere, likely:

```txt
range domain xform rewrite texpand tcollect
```

need verify the actual editor colouring code path. It may not use the helper currently patched. Search:

```sh
rg -n "suvat|force|color|colour|keyword|ident|syntax|strcmp\\(buf|catalog|F6|help" khicas/upstream/giac90_1addin/main.cc khicas/upstream/giac90_1addin/catalogen.cpp
```

do not assume `main.cc` helper is used by the console editor. Trace call sites.

### F6 command-sheet help

look in `catalogen.cpp`.

Need verify:

- F6 key path in command menu
- whether custom functions are in catalog rows correctly
- whether the command-sheet lookup is using `CAS.PAK`
- whether `CAS.PAK` is actually opened from calculator storage path
- whether selected command name passed to help lookup matches exact record key
- whether F6 is intercepted before EXE insert path

Likely debug search:

```sh
rg -n "KEY_CTRL_F6|F6|CAS.PAK|command sheet|help|catalog|CAT_CATEGORY|suvat|xform" khicas/upstream/giac90_1addin/catalogen.cpp khicas/upstream/giac90_1addin/main.cc
```

## required fix

make custom functions behave exactly like normal KhiCAS functions:

- function names colour in the console/editor while typing
- F4 command menu lists them in the right categories
- selecting a custom function works
- pressing F6 while browsing a custom function shows its command sheet/help
- help is detailed and matches expected arguments
- no CAS syntax errors or missing file messages when `CAS.PAK` is present

## commands to test

host tests:

```sh
cmake --build tools/working_engine/host/build
cp tools/working_engine/host/build/casio_host tools/host/khicas_host_runner
python3 tests/check_cas_p3_mechanics.py
python3 tests/check_help_examples.py
python3 tools/checks/check_help_quality.py
python3 tools/checks/check_catalog_scope.py
python3 tools/checks/check_removed_features.py
```

target build:

```sh
./compile cas
python3 tools/checks/check_g3a_size.py calculator_files/CAS.g3a
python3 tools/checks/check_g3a_metadata.py calculator_files/CAS.g3a --name CAS --internal @CAS --filename CAS.g3a
```

manual calculator/emulator test:

1. copy `calculator_files/CAS.g3a` and `calculator_files/CAS.PAK` to calculator
2. open CAS
3. type `suvat(` and confirm `suvat` colours like built-in KhiCAS commands
4. type `xform(` and confirm `xform` colours
5. open F4 command menu
6. highlight `suvat`
7. press F6
8. confirm command-sheet help opens, not insertion into console
9. repeat for `xform`, `range`, `friction`, `projectile`

## caution

space is tiny. Prefer fixes that reuse existing tables/functions rather than adding strings. If extra bytes are needed, shorten existing help/error wording, not behaviour.

Do not remove `CAS.PAK` support unless replacing it with something proven working and smaller.

Commit and push when done. Final response should include build size and exact verification commands.
