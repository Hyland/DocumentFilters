# Document Filters: Extract Plain Text

Extract raw UTF-8 text from any document — PDF, Word, PowerPoint, Excel, HTML, email archives, and ZIP containers — across 600+ supported formats. Produces plain text only: no headings, tables, markup, or layout. For HTML input, all tags are stripped and only the visible text content is written to the output file. Use this when you need just the words (search indexing, RAG ingestion, copy-pasting into a summary).

Based on: `skills/cli/src/document_filters_cli/main.py` (`cmd_extract_text`)

> **Agent note**: Always pass `--output` to avoid dumping document text into context. Report only path and character count.

## Prerequisites

`df` CLI must be installed and the runtime downloaded. If `df info` fails, install first:
```bash
pip install "git+https://github.com/Hyland/DocumentFilters.git@master#subdirectory=skills/cli"
df init
```

Run `df info` before processing. In evaluation mode (no key), PDF/Office/TXT extraction works normally — text output contains no watermark text. HTML, email (EML/MSG/PST), and CAD formats return error 4015 without a license key.

## Usage

```bash
# Most common case — extract text from a Word document
df extract-text report.docx --output report.txt

# Pull all slide text from a PowerPoint deck
df extract-text slides.pptx --output slides.txt

# Extract text from a contract to paste into a summary email
df extract-text contract.pdf --output contract.txt

# Extract visible text from an HTML file (all tags are stripped)
df extract-text sample.html --output html_text.txt

# Scanned invoice — OCR required (image-only PDF has no selectable text)
df extract-text invoice.pdf --ocr --output invoice.txt

# Word doc with embedded scanned figures — use convert-to-markdown for this
# (--ocr-images only works in render mode, which convert-to-markdown uses)
df convert-to-markdown report.docx --ocr-images --output report.md

# Recurse into an email archive and extract all attachment text
df extract-text inbox.pst --subfiles --output inbox.txt

# Scanned PDF in an email archive
df extract-text inbox.pst --ocr --subfiles --output inbox.txt

# Read the text from a received email (.eml or .msg file)
df extract-text meeting-notes.eml --output notes.txt

# Extract text from all documents inside a ZIP archive
df extract-text documents.zip --subfiles --output documents.txt

# Get the plain text from a spreadsheet (tables become tab-separated text)
df extract-text budget.xlsx --output budget.txt

# Batch (shell loop)
# On Linux/macOS:
find ./docs -type f | while read f; do df extract-text "$f" --output "${f}.txt" 2>/dev/null; done
# On Windows (PowerShell):
# Get-ChildItem -Recurse ./docs -File | ForEach-Object { df extract-text $_.FullName --output ($_.FullName + ".txt") 2>$null }
```

## Flags

| Flag | Description |
|------|-------------|
| `--output / -o` | Write to file instead of stdout. **Always use this.** |
| `--ocr` | Enable OCR for fully scanned or image-only documents. Without this flag, scanned files return only file metadata — no text. Expect 1–5 seconds per page. Requires `ISYSreadersocr.dat` — install once with `df init --ocr`. **Note:** image formats (TIFF, BMP, PNG, JPEG) require a license key — in evaluation mode, use an image-only PDF instead (PDF is available in eval). |
| `--ocr-language LANG` | 3-letter ISO 639-2 language code for OCR (e.g. `fra` for French, `deu` for German, `spa` for Spanish, `jpn` for Japanese). Default: `eng`. Only needed when the document is not English. |
| `--ocr-min-width N` | Minimum pixel width for images to attempt OCR (default: `1000`). Lower if your scanned pages are narrow. |
| `--subfiles` | Recurse into ZIP archives, email containers (PST, MBOX, EML, MSG), and embedded attachments. Without this flag, only the top-level document body is extracted — attachments inside a PST or MBOX are silently ignored. |
| `--slide-notes` | Include PowerPoint speaker notes in the extracted text. Without this flag, notes are silently omitted. Useful when slides are used as knowledge bases (speaker notes often contain more detail than slide text). |

Without `--output`, text is written to stdout (directly into the agent context). With `--subfiles`, a PST or MBOX archive may contain thousands of messages. Even a single large PDF or spreadsheet can produce hundreds of thousands of characters. Always pass `--output`; never omit it.

Control characters (null bytes, form feeds, etc.) are always stripped from output. This is not configurable.

## When to use extract-text vs convert-to-markdown

In plain terms: use extract-text when you just need the raw words. Use convert-to-markdown when structure matters (headings, tables, lists).

For AI pipelines: `df convert-to-markdown --flavor GPT` preserves headings, tables, and lists which improve how accurately an AI can find and cite the document.

Use `df extract-text` for:
- **Scanned documents** (`--ocr`) — the only path to text from image-only files. Without `--ocr`, scanned files return only file metadata — no text. In evaluation mode (no license), use image-only PDFs for OCR; raw image formats (TIFF, BMP, PNG, JPEG) are not available in eval.
- **Archive/email containers** (`--subfiles`) — recurse into PST, MBOX, ZIP
- **Minimum output size** — when structure is irrelevant and token count (how much this costs to process with an LLM) matters
- **Search indexing / embeddings** — when the downstream system (Elasticsearch, vector store ingest, OpenAI embeddings endpoint) requires plain text and cannot handle Markdown syntax characters

For documents that have readable text but also contain **embedded images with text** (scanned figures, diagrams, photos with labels), use `df convert-to-markdown --ocr-images` — inline image OCR only works in render mode, which `convert-to-markdown` uses.

## RAG pipeline notes

### For AI/developer pipelines

`extract-text` output is clean UTF-8 with control characters stripped — feed it directly to your chunker. For structure-aware chunking (headings as chunk boundaries) use `convert-to-markdown --flavor GPT` instead.

**Important:** `extract-text` output contains no page markers — you cannot determine which chunk came from which page. For chunk attribution, use:
- `df extract-structure --format json --words` — returns word-level bounding boxes per page as JSON. Bounding boxes describe where on the page each word appears (left, top, right, bottom coordinates) — useful for highlighting search hits in a PDF viewer or attributing an AI answer back to an exact page location.
- `df convert-to-markdown --locations` — embeds `<!-- LOC: page, (l,t,r,b) -->` location comments that survive most Markdown chunkers

## License

With `DF_LICENSE_KEY` set, all 600+ formats are unlocked. In evaluation mode (no key), PDF, Office, ODF, and plain text work with clean text output — no watermark is injected into text/plain-text output. HTML, email (EML, MSG, PST, MBOX), and CAD formats return error 4015 without a license key and produce no output. Run `df info` to confirm the key is loaded.
