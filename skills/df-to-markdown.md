# Document Filters: Convert to Markdown

Convert documents to Markdown using the Document Filters CLI. Supports 600+ formats including PDF, Word, PowerPoint, Excel, HTML, email (EML, MSG), CAD, and more. With `DF_LICENSE_KEY` set, all formats are unlocked. Produces structured Markdown with optional headings, tables, and metadata.

Based on: `skills/cli/src/document_filters_cli/main.py` (`cmd_convert_to_markdown`)

> **Agent note**: Output defaults to `<input>.md` alongside the source file. Use `--output` to specify a different path; report the output path printed by the CLI. Do not read the output file back into context.

## Prerequisites

`df` CLI must be installed and the runtime downloaded. If `df info` fails, install first:
```bash
pip install "git+https://github.com/Hyland/DocumentFilters.git@master#subdirectory=skills/cli"
df init
```

## License and format support

With `DF_LICENSE_KEY` set, all 600+ formats are available — PDF, Office, ODF, HTML, email (EML, MSG, PST, OST, MBOX), CAD, archives, and more. Email (EML, MSG, PST, MBOX), HTML, CAD, and medical imaging formats are all available in the current environment. Without a key, evaluation mode limits output to PDF, Office, ODF, and plain text. For those formats, text and Markdown output is clean with no watermark text injected. HTML, email, and CAD formats require a license and return error 4015 in eval mode.

```bash
export DF_LICENSE_KEY="your-key-here"
```

## Common use cases

- Convert a Word contract or PDF report to Markdown for easy editing or version control
- Extract all text from a stack of PDF reports or scanned contracts so you can search, copy, or paste the content — no special software needed to read the result
- Archive email or HTML newsletters as searchable plain-text Markdown
- Turn a PDF or Word document into plain text you can paste into an email, edit in any text editor, or upload to a chat tool like ChatGPT or Claude

## Usage

```bash
# Convert a PDF report to Markdown (output written to report.md)
df convert-to-markdown report.pdf --output report.md

# GPT flavor (default) — for DOCX, PDF, PPTX and all structured formats this produces
# standard unescaped # headings and pipe tables. Identical to GFM for these formats.
df convert-to-markdown report.pdf --flavor GPT --output report.md

# GFM flavor — identical to GPT for all structured formats
df convert-to-markdown report.pdf --flavor GFM --output report.md

# NOTE: For plain .txt input ONLY, both GPT and GFM escape # as \# regardless of flavor,
# because plain text has no heading semantics. Output will contain \# headings.
# To get proper # headings, use a format that carries heading structure (.docx, .pdf).
df convert-to-markdown sample.txt --output txt.md

# Include file metadata (title, author, page count) at the top
df convert-to-markdown slides.pptx --flavor GPT --metadata --output slides.md

# Enable PDF layout analysis (tables, lists, multi-column layout)
df convert-to-markdown report.pdf --pdf-tables --output report.md

# Convert an HTML file
df convert-to-markdown page.html --output page.md

# Embed bounding-box location comments (useful for RAG source mapping)
df convert-to-markdown report.pdf --flavor GPT --locations --output report_loc.md

# RAG-optimised: strip headers/footers, normalize punctuation, add source attribution
df convert-to-markdown report.pdf --flavor GPT --pdf-tables --no-headers --no-footers --normalize --locations --output report_rag.md

# Also OCR any embedded images (scanned figures, diagrams with labels)
df convert-to-markdown report.pdf --flavor GPT --pdf-tables --ocr-images --no-headers --no-footers --normalize --locations --output report_rag.md
```

## Flags

| Flag | Description |
|------|-------------|
| `--output / -o` | Write to a specific file path. |
| `--flavor` | Markdown flavor: `GPT` (default, LLM-optimised) or `GFM` (GitHub-Flavored Markdown). |
| `--pdf-tables` | Enable PDF layout analysis: detects tables (rendered as Markdown tables), lists, and multi-column layout. Recommended for structured reports and forms. Accepted without error on non-PDF formats (HTML, Word, etc.) but has no effect on those formats — layout analysis only applies to PDF input. Word (`.docx`) tables are extracted natively as Markdown pipe tables. |
| `--metadata` | Prepend a document metadata block (title, author, dates, etc.). **Note:** HTML files carry no embedded document properties (no title, author, or date fields in the file stream), so the metadata block is silently omitted for `.html` input even when this flag is set. Formats such as PDF, DOCX, and PPTX reliably populate the metadata block. |
| `--metadata-format` | Format for the metadata block: `YAML` (default), `TOML`, `JSON`, or `COMMENTS`. Has no effect unless `--metadata` is also passed. Each format uses a different delimiter style: YAML emits `---\ntitle: …\n---`, TOML emits `+++\ntitle = …\n+++`, JSON emits `{"title": …}`, and COMMENTS emits `<!-- title: … -->`. |
| `--locations` | Embed `<!-- LOC: page, (l,t,r,b) -->` bounding-box comments before each content block. See [Location comments](#location-comments). |
| `--ocr-images` | OCR images embedded within otherwise-text documents — scanned figures, photos with captions, diagrams with labels inside a Word doc or PDF. Extracted text is merged inline into the Markdown output after the image. The SDK default `OCR_MIN_WIDTH` of 1000px is automatically lowered to 100px when this flag is set, because Office documents render embedded images at screen DPI (~96), making typical images 400–600px wide. Requires `ISYSreadersocr.dat` — install once with `df init --ocr`. Expect 1–3 seconds per image. |
| `--ocr-language LANG` | 3-letter ISO 639-2 language code for OCR (e.g. `fra`, `deu`, `spa`, `jpn`). Default: `eng`. Only needed when image text is non-English. |
| `--ocr-min-width N` | Override the minimum pixel width for images to be OCR'd. When `--ocr-images` is set the default is already `100` (lowered from the SDK default of `1000`). Raise this (e.g. `--ocr-min-width 500`) to skip smaller decorative graphics. |
| `--no-headers` | Exclude page headers from output. Note: the SDK default already excludes headers — this flag makes the intent explicit and guards against future default changes. Still recommended in RAG pipelines to document intent. |
| `--no-footers` | Exclude page footers from output. Same note as `--no-headers` — footers are excluded by default but the flag is recommended for RAG pipelines to make intent clear. |
| `--normalize` | Normalize smart quotes (`"` `"` `'` `'`) to straight ASCII (`"` `'`) and em/en-dashes (`—` `–`) to hyphens. Use when downstream embedding models or tokenizers handle curly quotes inconsistently. |

Without `--output`, the CLI writes to `<input>.md` in the same directory as the input file. Pass `--output <path>` to control the destination.

## Batch processing

```bash
# Convert every PDF in a directory (output goes to <name>.md alongside each file)
for f in docs/*.pdf; do df convert-to-markdown "$f" --flavor GPT --locations; done

# With explicit output directory
for f in docs/*.pdf; do df convert-to-markdown "$f" --output "out/$(basename "$f" .pdf).md" --flavor GPT --locations; done
```

## Flavors

The `--flavor` flag (default: `GPT`) controls Markdown output style. In practice, for structured document formats (DOCX, PDF, PPTX), both GPT and GFM produce identical output — standard unescaped `#` headings, pipe tables, and bold/italic text. The flavor difference only manifests for **plain-text `.txt` input**:

- **`GPT`** (default) — For `.txt` files: backslash-escapes `#` characters (e.g., `\# Heading`) because plain text has no heading semantics. For all other formats (DOCX, PDF, HTML, etc.): identical to GFM.
- **`GFM`** — Standard GitHub-Flavored Markdown. For `.txt` files: also backslash-escapes `#` (same as GPT — the format carries no heading information regardless of flavor). For all other formats: identical to GPT.

**Practical guidance**: Use `--flavor GPT` as the default (it's the CLI default). The flavor choice only matters if you're converting `.txt` files and need heading behavior documented above.

Example output for DOCX or PDF source (both flavors produce this):
```
# Document Title

## Section Heading

Body text here...
```

Example output for `.txt` source (both flavors produce this):
```
\# Document Title

\#\# Section Heading

Body text here...
```

> **HTML sources**: When converting `.html` files, heading elements (`<h1>`, `<h2>`, etc.) are rendered as `**bold**` text rather than `#` heading markers in both GPT and GFM flavors. HTML tables are not extracted as Markdown pipe tables — each cell appears as a separate bold/plain text line. The `--metadata` flag is silently accepted on `.html` input but produces no block — HTML files carry no embedded document properties. The `--pdf-tables` flag is also silently accepted on `.html` input but has no effect.

> **Plain-text sources**: When converting `.txt` files, both `GPT` and `GFM` flavors backslash-escape `#` characters (e.g., `\# Heading`). Because plain text has no heading semantics, the converter cannot distinguish an intentional Markdown heading from a literal `#` character and escapes them defensively. If the `.txt` file contains Markdown syntax and you want headings rendered as-is, use a format that carries heading structure (e.g., `.docx`, `.pdf`) or rename to `.md`.

## Location comments

When `--locations` is passed, the CLI embeds HTML comments immediately before each content block:

```
<!-- LOC: page, (l,t,r,b) -->
```

- `page` — 1-based page number
- `l,t,r,b` — left, top, right, bottom bounding-box coordinates in document units

Example output with `--flavor GPT --locations` on a DOCX source:

```markdown
<!-- LOC: 1, (120,96,696,145) -->
# AI Engineer World Fair Demo

<!-- LOC: 1, (120,171,696,194) -->
## Introduction
```

These comments are invisible in rendered Markdown. They are useful for:
- **RAG source attribution** — map a retrieved chunk back to its exact page and bounding box
- **Document highlighting** — drive UI overlays that highlight the source region in the original document

## When to use convert-to-markdown vs extract-text

For structured documents (PDF, Word, PPTX), prefer `df convert-to-markdown --flavor GPT` — it preserves headings, tables, and lists which improve RAG retrieval quality.

Email messages (EML, MSG) and HTML pages also work with `convert-to-markdown` when `DF_LICENSE_KEY` is set, producing structured Markdown with metadata. For a single email: `df convert-to-markdown message.eml --output message.md`. For an Outlook archive with thousands of messages, use `df extract-text archive.pst --subfiles --output archive.txt` instead.

Use `df extract-text` for:
- **Scanned documents** (`--ocr`) — the only path to text from image-only PDFs
- **Archive/email containers** (`--subfiles`) — recurse into PST, MBOX, ZIP
- **Minimum output size** — when structure is irrelevant and token count matters
- **Precise word-level bounding boxes for production RAG** — use `df extract-structure --format json --words` which emits a structured JSON with per-word coordinates, easier to parse programmatically than LOC HTML comments.