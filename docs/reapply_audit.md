# Reapply Audit From Known-Good 22:08 Build

Baseline: `0cb702bf` (`Sun May 31 22:08:17 2026`). Calculator verified by user: menus work; pink border works; R visible.

Menu lock: do not replay changes to `console.*`, `main.*`, `menuGUI.*`, `fileGUI.*`, `textGUI.*`, `catalogen.cpp`, or broad `graphicsProvider.*` changes. Only accepted UI edit: make `drawRecordingIndicator()` a no-op.

| # | Commit | Time | Improvement | Area | Menu risk | Reapply action |
|---:|---|---|---|---|---|---|
| 1 | `6440f168` | 05-31 22:21 | chore: polish audit tui and prune stale tools | calculator logic/build | no | replay |
| 2 | `afbaaae0` | 05-31 22:50 | chore: enhance audit tui and prune stale files | calculator logic/build | no | replay |
| 3 | `88355943` | 05-31 22:53 | docs: refresh queue audit status | tests/docs/tools | no | replay |
| 4 | `366468ec` | 05-31 23:09 | feat: add optimisation derivative working routes | calculator logic/build | no | replay |
| 5 | `3f598601` | 05-31 23:28 | feat: add quotient derivative working route | calculator logic/build | no | replay |
| 6 | `cdcc01cf` | 05-31 23:35 | test: add substitution queue rows | tests/docs/tools | no | replay |
| 7 | `95f112df` | 05-31 23:44 | test: add more substitution queue rows | tests/docs/tools | no | replay |
| 8 | `cc7a47d7` | 06-01 00:07 | chore: polish audit tui and tidy project | tests/docs/tools | no | replay |
| 9 | `6daa4c83` | 06-01 00:25 | feat: improve polynomial antiderivative output | calculator logic/build | no | replay |
| 10 | `ac8839f9` | 06-01 00:40 | feat: improve queue strict markers | calculator logic/build | no | replay |
| 11 | `fe0007f2` | 06-01 00:55 | feat: add definite substitution working route | calculator logic/build | no | replay |
| 12 | `6641f7f5` | 06-01 01:17 | feat: add linear rational integration route | calculator logic/build | no | replay |
| 13 | `9aa6cb55` | 06-01 01:33 | feat: add explicit quadratic root lines | calculator logic/build | no | replay |
| 14 | `d1c463fc` | 06-01 01:45 | feat: add sqrt limit markers | calculator logic/build | no | replay |
| 15 | `bee0c1fd` | 06-01 01:58 | improve audit tui and tidy project files | calculator logic/build | no | replay |
| 16 | `a83c382d` | 06-01 02:07 | improve numeric rounding markers | calculator logic/build | no | replay |
| 17 | `192573e5` | 06-01 02:18 | improve separable de working | calculator logic/build | no | replay |
| 18 | `0cfb6686` | 06-01 02:39 | improve quadratic root ordering | calculator logic/build | no | replay |
| 19 | `11ae8c2b` | 06-01 02:51 | improve exponential solve route | calculator logic/build | no | replay |
| 20 | `887e1883` | 06-01 03:09 | improve audit tui and numeric log | calculator logic/build | no | replay |
| 21 | `56e486b3` | 06-01 03:22 | improve radical integration route | calculator logic/build | no | replay |
| 22 | `7a199181` | 06-01 03:29 | improve compact radical routes | calculator logic/build | no | replay |
| 23 | `1e42b385` | 06-01 03:41 | improve audit tui and sine integration | calculator logic/build | no | replay |
| 24 | `ee251855` | 06-01 03:51 | improve ln squared integration format | calculator logic/build | no | replay |
| 25 | `1ef83ab3` | 06-01 04:10 | improve audit tui and definite ln route | calculator logic/build | no | replay |
| 26 | `e8e08a6e` | 06-01 04:26 | improve exponential solve working | calculator logic/build | no | replay |
| 27 | `2349eca3` | 06-01 04:51 | improve periodic trig solve working | calculator logic/build | no | replay |
| 28 | `49abf0d7` | 06-01 05:18 | improve audit tui and strict markers | calculator logic/build | no | replay |
| 29 | `014c23dd` | 06-01 05:29 | add exp product derivative route | calculator logic/build | no | replay |
| 30 | `b7fa6dde` | 06-01 05:45 | improve audit tui and arctan route | calculator logic/build | no | replay |
| 31 | `efc34387` | 06-01 06:04 | fix ordered cubic derivative output | calculator logic/build | no | replay |
| 32 | `8c320e0d` | 06-01 06:12 | add cosine by-parts integration route | calculator logic/build | no | replay |
| 33 | `4ac59604` | 06-01 06:21 | add sine by-parts integration route | calculator logic/build | no | replay |
| 34 | `122919c1` | 06-01 06:29 | add reverse chain integration routes | calculator logic/build | no | replay |
| 35 | `1ff3e7b3` | 06-01 06:42 | improve linear solve working markers | calculator logic/build | no | replay |
| 36 | `7f686707` | 06-01 06:53 | polish audit progress tui | calculator logic/build | no | replay |
| 37 | `17d931ae` | 06-01 07:10 | add vector subtraction working route | calculator logic/build | no | replay |
| 38 | `8b598d60` | 06-01 07:18 | add parts and apart working routes | calculator logic/build | no | replay |
| 39 | `3433eebb` | 06-01 07:25 | add rational solve working route | calculator logic/build | no | replay |
| 40 | `7b0e4666` | 06-01 07:33 | add definite substitution working route | calculator logic/build | no | replay |
| 41 | `0e314fd5` | 06-01 07:46 | add affine trig integration route | calculator logic/build | no | replay |
| 42 | `6f91bce1` | 06-01 08:05 | improve audit tui and trig parts routes | calculator logic/build | yes | split; replay only math/tool files, skip menu UI hunks |
| 43 | `bf6defbf` | 06-01 08:18 | add reciprocal affine integration route | calculator logic/build | no | replay |
| 44 | `a364fc7e` | 06-01 08:33 | add binomial series working routes | calculator logic/build | no | replay |
| 45 | `b6b16b4d` | 06-01 08:33 | improve audit progress tui | tests/docs/tools | no | replay |
| 46 | `b806a228` | 06-01 08:52 | add circle and vector working routes | calculator logic/build | no | replay |
| 47 | `92fe3204` | 06-01 09:10 | improve quadratic working routes | calculator logic/build | no | replay |
| 48 | `a9c35020` | 06-01 09:37 | hide F6 file menu and add reciprocal log sums | calculator logic/build | yes | split; replay only math/tool files, skip menu UI hunks |
| 49 | `5510d21a` | 06-01 09:51 | fix recording indicator and by-parts working | calculator logic/build | yes | split; replay only math/tool files, skip menu UI hunks |
| 50 | `92e770a8` | 06-01 09:58 | improve basic integration working | calculator logic/build | no | replay |
| 51 | `9ece3344` | 06-01 09:59 | add trig identity definite integral queue cases | tests/docs/tools | no | replay |
| 52 | `326d8079` | 06-01 10:23 | fix file menu and status indicator | calculator logic/build | yes | split; replay only math/tool files, skip menu UI hunks |
| 53 | `f539709b` | 06-01 10:45 | improve simplify cancellation working | calculator logic/build | yes | split; replay only math/tool files, skip menu UI hunks |
| 54 | `a16912cd` | 06-01 10:51 | handle proportional simplify cancellation | calculator logic/build | no | replay |
| 55 | `12387b79` | 06-01 11:52 | fit calculator build and restore file menu | calculator logic/build | yes | split; replay only math/tool files, skip menu UI hunks |
| 56 | `05075e5d` | 06-01 12:06 | expand integration working routes | calculator logic/build | no | replay |
| 57 | `15a5c744` | 06-01 12:09 | record latest integration queue rows | tests/docs/tools | no | replay |
| 58 | `f1991f69` | 06-01 12:30 | fix console file menu and recording indicator | tests/docs/tools | yes | split; replay only math/tool files, skip menu UI hunks |
| 59 | `d68651da` | 06-01 12:46 | fix console menu and status indicator | tests/docs/tools | yes | split; replay only math/tool files, skip menu UI hunks |
| 60 | `023c6534` | 06-01 12:47 | add mixed integration queue cases | tests/docs/tools | no | replay |
| 61 | `6d228dd6` | 06-01 12:50 | update queue verification status | tests/docs/tools | no | replay |
| 62 | `ddf2a9be` | 06-01 13:06 | improve working routes and restore solve menu | calculator logic/build | yes | split; replay only math/tool files, skip menu UI hunks |
| 63 | `965f6d37` | 06-01 14:43 | fix: restore menus and add log-product solve working | calculator logic/build | yes | split; replay only math/tool files, skip menu UI hunks |
| 64 | `f9087687` | 06-01 15:15 | feat: broaden differentiation working routes | calculator logic/build | no | replay |
| 65 | `4297cfcb` | 06-01 15:15 | size: stub hidden plotting runtime | calculator logic/build | yes | split; replay only math/tool files, skip menu UI hunks |
| 66 | `a01f8741` | 06-01 15:27 | feat: add general chain rule working | calculator logic/build | no | replay |
| 67 | `2511b7c5` | 06-01 15:54 | fix: restore khicas menu input path | docs/tests | yes | split; replay only math/tool files, skip menu UI hunks |
| 68 | `8feba7cf` | 06-01 16:18 | fix: restore khicas menu rendering path | docs/tests | yes | split; replay only math/tool files, skip menu UI hunks |
| 69 | `77fa0e21` | 06-01 16:18 | test: add random working fuzzer and range coverage | calculator logic/build | no | replay |
| 70 | `b6958e11` | 06-01 16:35 | fix: restore khicas menu state | tests/docs/tools | yes | split; replay only math/tool files, skip menu UI hunks |
| 71 | `0e83a86e` | 06-01 16:57 | fix: restore upstream khicas menus | docs/tests | yes | split; replay only math/tool files, skip menu UI hunks |
| 72 | `fa9ad2a8` | 06-01 17:07 | fix: normalize grouped working inputs | calculator logic/build | no | replay |
| 73 | `00400046` | 06-01 17:19 | fix: restore khicas menu rendering | docs/tests | yes | split; replay only math/tool files, skip menu UI hunks |
| 74 | `c7daa153` | 06-01 17:31 | feat: generalise affine working routes | calculator logic/build | no | replay |
| 75 | `a450ab77` | 06-01 17:43 | feat: integrate fractional power products | calculator logic/build | no | replay |
| 76 | `3121e357` | 06-01 18:13 | feat: broaden fallback working routes | calculator logic/build | no | replay |
| 77 | `59cfb880` | 06-01 18:51 | feat: add final working fallback route | calculator logic/build | no | replay |
| 78 | `f520a203` | 06-01 19:06 | feat: route no-op xform wrappers | calculator logic/build | no | replay |
| 79 | `8f034915` | 06-01 19:22 | fix: initialize khicas menu state | docs/tests | yes | split; replay only math/tool files, skip menu UI hunks |
