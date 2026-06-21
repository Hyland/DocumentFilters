[CmdletBinding()]
param(
    [ValidateSet('claude-code', 'cursor', 'copilot', 'all')]
    [string]$Platform = 'all',
    [string]$Skills = ''
)

$SkillsDir = $PSScriptRoot

function Get-SkillList {
    if ($Skills) {
        return ($Skills -split ',') | ForEach-Object { $_.Trim() } | Where-Object { $_ -ne '' }
    }
    Get-ChildItem -Path $SkillsDir -Directory |
        Where-Object { Test-Path (Join-Path $_.FullName 'skill.md') } |
        Select-Object -ExpandProperty Name
}

function Install-ClaudeCode($skill) {
    $src = Join-Path $SkillsDir $skill
    $dest = Join-Path $HOME ".claude\plugins\document-filters-$skill"
    if (-not (Test-Path (Join-Path $src 'skill.md'))) {
        Write-Host "  SKIP $skill (skill.md not found)"
        return
    }
    New-Item -ItemType Directory -Force -Path $dest | Out-Null
    Copy-Item "$src\*" $dest -Recurse -Force
    Write-Host "  OK   claude-code: $dest"
}

function Install-Cursor($skill) {
    $src = Join-Path $SkillsDir "$skill\skill.md"
    $dest = Join-Path (Get-Location) '.cursorrules'
    if (-not (Test-Path $src)) {
        Write-Host "  SKIP $skill (skill.md not found)"
        return
    }
    $marker = "<!-- document-filters-$skill -->"
    $content = if (Test-Path $dest) { Get-Content $dest -Raw } else { '' }
    # Remove existing block
    $content = [regex]::Replace($content, '\n?' + [regex]::Escape($marker) + '[\s\S]*?' + [regex]::Escape($marker) + '\r?\n?', '')
    $skillContent = Get-Content $src -Raw
    $content += "`n$marker`n$skillContent`n$marker`n"
    Set-Content $dest $content -NoNewline
    Write-Host "  OK   cursor: $dest"
}

function Install-Copilot($skill) {
    $src = Join-Path $SkillsDir "$skill\skill.md"
    $githubDir = Join-Path (Get-Location) '.github'
    $dest = Join-Path $githubDir 'copilot-instructions.md'
    if (-not (Test-Path $src)) {
        Write-Host "  SKIP $skill (skill.md not found)"
        return
    }
    New-Item -ItemType Directory -Force -Path $githubDir | Out-Null
    $marker = "<!-- document-filters-$skill -->"
    $content = if (Test-Path $dest) { Get-Content $dest -Raw } else { '' }
    $content = [regex]::Replace($content, '\n?' + [regex]::Escape($marker) + '[\s\S]*?' + [regex]::Escape($marker) + '\r?\n?', '')
    $skillContent = Get-Content $src -Raw
    $content += "`n$marker`n$skillContent`n$marker`n"
    Set-Content $dest $content -NoNewline
    Write-Host "  OK   copilot: $dest"
}

Write-Host "Installing Document Filters skills (platform: $Platform)"
foreach ($skill in (Get-SkillList)) {
    if (-not $skill) { continue }
    Write-Host "[$skill]"
    switch ($Platform) {
        'claude-code' { Install-ClaudeCode $skill }
        'cursor'      { Install-Cursor $skill }
        'copilot'     { Install-Copilot $skill }
        'all' {
            Install-ClaudeCode $skill
            Install-Cursor $skill
            Install-Copilot $skill
        }
    }
}
Write-Host "Done."
