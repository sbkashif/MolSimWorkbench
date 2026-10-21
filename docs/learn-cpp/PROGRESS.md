# Progress — learn-cpp

Where you are in the C++ track. Quizzes happen in chat; this records the result and what to re-drill.

| Ch | Read | Quiz | Re-drill |
|---|---|---|---|
| 1 | ✅ 2026-10-07 (technical) | ~6½ / 8 | `argc` counts the program name too (`run_simulation a.yaml b.xyz` → 3). Missing header → **compiler** ("undeclared identifier", has a line). Header present but `.cpp` not built → **linker** ("Undefined symbols", no line). Both re-drilled and stated back correctly. |
| 2 | next | | |

**Rule in your words (ch 1):** the header is for the compiler and the `.cpp` is for the linker. If a header is
missing, the compiler fails; if the `.cpp` is missing but the header declares it, compiling passes and the
linker fails.
