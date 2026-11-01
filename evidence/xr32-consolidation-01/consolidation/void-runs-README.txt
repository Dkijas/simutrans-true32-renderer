VOID - do not use. The perf scripts copied every build onto one shared perf-exe; cp hit ETXTBSY ("Text file busy")
and the run used the previous build under the wrong label. Idle run 2026-11-01 15:12: 3 failed copies; the 13:56 fps500
run: 1 (plus a concurrent perf_rezoom). Fixed by tools/fix_perf_exe.py (one file per build, hash in every row).
