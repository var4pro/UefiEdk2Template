#!/usr/bin/env bash
# Rename the project: only .inf / .dsc file names + their content, and Dockerfile content.
# Also gives every renamed .inf / .dsc a fresh FILE_GUID / PLATFORM_GUID.
# Usage: ./rename_project.sh <NewProjectName>   (run from the project root)
set -euo pipefail

OLD="UefiEdk2Template"
NEW="${1:-}"

if [[ -z "$NEW" ]]; then
    echo "Usage: $0 <NewProjectName>"
    exit 1
fi
# Used in BASE_NAME / PLATFORM_NAME and as a sed replacement, so keep it a plain identifier
if [[ ! "$NEW" =~ ^[A-Za-z_][A-Za-z0-9_]*$ ]]; then
    echo "[ERROR] Name must contain only letters, digits and '_' (and not start with a digit)"
    exit 1
fi
if [[ "$NEW" == "$OLD" ]]; then
    echo "[EXIT] New name is the same as the old one"
    exit 0
fi

# 8-4-4-4-12 hex GUID
GUID_RE='[0-9A-Fa-f]{8}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{4}-[0-9A-Fa-f]{12}'

# find regular files matching the given -name tests, skipping .git, Build and edk2
find_targets() {
    find . \( -path ./.git -o -name Build -o -name edk2 \) -prune -o \
           -type f \( "$@" \) -print0
}

# Replace the GUID in a "KEY = <GUID>" line with a fresh uppercase one.
# Anchored at line start, so commented-out lines are left alone; alignment is kept.
regen_guid() {
    local key="$1" file="$2" guid
    grep -Eq "^[[:space:]]*${key}[[:space:]]*=[[:space:]]*${GUID_RE}" "$file" || return 0
    guid="$(uuidgen | tr '[:lower:]' '[:upper:]')"
    sed -i -E "s/^([[:space:]]*${key}[[:space:]]*=[[:space:]]*)${GUID_RE}/\1${guid}/" "$file"
    echo "    $key -> $guid"
}

echo "Renaming '$OLD' to '$NEW'..."

# 1. Content: .inf, .dsc, Dockerfile(s); new GUIDs for .inf / .dsc
while IFS= read -r -d '' f; do
    if grep -q -- "$OLD" "$f"; then
        sed -i "s/${OLD}/${NEW}/g" "$f"
        echo "  Updated content: $f"
        case "$f" in
            *.inf|*.dsc)
                regen_guid FILE_GUID "$f"
                regen_guid PLATFORM_GUID "$f"
                ;;
        esac
    fi
done < <(find_targets -name '*.inf' -o -name '*.dsc' -o -name 'Dockerfile' -o -name 'Dockerfile.*' -o -name '*.Dockerfile')

# 2. File names: only .inf and .dsc (directories are left alone)
while IFS= read -r -d '' f; do
    base="$(basename "$f")"
    [[ "$base" == *"$OLD"* ]] || continue
    new="$(dirname "$f")/${base//$OLD/$NEW}"
    if [[ -e "$new" ]]; then
        echo "[ERROR] $new already exists, not overwriting"
        exit 1
    fi
    mv -- "$f" "$new"
    echo "  Renamed: $f -> $new"
done < <(find_targets -name '*.inf' -o -name '*.dsc')

echo "Done!"