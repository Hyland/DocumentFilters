# Document Filters: Setup

Install the `df` CLI and download the native runtime binaries. Run this once before using any other Document Filters commands. Document Filters processes 600+ file formats — extracting text, converting to PDF/Markdown/PNG, redacting PII, comparing versions, and more — useful for building RAG pipelines over email archives, converting legacy Office files for web display, or redacting PII before LLM processing.

If you want to convert a Word document to PDF, extract text from a scanned invoice, or black out sensitive names in a contract before sharing, Document Filters handles all of that from the command line.

## Install the CLI

```bash
pip install "git+https://github.com/Hyland/DocumentFilters.git@master#subdirectory=skills/cli"
```

(For reproducible installs, pin to a release tag, e.g. @v25.1.0 instead of @master.)

## Download the runtime

```bash
df init
```

This downloads the platform-specific native binaries (~60 MB) to `~/.document-filters/runtimes/<version>/`. Only downloads once per version; if the runtime for the current version is already cached, df init completes immediately.

## Enable OCR (optional)

OCR (`--ocr` and `--ocr-images`) requires the Tesseract training data file `ISYSreadersocr.dat`. Pass `--ocr` to `df init` to download and install it automatically:

```bash
df init --ocr
```

This downloads `assets.zip` from the same GitHub release as the runtime, extracts `ISYSreadersocr.dat` into the runtime directory, and is safe to re-run (skips download if already installed). Without `ISYSreadersocr.dat`, `--ocr` runs but returns no text (silent failure — no error is raised).

## Verify

```bash
df info
```

Expected output once setup is complete:

```
Document Filters CLI
  SDK version : <current version>
  Platform    : <platform-specific>
  Cache dir   : ~/.document-filters/runtimes
  Runtime     : installed
  License key : set
```

(version number will match the installed package)

- **Runtime: installed** — native binaries are present; `df init` has been run (or auto-downloaded on first use).
- **Runtime: not downloaded (run: df init)** — native binaries are missing; run `df init` to download them.
- **License key: set** — `DF_LICENSE_KEY` is in the environment; full 600+ format access is active.
- **License key: not set (evaluation mode)** — only PDF, Office, ODF, and plain text are available; rendered output has an "Evaluation Copy" watermark.

## Quick examples

```bash
# Turn a Word document into a PDF
df convert-to-pdf report.docx

# Extract all text from a scanned invoice PDF (requires OCR data — run df init --ocr once first)
# Note: TIFF/BMP/PNG/JPEG image formats require a license key; use image-only PDF in eval mode
df init --ocr
df extract-text scanned_invoice.pdf --ocr --output invoice.txt

# Black out all Social Security Numbers in a contract before sharing
df redact contract.pdf '\d{3}-\d{2}-\d{4}' --output contract_redacted.pdf
```

## License

With `DF_LICENSE_KEY` set (as in this environment), all 600+ formats are unlocked — HTML, email (PST, MBOX, EML, MSG), images, CAD, ZIP archives, and more.

Document Filters has two modes:

| Mode | Key required | Formats | Output |
|------|-------------|---------|--------|
| Evaluation | No | PDF, Word, Excel, PowerPoint, ODF, plain text | No watermark on text/Markdown output; rendered PNG/PDF pages may carry a watermark for some formats |
| Licensed (full or trial) | Yes | 600+ formats including HTML, email (PST, MBOX, EML, MSG), CAD, images, ZIP archives | No watermarks |

With a license key active (as in this environment), all 600+ formats work without restriction. The evaluation restriction on images, HTML, and email is lifted.

Trial keys have the same format support as full keys. Request one at DocFiltersEval@hyland.com.

To use a license key:
```bash
export DF_LICENSE_KEY="your-key-here"   # macOS / Linux / Git Bash / WSL
$env:DF_LICENSE_KEY="your-key-here"     # Windows PowerShell
```

Environment variables:
```bash
export DF_VERSION=25.1.0        # pin or override the runtime version
export DF_RUNTIME_PATH=/opt/docfilters/runtime  # directory containing the native .so/.dll files (use a pre-installed runtime, skip download)
export DOCFILTERS_RELEASES_URL=https://... # override release download URL (proxies, air-gap)
```

## What you can do now

Once set up, use these commands (each has a corresponding skill):

| Command | What it does |
|---------|-------------|
| `df extract-text` | Extract plain text from any supported file |
| `df convert-to-markdown` | Convert to Markdown for LLMs and RAG pipelines; `--flavor GPT` (default, LLM-optimised) or `--flavor GFM` (GitHub-compatible); `--locations` embeds `<!-- LOC: page, (l,t,r,b) -->` anchors for chunk attribution; `--pdf-tables` improves table extraction from PDF; `--metadata` prepends a YAML/TOML/JSON front-matter block. |
| `df convert-to-pdf` | Convert any format to PDF; `--bookmarks` generates a PDF bookmark/outline from document headings. |
| `df convert-to-html` | Convert any format to HD HTML5 for web display; `--no-inline-images` writes images as separate files instead of base64-encoding them inline. |
| `df extract-structure` | Extract structure with paragraph bounding boxes (Markdown or JSON); `--words` adds word-level bounding boxes to JSON output — key for fine-grained RAG chunk attribution; `--locations` embeds anchors in Markdown output. |
| `df redact` | Cover (black out) text matching regex patterns, output PDF |
| `df compare` | Word-by-word diff between two document versions — outputs JSON to stdout; use --output-left / --output-right to also write plain PDF renderings of each input document (diff markup is in the JSON only). Always redirect stdout to a file for large documents to avoid flooding agent context. |
| `df convert-to-png` | Render each page as a PNG image; `--dpi <n>` sets resolution (default 96), `--output-prefix` sets the filename stem. |
| `df extract-metadata` | Show file type name, page count, and text/subfile support flags; `--json` for machine-readable output. |
