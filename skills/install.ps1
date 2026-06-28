# Install Document Filters skills for your AI agent.
#
# Usage:
#   .\skills\install.ps1                                    # Claude Code (global)
#   .\skills\install.ps1 -Agent cursor                     # Cursor (.cursorrules)
#   .\skills\install.ps1 -Agent copilot                    # GitHub Copilot (.github/copilot-instructions.md)
#   .\skills\install.ps1 -Agent claude-project             # Claude Code (project-local .claude/commands/)
#   .\skills\install.ps1 -Skills df-extract-text,df-to-markdown  # subset only

param(
    [ValidateSet('claude','claude-project','cursor','copilot')]
    [string]$Agent = 'claude',
    [string]$Skills = ''
)

$SkillsDir = $PSScriptRoot

function Get-SkillFiles {
    if ($Skills) {
        ($Skills -split ',') | ForEach-Object {
            $name = $_.Trim().TrimEnd('.md')
            Join-Path $SkillsDir "$name.md"
        }
    } else {
        Get-ChildItem -Path $SkillsDir -Filter '*.md' | Select-Object -ExpandProperty FullName
    }
}

function Install-Claude {
    $dest = Join-Path $HOME '.claude\commands'
    New-Item -ItemType Directory -Force -Path $dest | Out-Null
    Get-SkillFiles | Where-Object { Test-Path $_ } | ForEach-Object {
        $target = Join-Path $dest (Split-Path $_ -Leaf)
        Copy-Item $_ $target -Force
        Write-Host "  OK  $target"
    }
}

function Install-ClaudeProject {
    $dest = '.claude\commands'
    New-Item -ItemType Directory -Force -Path $dest | Out-Null
    Get-SkillFiles | Where-Object { Test-Path $_ } | ForEach-Object {
        $target = Join-Path $dest (Split-Path $_ -Leaf)
        Copy-Item $_ $target -Force
        Write-Host "  OK  $target"
    }
}

function Install-Cursor {
    $dest = '.cursorrules'
    Get-SkillFiles | Where-Object { Test-Path $_ } | ForEach-Object {
        $name = [System.IO.Path]::GetFileNameWithoutExtension($_)
        $marker = "<!-- document-filters-$name -->"
        $existing = if (Test-Path $dest) { Get-Content $dest -Raw } else { '' }
        $existing = [regex]::Replace($existing, "(?s)\n?\Q$marker\E.*?\Q$marker\E\r?\n?", '')
        $skillContent = Get-Content $_ -Raw
        $existing += "`n$marker`n$skillContent`n$marker`n"
        Set-Content $dest $existing -NoNewline
        Write-Host "  OK  $dest [$name]"
    }
}

function Install-Copilot {
    $githubDir = '.github'
    $dest = Join-Path $githubDir 'copilot-instructions.md'
    New-Item -ItemType Directory -Force -Path $githubDir | Out-Null
    Get-SkillFiles | Where-Object { Test-Path $_ } | ForEach-Object {
        $name = [System.IO.Path]::GetFileNameWithoutExtension($_)
        $marker = "<!-- document-filters-$name -->"
        $existing = if (Test-Path $dest) { Get-Content $dest -Raw } else { '' }
        $existing = [regex]::Replace($existing, "(?s)\n?\Q$marker\E.*?\Q$marker\E\r?\n?", '')
        $skillContent = Get-Content $_ -Raw
        $existing += "`n$marker`n$skillContent`n$marker`n"
        Set-Content $dest $existing -NoNewline
        Write-Host "  OK  $dest [$name]"
    }
}

Write-Host "Installing Document Filters skills (agent: $Agent)"
switch ($Agent) {
    'claude'         { Install-Claude }
    'claude-project' { Install-ClaudeProject }
    'cursor'         { Install-Cursor }
    'copilot'        { Install-Copilot }
}
Write-Host "Done."
