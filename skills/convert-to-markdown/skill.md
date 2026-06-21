---
name: convert-to-markdown
description: Convert any document to Markdown using Document Filters
---

# Convert a Document to Markdown

Hyland Document Filters converts 600+ file formats — Office, PDF, CAD, archives, email — to Markdown locally, with no conversion servers or cloud calls. Markdown output preserves headings, tables, images, and document structure, making it the recommended ingestion format for LLM pipelines and RAG systems.

## Setup

```bash
pip install document-filters
```

Native runtime binaries (~60 MB) are downloaded automatically on first use. Set `DF_LICENSE_KEY` in your environment for full features — omitting it runs in evaluation mode (watermarked output, limited pages). Supports 600+ input formats including Word, Excel, PowerPoint, PDF, HTML, CAD, ZIP, PST, MBOX, EML, and more.

## Python

```python
import io
import re
import glob
import logging
import os
from DocumentFilters import *

api = DocumentFilters()
api.Initialize(os.environ.get("DF_LICENSE_KEY", ""), ".")

def convert_to_markdown(input_path, output_path, doc_opts="", canvas_opts=""):
    with api.GetExtractor(input_path) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, doc_opts)
        with api.MakeOutputCanvas(output_path, IGR_DEVICE_MARKDOWN, canvas_opts) as canvas:
            for page in doc.Pages:
                with page:
                    canvas.RenderPage(page)

# Basic conversion
convert_to_markdown("report.docx", "report.md")

# Optimised for LLM ingestion (GPT flavor, metadata as YAML, unwrap PDF text)
convert_to_markdown(
    "report.pdf", "report.md",
    doc_opts="PDF_LAYOUT_DETECTION=on;PDF_TABLE_DETECTION=on;PDF_LIST_DETECTION=on",
    canvas_opts="MARKDOWN_FLAVOR=GPT;MARKDOWN_METADATA_FORMAT=YAML;MARKDOWN_INCLUDE_METADATA=on;MARKDOWN_INCLUDE_FOOTNOTES=on;PDF_MARKDOWN_UNWRAP_TEXT=on"
)

# Full fidelity (images, charts, formatting)
convert_to_markdown(
    "report.pptx", "report.md",
    canvas_opts="MARKDOWN_INCLUDE_IMAGES=on;MARKDOWN_INCLUDE_CHARTS=on;MARKDOWN_INCLUDE_CHART_TABLE=on;MARKDOWN_INCLUDE_FORMATTING=on"
)

# In-memory output — MakeOutputCanvas accepts any Python file-like object, no file written to disk
buf = io.BytesIO()
convert_to_markdown("report.pdf", buf)
markdown_text = buf.getvalue().decode("utf-8")

# RAG grounding — inject location comments for citation/source-linking
def convert_to_markdown_with_locations(input_path, output_path):
    # PDF_LAYOUT_DETECTION and PDF_TABLE_DETECTION are ignored for non-PDF formats,
    # so this helper is safe to call on any supported input type.
    convert_to_markdown(
        input_path, output_path,
        doc_opts="PDF_LAYOUT_DETECTION=on;PDF_TABLE_DETECTION=on",
        canvas_opts="MARKDOWN_INCLUDE_LOCATIONS=on;MARKDOWN_FLAVOR=GPT"
    )

convert_to_markdown_with_locations("report.pdf", "report.md")

# Parse LOC comments to pair each text chunk with its page and bounding box.
# Each <!-- LOC: page, (left,top,right,bottom) --> comment immediately precedes
# the paragraph it annotates; coordinates are in points (1/72 inch) from top-left.
LOC_RE = re.compile(r'<!-- LOC: (\d+), \((-?\d+),(-?\d+),(-?\d+),(-?\d+)\) -->')

def parse_markdown_chunks(md_text):
    """Split location-annotated Markdown into chunks with page/bbox metadata."""
    chunks = []
    parts = LOC_RE.split(md_text)
    # parts layout: [pre-text, page, l, t, r, b, text, page, l, t, r, b, text, ...]
    # The first element is any content before the first LOC comment (no metadata).
    i = 1
    while i + 5 <= len(parts):
        page, left, top, right, bottom = (int(parts[i]), int(parts[i+1]),
                                          int(parts[i+2]), int(parts[i+3]),
                                          int(parts[i+4]))
        text = parts[i+5].strip() if i + 5 < len(parts) else ""
        if text:
            chunks.append({
                "text": text,
                "page": page,
                "bbox": (left, top, right, bottom),
            })
        i += 6
    return chunks

with open("report.md", encoding="utf-8") as f:
    md = f.read()
chunks = parse_markdown_chunks(md)
# Example: [{"text": "Executive Summary\n...", "page": 1, "bbox": (72, 90, 540, 110)}, ...]

# See extract-document-structure for a full parsing example and additional metadata fields.

# Batch conversion with SupportsText guard — skips formats that cannot yield text
# (e.g. container formats like ZIP, PST, MBOX, EML whose content lives in sub-files).
def batch_convert_to_markdown(input_paths, output_dir, doc_opts="", canvas_opts=""):
    os.makedirs(output_dir, exist_ok=True)
    for input_path in input_paths:
        try:
            with api.GetExtractor(input_path) as doc:
                doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, doc_opts)
                if not doc.SupportsText:
                    logging.warning("Skipping %s: format does not support text extraction "
                                    "(container format — use doc.SubFiles to recurse)", input_path)
                    continue
                stem = os.path.splitext(os.path.basename(input_path))[0]
                output_path = os.path.join(output_dir, stem + ".md")
                with api.MakeOutputCanvas(output_path, IGR_DEVICE_MARKDOWN, canvas_opts) as canvas:
                    for page in doc.Pages:
                        with page:
                            canvas.RenderPage(page)
        except IGRException as e:
            logging.error("Failed to convert %s: %s", input_path, e)

batch_convert_to_markdown(glob.glob("inbox/*"), "output_md")
```

## Options

PDF options are **document-level** — pass them to `doc.Open()`. Markdown options are **canvas-level** — pass them to `MakeOutputCanvas()`.

### PDF Input Options (document-level)

| Option                     | Values      | Default | Notes                                                                    |
| -------------------------- | ----------- | ------- | ------------------------------------------------------------------------ |
| `PDF_LAYOUT_DETECTION`     | `on`, `off` | `off`   | (PDF only) Master switch — enable for better structure detection in PDFs |
| `PDF_TABLE_DETECTION`      | `on`, `off` | `off`   | (PDF only) Detect tables in PDFs; requires `PDF_LAYOUT_DETECTION=on`     |
| `PDF_LIST_DETECTION`       | `on`, `off` | `off`   | (PDF only) Detect lists in PDFs; requires `PDF_LAYOUT_DETECTION=on`      |

> Note: `on`/`off` are the canonical values; `true`/`false` are accepted synonyms.

### Markdown Output Options (canvas-level)

#### Flavor & Structure

| Option                           | Values                            | Default | Notes                             |
| -------------------------------- | --------------------------------- | ------- | --------------------------------- |
| `MARKDOWN_FLAVOR`                | `GFM`, `GPT`                      | `GFM`   | `GPT` optimises for LLM ingestion |
| `MARKDOWN_INCLUDE_LOCATIONS`     | `on`, `off`                       | `off`   | Inject `<!-- LOC: page, (left,top,right,bottom) -->` before each paragraph — coordinates are in points (1/72 inch) from the top-left of the page; links text back to its page and bounding box for citation-aware RAG |
| `MARKDOWN_HEADERS_STYLE`         | `ATX` (`#`), `SETEXT` (underline) | `ATX`   |                                   |
| `MARKDOWN_PREFERRED_LINE_LENGTH` | integer                           | `80`    | Line wrap length                  |
| `MARKDOWN_HR_BETWEEN_PAGES`      | `on`, `off`                       | `off`   | Insert `---` between pages        |

#### Tables

| Option                         | Values                                                 | Default | Notes                                     |
| ------------------------------ | ------------------------------------------------------ | ------- | ----------------------------------------- |
| `MARKDOWN_SIMPLE_TABLE_STYLE`  | `PIPE`, `GRID`, `HTML`, `PIPE_WITH_HTML`, `PARAGRAPHS` | `PIPE`  |                                           |
| `MARKDOWN_COMPLEX_TABLE_STYLE` | `PIPE`, `GRID`, `HTML`, `PIPE_WITH_HTML`, `PARAGRAPHS` | `HTML`  | Merged/spanning cells                     |
| `MARKDOWN_TABLE_PADDING`       | `on`, `off`                                            | `on`    | Padding in pipe-style tables              |
| `MARKDOWN_TABLE_DETECT_TITLES` | `on`, `off`                                            | `off`   | Treat full-width first row as table title |

#### Content Inclusion

| Option                        | Values               | Default | Notes                            |
| ----------------------------- | -------------------- | ------- | -------------------------------- |
| `MARKDOWN_INCLUDE_IMAGES`     | `on`, `off`          | `off`   | Embed images as base64 data URIs |
| `MARKDOWN_INCLUDE_LINKS`      | `on`, `off`          | `on`    | Preserve hyperlinks              |
| `MARKDOWN_INCLUDE_FORMATTING` | `on`, `off`          | `on`    | Bold, italic, etc.               |
| `MARKDOWN_INCLUDE_HEADERS`    | `off`, `on`, `FIRST` | `off`   | Page headers                     |
| `MARKDOWN_INCLUDE_FOOTERS`    | `off`, `on`, `FIRST` | `off`   | Page footers                     |
| `MARKDOWN_INCLUDE_FOOTNOTES`  | `on`, `off`          | `off`   | Footnotes and endnotes           |
| `MARKDOWN_INCLUDE_FORMS`      | `on`, `off`          | `on`    | Form fields                      |
| `MARKDOWN_INCLUDE_BOOKMARKS`  | `on`, `off`          | `on`    | Document bookmarks               |
| `MARKDOWN_INCLUDE_FIELDS`     | `on`, `off`          | `on`    | Field codes                      |

#### Metadata

| Option                      | Values                                     | Default    | Notes                                                                                        |
| --------------------------- | ------------------------------------------ | ---------- | -------------------------------------------------------------------------------------------- |
| `MARKDOWN_INCLUDE_METADATA` | `on`, `off`                                | `off`      | Prepend document metadata                                                                    |
| `MARKDOWN_METADATA_FORMAT`  | `COMMENTS`, `YAML`, `TOML`, `JSON`, `None` | `COMMENTS` | Format for metadata block; `None` includes metadata as a plain block with no format wrapper. Only takes effect when `MARKDOWN_INCLUDE_METADATA=on` is set. |

#### Charts

| Option                         | Values      | Default | Notes                           |
| ------------------------------ | ----------- | ------- | ------------------------------- |
| `MARKDOWN_INCLUDE_CHARTS`      | `on`, `off` | `off`   | Master switch for chart data    |
| `MARKDOWN_INCLUDE_CHART_TABLE` | `on`, `off` | `off`   | Chart data as Markdown table    |
| `MARKDOWN_INCLUDE_CHART_JSON`  | `on`, `off` | `off`   | Chart data as JSON fenced block |

#### PDF-specific canvas options

| Option                     | Values      | Default | Notes                                                          |
| -------------------------- | ----------- | ------- | -------------------------------------------------------------- |
| `PDF_MARKDOWN_UNWRAP_TEXT` | `on`, `off` | `off`   | (PDF only) Unwrap hard-wrapped lines in PDFs for cleaner prose |

#### Cleanup

| Option                   | Values                                                          | Default | Notes                                      |
| ------------------------ | --------------------------------------------------------------- | ------- | ------------------------------------------ |
| `MARKDOWN_CLEAN_CONTENT` | `clean_non_ascii_chars`, `normalize_quotes`, `normalize_dashes` | —       | Comma-separated list of cleaning functions, e.g. `MARKDOWN_CLEAN_CONTENT=clean_non_ascii_chars,normalize_quotes` |

## Notes

- Eval mode limits output to the first few pages with a watermark; the following options require a valid license key: `MARKDOWN_FLAVOR=GPT`, `MARKDOWN_INCLUDE_CHARTS`, `MARKDOWN_INCLUDE_IMAGES`, `MARKDOWN_INCLUDE_LOCATIONS`, and PDF detection options (`PDF_LAYOUT_DETECTION`, `PDF_TABLE_DETECTION`, `PDF_LIST_DETECTION`) — eval mode only supports basic GFM output
- `MARKDOWN_FLAVOR=GPT` produces cleaner output for LLM ingestion — fewer decorative elements, plainer structure
- `MARKDOWN_METADATA_FORMAT` has no effect unless `MARKDOWN_INCLUDE_METADATA=on` is also set
- For PDFs with complex layouts, enable `PDF_LAYOUT_DETECTION=on` and `PDF_TABLE_DETECTION=on` together for best table fidelity
- For container formats (ZIP, PST, MBOX, EML), content is in sub-files — use `doc.SubFiles` to iterate and recursively extract each sub-document; iterating `doc.Pages` on these formats will produce empty output. Always call `doc.SupportsText` after `doc.Open()` in a batch loop and skip (or log a warning for) documents where it returns `False`
- Wrap `doc.Open()` in a `try/except IGRException` block to skip unsupported formats gracefully in batch conversion. Note: file-not-found also raises `IGRException` (not Python's `FileNotFoundError`) — check path existence beforehand or catch `IGRException` for all file I/O errors
- For password-protected files, set `doc.PasswordCallback = lambda id: os.environ.get("DOC_PASSWORD", "")` before calling `doc.Open()`. An incorrect or missing password raises `IGRException`
- For AI ingestion, canvas-based Markdown output (`MARKDOWN_FLAVOR=GPT`) preserves document structure — headings, tables, and lists — that is lost with plain `GetText()` extraction. Prefer this skill over raw text extraction when document structure matters for your LLM

## Reference

https://hyland.github.io/DocumentFilters-Docs/latest/
