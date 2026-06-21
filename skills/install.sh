#!/usr/bin/env bash
set -euo pipefail

SKILLS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PLATFORM="all"
SELECTED_SKILLS=""

usage() {
    echo "Usage: $0 [--platform claude-code|cursor|copilot|all] [--skills skill1,skill2]"
    exit 1
}

while [[ $# -gt 0 ]]; do
    case $1 in
        --platform) PLATFORM="$2"; shift 2 ;;
        --skills)   SELECTED_SKILLS="$2"; shift 2 ;;
        -h|--help)  echo "Usage: $0 [--platform claude-code|cursor|copilot|all] [--skills skill1,skill2]"; exit 0 ;;
        *) echo "Unknown option: $1"; usage ;;
    esac
done

# Build list of skill folders to install
get_skills() {
    if [[ -n "$SELECTED_SKILLS" ]]; then
        echo "$SELECTED_SKILLS" | tr ',' '\n'
    else
        for d in "$SKILLS_DIR"/*/; do
            [ -f "$d/skill.md" ] && basename "$d"
        done
    fi
}

install_claude_code() {
    local skill="$1"
    local src="$SKILLS_DIR/$skill"
    local dest="$HOME/.claude/plugins/document-filters-$skill"
    if [ ! -f "$src/skill.md" ]; then
        echo "  SKIP $skill (skill.md not found)"
        return
    fi
    mkdir -p "$dest"
    cp -r "$src/." "$dest/"
    echo "  OK   claude-code: $dest"
}

install_cursor() {
    local skill="$1"
    local src="$SKILLS_DIR/$skill/skill.md"
    local dest=".cursorrules"
    if [ ! -f "$src" ]; then
        echo "  SKIP $skill (skill.md not found)"
        return
    fi
    # Idempotent: remove old block then re-append
    local marker="<!-- document-filters-$skill -->"
    if [ -f "$dest" ]; then
        # Remove existing block for this skill
        perl -i'' -0pe "s/\Q$marker\E.*?\Q$marker\E\n?//s" "$dest" 2>/dev/null || true
    fi
    printf "\n%s\n" "$marker" >> "$dest"
    cat "$src" >> "$dest"
    printf "\n%s\n" "$marker" >> "$dest"
    echo "  OK   cursor: $dest"
}

install_copilot() {
    local skill="$1"
    local src="$SKILLS_DIR/$skill/skill.md"
    local dest=".github/copilot-instructions.md"
    if [ ! -f "$src" ]; then
        echo "  SKIP $skill (skill.md not found)"
        return
    fi
    mkdir -p .github
    local marker="<!-- document-filters-$skill -->"
    if [ -f "$dest" ]; then
        perl -i'' -0pe "s/\Q$marker\E.*?\Q$marker\E\n?//s" "$dest" 2>/dev/null || true
    fi
    printf "\n%s\n" "$marker" >> "$dest"
    cat "$src" >> "$dest"
    printf "\n%s\n" "$marker" >> "$dest"
    echo "  OK   copilot: $dest"
}

echo "Installing Document Filters skills (platform: $PLATFORM)"
while IFS= read -r skill; do
    [[ -z "$skill" ]] && continue
    echo "[$skill]"
    case "$PLATFORM" in
        claude-code) install_claude_code "$skill" ;;
        cursor)      install_cursor "$skill" ;;
        copilot)     install_copilot "$skill" ;;
        all)
            install_claude_code "$skill"
            install_cursor "$skill"
            install_copilot "$skill"
            ;;
        *) echo "Unknown platform: $PLATFORM"; usage ;;
    esac
done < <(get_skills)

echo "Done."
