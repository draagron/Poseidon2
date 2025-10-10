#!/usr/bin/env bash

set -e

JSON_MODE=false
ARGS=()
for arg in "$@"; do
    case "$arg" in
        --json) JSON_MODE=true ;;
        --help|-h) echo "Usage: $0 [--json] <incident_description>"; exit 0 ;;
        *) ARGS+=("$arg") ;;
    esac
done

INCIDENT_DESCRIPTION="${ARGS[*]}"
if [ -z "$INCIDENT_DESCRIPTION" ]; then
    echo "Usage: $0 [--json] <incident_description>" >&2
    exit 1
fi

# Find repository root
find_repo_root() {
    local dir="$1"
    while [ "$dir" != "/" ]; do
        if [ -d "$dir/.git" ] || [ -d "$dir/.specify" ]; then
            echo "$dir"
            return 0
        fi
        dir="$(dirname "$dir")"
    done
    return 1
}

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if git rev-parse --show-toplevel >/dev/null 2>&1; then
    REPO_ROOT=$(git rev-parse --show-toplevel)
    HAS_GIT=true
else
    REPO_ROOT="$(find_repo_root "$SCRIPT_DIR")"
    if [ -z "$REPO_ROOT" ]; then
        echo "Error: Could not determine repository root" >&2
        exit 1
    fi
    HAS_GIT=false
fi

cd "$REPO_ROOT"

SPECS_DIR="$REPO_ROOT/specs"
mkdir -p "$SPECS_DIR"

# Find highest hotfix number
HIGHEST=0
if [ -d "$SPECS_DIR" ]; then
    for dir in "$SPECS_DIR"/hotfix-*; do
        [ -d "$dir" ] || continue
        dirname=$(basename "$dir")
        number=$(echo "$dirname" | sed 's/hotfix-//' | grep -o '^[0-9]\+' || echo "0")
        number=$((10#$number))
        if [ "$number" -gt "$HIGHEST" ]; then HIGHEST=$number; fi
    done
fi

NEXT=$((HIGHEST + 1))
HOTFIX_NUM=$(printf "%03d" "$NEXT")

# Create branch name from description
BRANCH_SUFFIX=$(echo "$INCIDENT_DESCRIPTION" | tr '[:upper:]' '[:lower:]' | sed 's/[^a-z0-9]/-/g' | sed 's/-\+/-/g' | sed 's/^-//' | sed 's/-$//')
WORDS=$(echo "$BRANCH_SUFFIX" | tr '-' '\n' | grep -v '^$' | head -3 | tr '\n' '-' | sed 's/-$//')
BRANCH_NAME="hotfix/${HOTFIX_NUM}-${WORDS}"
HOTFIX_ID="hotfix-${HOTFIX_NUM}"

# Create git branch
if [ "$HAS_GIT" = true ]; then
    git checkout -b "$BRANCH_NAME"
else
    >&2 echo "[hotfix] Warning: Git repository not detected; skipped branch creation for $BRANCH_NAME"
fi

# Create hotfix directory
HOTFIX_DIR="$SPECS_DIR/${HOTFIX_ID}-${WORDS}"
mkdir -p "$HOTFIX_DIR"

# Copy templates
HOTFIX_TEMPLATE="$REPO_ROOT/.specify/extensions/workflows/hotfix/hotfix-template.md"
POSTMORTEM_TEMPLATE="$REPO_ROOT/.specify/extensions/workflows/hotfix/post-mortem-template.md"

HOTFIX_FILE="$HOTFIX_DIR/hotfix.md"
POSTMORTEM_FILE="$HOTFIX_DIR/post-mortem.md"

if [ -f "$HOTFIX_TEMPLATE" ]; then
    cp "$HOTFIX_TEMPLATE" "$HOTFIX_FILE"
else
    echo "# Hotfix" > "$HOTFIX_FILE"
fi

if [ -f "$POSTMORTEM_TEMPLATE" ]; then
    cp "$POSTMORTEM_TEMPLATE" "$POSTMORTEM_FILE"
else
    echo "# Post-Mortem" > "$POSTMORTEM_FILE"
fi

# Add incident start timestamp to hotfix file
TIMESTAMP=$(date -u +"%Y-%m-%d %H:%M:%S UTC")
if grep -q "\[YYYY-MM-DD HH:MM:SS UTC\]" "$HOTFIX_FILE" 2>/dev/null; then
    # Replace first occurrence of placeholder with actual timestamp
    sed -i.bak "0,/\[YYYY-MM-DD HH:MM:SS UTC\]/s/\[YYYY-MM-DD HH:MM:SS UTC\]/$TIMESTAMP/" "$HOTFIX_FILE" 2>/dev/null || \
    sed -i '' "0,/\[YYYY-MM-DD HH:MM:SS UTC\]/s/\[YYYY-MM-DD HH:MM:SS UTC\]/$TIMESTAMP/" "$HOTFIX_FILE" 2>/dev/null || \
    true
fi

# Create reminder file for post-mortem
REMINDER_FILE="$HOTFIX_DIR/POST_MORTEM_REMINDER.txt"
cat > "$REMINDER_FILE" << EOF
POST-MORTEM REMINDER
====================

Hotfix ID: $HOTFIX_ID
Incident Start: $TIMESTAMP

⚠️  POST-MORTEM DUE WITHIN 48 HOURS ⚠️

Required Actions:
1. Complete post-mortem.md within 48 hours of incident resolution
2. Schedule post-mortem meeting with stakeholders
3. Create action items to prevent recurrence
4. Update monitoring and tests

Post-Mortem File: $POSTMORTEM_FILE

Do not delete this reminder until post-mortem is complete.
EOF

# Set environment variable
export SPECIFY_HOTFIX="$HOTFIX_ID"

if $JSON_MODE; then
    printf '{"HOTFIX_ID":"%s","BRANCH_NAME":"%s","HOTFIX_FILE":"%s","POSTMORTEM_FILE":"%s","HOTFIX_NUM":"%s","TIMESTAMP":"%s"}\n' \
        "$HOTFIX_ID" "$BRANCH_NAME" "$HOTFIX_FILE" "$POSTMORTEM_FILE" "$HOTFIX_NUM" "$TIMESTAMP"
else
    echo "HOTFIX_ID: $HOTFIX_ID"
    echo "BRANCH_NAME: $BRANCH_NAME"
    echo "HOTFIX_FILE: $HOTFIX_FILE"
    echo "POSTMORTEM_FILE: $POSTMORTEM_FILE"
    echo "HOTFIX_NUM: $HOTFIX_NUM"
    echo "INCIDENT_START: $TIMESTAMP"
    echo ""
    echo "⚠️  EMERGENCY HOTFIX - EXPEDITED PROCESS ⚠️"
    echo "Remember: Post-mortem due within 48 hours"
    echo "SPECIFY_HOTFIX environment variable set to: $HOTFIX_ID"
fi
