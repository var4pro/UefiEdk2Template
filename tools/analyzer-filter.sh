#!/usr/bin/env bash
# ===========================================================================
# Filter GCC -fanalyzer output (the LTO analysis link of "make analyzer") down
# to diagnostics that touch YOUR code.
#
# Usage: tools/analyzer-filter.sh <build log> <path prefix>...
#   e.g. tools/analyzer-filter.sh analyzer_full.log "$PWD/src/" "$PWD/include/"
#
# Expects the log to be produced with:
#   -fdiagnostics-plain-output -fdiagnostics-path-format=separate-events
# so every diagnostic looks like:
#   File.c: In function 'Foo':                      <- optional context line(s)
#   File.c:10:5: warning: ... [-Wanalyzer-...]      <- starts a diagnostic
#   Other.c:3:5: note: (1) entry to 'Bar'           <- events / notes
#   ...
#
# A diagnostic is KEPT if ANY of its lines (warning or event) is located in
# one of the given prefixes - not only the final location. With LTO the
# analyzer follows calls into EDK2 libraries, so a bug caused by your code is
# often reported inside e.g. MdePkg/Library/... and would be lost otherwise.
# Diagnostics entirely inside EDK2 code are dropped.
# ===========================================================================
set -euo pipefail

if [ $# -lt 2 ]; then
    echo "Usage: $0 <build log> <path prefix>..." >&2
    exit 2
fi

LOG_V=$1
shift
[ -f "$LOG_V" ] || { echo "[ERROR] log '$LOG_V' not found" >&2; exit 2; }

# Prefixes passed to awk as one newline-separated string
PREFIXES_V=$(printf '%s\n' "$@")

awk -v prefixes="$PREFIXES_V" '
BEGIN {
    n = split(prefixes, pfx, "\n")
    blk = ""; keep = 0; pending = ""; total = 0; kept = 0
}
# true if the line starts with one of the wanted path prefixes
function mine(line,    i) {
    for (i = 1; i <= n; i++)
        if (pfx[i] != "" && index(line, pfx[i]) == 1) return 1
    return 0
}
function flush() {
    if (blk != "") {
        total++
        if (keep) { printf "%s\n", blk; kept++ }
    }
    blk = ""; keep = 0
}
# start of a new diagnostic: "path:line:col: warning:" (or error)
/^[^ :][^:]*:[0-9]+:[0-9]+: (warning|error): / {
    flush()
    blk = pending $0
    pending = ""
    keep = mine($0) || keep_pending
    keep_pending = 0
    next
}
# event / note belonging to the current diagnostic
/^[^ :][^:]*:([0-9]+:[0-9]+:)? note: / && blk != "" {
    blk = blk "\n" $0
    if (mine($0)) keep = 1
    next
}
# context lines printed BEFORE a diagnostic ("In function", "inlined from", "In file included from", ...)
/: In (function|member function|lambda) |^In (function|member function) |^In file included from |^ +(from|inlined from) / {
    pending = pending $0 "\n"
    if (mine($0)) keep_pending = 1
    next
}
# anything else (build tool chatter) ends the current diagnostic
{
    flush()
    pending = ""; keep_pending = 0
}
END {
    flush()
    printf "[analyzer] %d diagnostic(s) total, %d touching your code\n", total, kept > "/dev/stderr"
}
' "$LOG_V"
