#!/usr/bin/env bash
# Install Document Filters skills for your AI agent.
#
# Usage:
#   bash skills/install.sh                          # Claude Code (global)
#   bash skills/install.sh --agent cursor           # Cursor (.cursorrules)
#   bash skills/install.sh --agent copilot          # GitHub Copilot (.github/copilot-instructions.md)
#   bash skills/install.sh --agent claude-project   # Claude Code (project-local .claude/commands/)
#   bash skills/install.sh --skills df-extract-text,df-to-markdown  # subset only

set -euo pipefail

SKILLS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
AGENT="claude"
SELECTED=""

while [[ $# -gt 0 ]]; do
    case $1 in
        --agent)   AGENT="$2"; shift 2 ;;
        --skills)  SELECTED="$2"; shift 2 ;;
        -h|--help)
            sed -n '2,8p' "$0" | sed 's/^# //'
            exit 0 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

skill_files() {
    if [[ -n "$SELECTED" ]]; then
        echo "$SELECTED" | tr ',' '\n' | while read -r name; do
            name="${name%.md}"
            echo "$SKILLS_DIR/$name.md"
        done
    else
        ls "$SKILLS_DIR"/*.md 2>/dev/null
    fi
}

install_claude() {
    local dest="$HOME/.claude/commands"
    mkdir -p "$dest"
    while IFS= read -r src; do
        [[ -f "$src" ]] || continue
        cp "$src" "$dest/$(basename "$src")"
        echo "  OK  $dest/$(basename "$src")"
    done < <(skill_files)
}

install_claude_project() {
    local dest=".claude/commands"
    mkdir -p "$dest"
    while IFS= read -r src; do
        [[ -f "$src" ]] || continue
        cp "$src" "$dest/$(basename "$src")"
        echo "  OK  $dest/$(basename "$src")"
    done < <(skill_files)
}

install_cursor() {
    local dest=".cursorrules"
    while IFS= read -r src; do
        [[ -f "$src" ]] || continue
        local name
        name="$(basename "$src" .md)"
        local marker="<!-- document-filters-$name -->"
        [[ -f "$dest" ]] && perl -i'' -0pe "s/\Q$marker\E.*?\Q$marker\E\n?//s" "$dest" 2>/dev/null || true
        printf "\n%s\n" "$marker" >> "$dest"
        cat "$src" >> "$dest"
        printf "%s\n" "$marker" >> "$dest"
        echo "  OK  $dest [$name]"
    done < <(skill_files)
}

install_copilot() {
    local dest=".github/copilot-instructions.md"
    mkdir -p .github
    while IFS= read -r src; do
        [[ -f "$src" ]] || continue
        local name
        name="$(basename "$src" .md)"
        local marker="<!-- document-filters-$name -->"
        [[ -f "$dest" ]] && perl -i'' -0pe "s/\Q$marker\E.*?\Q$marker\E\n?//s" "$dest" 2>/dev/null || true
        printf "\n%s\n" "$marker" >> "$dest"
        cat "$src" >> "$dest"
        printf "%s\n" "$marker" >> "$dest"
        echo "  OK  $dest [$name]"
    done < <(skill_files)
}

echo "Installing Document Filters skills (agent: $AGENT)"
case "$AGENT" in
    claude)          install_claude ;;
    claude-project)  install_claude_project ;;
    cursor)          install_cursor ;;
    copilot)         install_copilot ;;
    *) echo "Unknown agent: $AGENT. Use: claude, claude-project, cursor, copilot"; exit 1 ;;
esac
echo "Done."
