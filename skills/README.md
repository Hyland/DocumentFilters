# Document Filters Skills

AI agent skills for [Hyland Document Filters](https://www.documentfilters.com) — extract, convert, and process 600+ document formats from the command line or any AI coding agent.

Each skill is a Markdown file that teaches an AI agent how to use the `df` CLI. Skills work with any AI coding agent: Claude Code, Cursor, GitHub Copilot, or any tool that accepts a system prompt or custom instructions.

## What you can do

| Command | What it does | Eval (no key) | Licensed |
|---------|-------------|:---:|:---:|
| `df extract-text` | Extract raw UTF-8 text from any document | PDF, Office, ODF, TXT | 600+ formats |
| `df convert-to-markdown` | Convert to LLM-ready Markdown with headings, tables, LOC anchors | PDF, Office, ODF, TXT | 600+ formats |
| `df convert-to-pdf` | Convert any document to PDF | PDF, Office, ODF, TXT | 600+ formats |
| `df convert-to-html` | Convert any document to HTML5 | — | 600+ formats |
| `df convert-to-png` | Render each page as a PNG (vision models, thumbnails) | PDF, Office, ODF, TXT | 600+ formats |
| `df extract-metadata` | Inspect file type, page count, capability flags | PDF, Office, ODF, TXT | 600+ formats |
| `df extract-structure` | Extract structure as Markdown or JSON (with bounding boxes) | Markdown: PDF, Office, ODF, TXT | 600+ formats |
| `df redact` | Black out text matching regex patterns, output burned-in PDF | PDF, Office, ODF, TXT | 600+ formats |
| `df compare` | Word-by-word diff between two document versions (JSON output) | PDF, Office, ODF, TXT | 600+ formats |
| `df init` | Download the native runtime (and optionally OCR data) | — | — |
| `df info` | Show version, platform, runtime status, and license mode | — | — |

**Eval mode** (no license key): PDF, Word, Excel, PowerPoint, ODF, and plain text work fully. HTML, email (EML, MSG, PST, MBOX), CAD, and raw image formats (TIFF, BMP, PNG, JPEG) require a license key and return error 4015 without one. No watermarks on text or Markdown output in eval mode. Request a free trial key: [DocFiltersEval@hyland.com](mailto:DocFiltersEval@hyland.com)

## Quick start (5 minutes)

### 1. Install the `df` CLI

```bash
pip install "git+https://github.com/Hyland/DocumentFilters.git@master#subdirectory=skills/cli"
```

For reproducible installs, pin to a release tag:
```bash
pip install "git+https://github.com/Hyland/DocumentFilters.git@v26.2.0#subdirectory=skills/cli"
```

### 2. Download the runtime

```bash
df init
```

Downloads platform-specific native binaries (~60 MB) to `~/.document-filters/runtimes/<version>/`. Only runs once — already-cached runtimes are detected and skipped.

### 3. (Optional) Enable OCR

```bash
df init --ocr
```

Downloads `ISYSreadersocr.dat` (Tesseract training data) needed for `--ocr` and `--ocr-images`. Re-run safe — skips if already installed.

### 4. (Optional) Set a license key

```bash
export DF_LICENSE_KEY="your-key-here"        # macOS / Linux / Git Bash / WSL
$env:DF_LICENSE_KEY="your-key-here"          # Windows PowerShell
```

Unlocks HTML, email (EML, MSG, PST, MBOX), CAD, raw images, and all other 600+ formats. Without a key, PDF, Office, ODF, and plain text work in evaluation mode.

### 5. Verify

```bash
df info
```

Expected output:
```
Document Filters CLI
  SDK version : 26.2.0
  Platform    : <your platform>
  Cache dir   : ~/.document-filters/runtimes
  Runtime     : installed
  License key : set          ← or "not set (evaluation mode)"
```

## Common recipes

```bash
# Turn a Word document into a PDF
df convert-to-pdf report.docx --bookmarks

# Extract text from a PDF for AI search/RAG
df convert-to-markdown report.pdf --flavor GPT --pdf-tables --no-headers --no-footers --normalize --locations --output report.md

# Black out all Social Security Numbers before sharing
df redact contract.pdf '\b\d{3}-\d{2}-\d{4}\b' --output contract_redacted.pdf

# Compare two versions of a contract
df compare contract_v1.docx contract_v2.docx > diff.json

# Render a document as page images for a vision model
df convert-to-png report.pdf --dpi 150 --output-prefix renders/page

# Extract text from a scanned invoice (needs df init --ocr first)
df extract-text invoice.pdf --ocr --output invoice.txt

# Extract email body and metadata
df convert-to-markdown message.eml --metadata --output message.md   # requires license key

# Get word-level bounding boxes for RAG source attribution
df extract-structure report.pdf --format json --words --output report.json
```

## Skills

| Skill | Description |
|-------|-------------|
| [df-setup](./df-setup.md) | Install the CLI, download the runtime, enable OCR |
| [df-extract-metadata](./df-extract-metadata.md) | Inspect file type, page count, and capability flags |
| [df-extract-text](./df-extract-text.md) | Extract plain text for AI ingestion, search, and RAG |
| [df-to-markdown](./df-to-markdown.md) | Convert documents to LLM-ready Markdown |
| [df-to-pdf](./df-to-pdf.md) | Convert any document to PDF |
| [df-to-png](./df-to-png.md) | Render pages as PNG images for vision models or thumbnails |
| [df-redact](./df-redact.md) | Redact PII and sensitive content |
| [df-compare](./df-compare.md) | Diff two document versions word-by-word |
| [df-extract-structure](./df-extract-structure.md) | Extract document structure as Markdown or JSON with bounding boxes |
| [df-rag-prep](./df-rag-prep.md) | Batch-process a folder of mixed documents into a RAG corpus |

## Install skills into your agent

### Claude Code — global (available in every project)

```bash
# macOS / Linux / Git Bash
bash skills/install.sh

# Windows PowerShell
.\skills\install.ps1
```

Skills are copied to `~/.claude/commands/` and available as slash commands: `/df-setup`, `/df-extract-text`, `/df-to-markdown`, etc.

### Claude Code — project-local

```bash
bash skills/install.sh --agent claude-project
.\skills\install.ps1 -Agent claude-project
```

### Cursor

```bash
bash skills/install.sh --agent cursor
.\skills\install.ps1 -Agent cursor
```

Appends skills to `.cursorrules` in the current directory. Idempotent.

### GitHub Copilot

```bash
bash skills/install.sh --agent copilot
.\skills\install.ps1 -Agent copilot
```

Appends skills to `.github/copilot-instructions.md`. Idempotent.

### Any other agent

Paste the contents of any `skills/df-*.md` file into your agent's system prompt or custom instructions field.

### Install a subset

```bash
bash skills/install.sh --skills df-extract-text,df-to-markdown
.\skills\install.ps1 -Skills df-extract-text,df-to-markdown
```

### Manual one-liner

```bash
# Claude Code global (bash)
cp skills/df-*.md ~/.claude/commands/

# Claude Code global (PowerShell)
Copy-Item skills\df-*.md "$HOME\.claude\commands\"
```

## Platform support

| Platform | Binary |
|----------|--------|
| Windows x64 | `windows-intel-msvc-64` |
| macOS Apple Silicon | `macos-arm64-clang-64` |
| macOS Intel | `macos-intel-clang-64` |
| Linux x64 (glibc) | `linux-intel-gcc-64` |
| Linux x64 (musl/Alpine) | `linux-intel-clang-musl-64` |
| Linux ARM64 | `linux-aarch64-gcc-64` |

## Environment variables

| Variable | Description |
|----------|-------------|
| `DF_LICENSE_KEY` | License key — unlocks all 600+ formats |
| `DF_VERSION` | Pin or override the runtime version (default: matches installed CLI) |
| `DF_RUNTIME_PATH` | Use a pre-installed runtime directory (skips download) |
| `DOCFILTERS_RELEASES_URL` | Override the release download URL (for proxies or air-gapped environments) |

## License

Hyland Document Filters. For evaluation keys: [DocFiltersEval@hyland.com](mailto:DocFiltersEval@hyland.com)
