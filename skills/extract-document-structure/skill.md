---
name: extract-document-structure
description: Extract structured document content as Markdown or JSON using Document Filters, preserving headings, tables, paragraphs, and word-level bounding boxes
---

# Extract Document Structure

Hyland Document Filters extracts clean, structured content from any document — PDFs, Word files, spreadsheets, email, and 600+ other formats — running locally with no servers required. Feed any document — including old PDFs, scanned invoices, or legacy Word files — into an AI pipeline and get clean, structured text back in seconds. This makes it well-suited for document AI pipelines where you need accurate, private, high-throughput extraction at scale.

It can extract the logical structure of a document in two complementary forms:

- **Markdown** — headings, tables, lists, and paragraphs as readable text; ideal for LLM ingestion
- **JSON** — paragraphs, runs, words, bounding boxes, fonts, and styles as machine-readable data; ideal for form parsing, spatial queries, and document understanding pipelines

## Setup

```bash
pip install document-filters
```

Native runtime binaries (~60 MB) are downloaded automatically on first use. Set `DF_LICENSE_KEY` in your environment for full features — omitting it runs in evaluation mode (watermarked output, limited pages).

Document Filters supports 600+ formats including PDF, Word, Excel, PowerPoint, email (MSG/EML), HTML, and many legacy formats — no per-format plugins required.

## Extract as Markdown

Markdown preserves document structure as readable text. Use `MARKDOWN_FLAVOR=GPT` for cleaner LLM ingestion.

```python
import os, io
from DocumentFilters import *

api = DocumentFilters()
api.Initialize(os.environ.get("DF_LICENSE_KEY", ""), ".")

def to_markdown(input_path, output_path=None, doc_opts="", canvas_opts=""):
    """
    Convert any document to Markdown.
    Always returns the Markdown string; when output_path is given, the file is
    also written to disk (no extra file-read — the string is captured in memory
    via BytesIO and written separately when an output path is provided).
    Note: returns an empty string for image-only or unsupported formats; use
    OCR pre-processing for scanned PDFs.
    """
    buf = io.BytesIO()
    with api.GetExtractor(input_path) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, doc_opts)
        if not doc.getSupportsText():
            return ""  # image-only or unsupported format — OCR needed for scanned PDFs
        with api.MakeOutputCanvas(buf, IGR_DEVICE_MARKDOWN, canvas_opts) as canvas:
            for page in doc.Pages:
                with page:
                    canvas.RenderPage(page)
    content = buf.getvalue().decode("utf-8")
    if output_path:
        with open(output_path, "w", encoding="utf-8") as f:
            f.write(content)
    return content

# Basic — preserves headings, tables, lists
md = to_markdown("report.docx")

# Optimised for LLM ingestion — disable image embedding (default on) to avoid
# large base64 blobs in the prompt; enable metadata for provenance tracking
md = to_markdown(
    "report.pdf",
    doc_opts="PDF_LAYOUT_DETECTION=on;PDF_TABLE_DETECTION=on;PDF_LIST_DETECTION=on",
    canvas_opts="MARKDOWN_FLAVOR=GPT;MARKDOWN_INCLUDE_IMAGES=off;MARKDOWN_INCLUDE_METADATA=on;MARKDOWN_METADATA_FORMAT=YAML",
)

# Full fidelity — include images, charts, headers/footers
md = to_markdown(
    "slides.pptx",
    canvas_opts="MARKDOWN_INCLUDE_IMAGES=on;MARKDOWN_INCLUDE_CHARTS=on;MARKDOWN_INCLUDE_CHART_TABLE=on",
)
```

### Markdown Options

PDF input options are **document-level** (pass to `doc.Open()`). Markdown options are **canvas-level** (pass to `MakeOutputCanvas()`).

#### PDF Input (document-level)

| Option                     | Values      | Default | Notes                                                     |
|----------------------------|-------------|---------|-----------------------------------------------------------|
| `PDF_LAYOUT_DETECTION`     | `on`, `off` | `off`   | Master switch for structure detection in PDFs             |
| `PDF_TABLE_DETECTION`      | `on`, `off` | `off`   | Reconstruct tables; requires `PDF_LAYOUT_DETECTION=on`    |
| `PDF_LIST_DETECTION`       | `on`, `off` | `off`   | Detect lists; requires `PDF_LAYOUT_DETECTION=on`          |

#### Markdown Output (canvas-level)

| Option                           | Values                             | Default    | Notes                              |
|----------------------------------|------------------------------------|------------|------------------------------------|
| `PDF_MARKDOWN_UNWRAP_TEXT`       | `on`, `off`                        | `off`      | (PDF only) Unwrap hard-wrapped lines in PDFs for cleaner prose |
| `MARKDOWN_FLAVOR`                | `GFM`, `GPT`                       | `GFM`      | `GPT` optimises output for LLMs    |
| `MARKDOWN_INCLUDE_LOCATIONS`     | `on`, `off`                        | `off`      | Inject `<!-- LOC: page, (l,t,r,b) -->` before each paragraph — links text back to its page and bounding box |
| `MARKDOWN_INCLUDE_METADATA`      | `on`, `off`                        | `off`      | Prepend document metadata block    |
| `MARKDOWN_METADATA_FORMAT`       | `YAML`, `TOML`, `JSON`, `COMMENTS` | `COMMENTS` | Format of the metadata block       |
| `MARKDOWN_SIMPLE_TABLE_STYLE`    | `PIPE`, `GRID`, `HTML`, `PIPE_WITH_HTML` | `PIPE` |                               |
| `MARKDOWN_COMPLEX_TABLE_STYLE`   | `PIPE`, `GRID`, `HTML`, `PIPE_WITH_HTML` | `HTML` | Merged/spanning cells; `PIPE_WITH_HTML` renders pipe-style with HTML for merged cells |
| `MARKDOWN_INCLUDE_IMAGES`        | `on`, `off`                        | `on`       | Embed images as base64 data URIs   |
| `MARKDOWN_INCLUDE_CHARTS`        | `on`, `off`                        | `off`      | Master switch for chart data       |
| `MARKDOWN_INCLUDE_CHART_TABLE`   | `on`, `off`                        | `off`      | Chart data as Markdown table       |
| `MARKDOWN_INCLUDE_HEADERS`       | `off`, `on`, `FIRST`               | `off`      | Page headers                       |
| `MARKDOWN_INCLUDE_FOOTERS`       | `off`, `on`, `FIRST`               | `off`      | Page footers                       |
| `MARKDOWN_INCLUDE_FOOTNOTES`     | `on`, `off`                        | `off`      | Footnotes and endnotes             |
| `MARKDOWN_HR_BETWEEN_PAGES`      | `on`, `off`                        | `off`      | Insert `---` between pages         |
| `MARKDOWN_HEADERS_STYLE`         | `ATX`, `SETEXT`                    | `ATX`      | ATX uses `#` prefixes; SETEXT uses underline style |

## Location Data in Markdown

`MARKDOWN_INCLUDE_LOCATIONS=on` injects an HTML comment before each paragraph with its page number and bounding box:

```
<!-- LOC: 1, (100,257,1284,683) -->
# How Hyland Won Erie Insurance
```

The format is `<!-- LOC: page, (left,top,right,bottom) -->` in points (1/72 inch) from the top-left of the page.

This is useful for **citation-aware RAG** — after retrieval, you can resolve each chunk back to its exact page and coordinates for highlighting or verification:

```python
import io, re, os
from DocumentFilters import *

api = DocumentFilters()
api.Initialize(os.environ.get("DF_LICENSE_KEY", ""), ".")

LOC_RE = re.compile(r'<!-- LOC: (\d+), \((-?\d+),(-?\d+),(-?\d+),(-?\d+)\) -->')

def to_markdown_with_locations(input_path):
    """Return list of {page, bounds, text} dicts, one per paragraph."""
    buf = io.BytesIO()
    with api.GetExtractor(input_path) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "")
        with api.MakeOutputCanvas(buf, IGR_DEVICE_MARKDOWN,
                                  "MARKDOWN_INCLUDE_LOCATIONS=on;MARKDOWN_FLAVOR=GPT") as canvas:
            for page in doc.Pages:
                with page:
                    canvas.RenderPage(page)
    md = buf.getvalue().decode("utf-8")

    paragraphs = []
    current_loc = None
    for line in md.splitlines():
        m = LOC_RE.match(line)
        if m:
            current_loc = {
                "page": int(m.group(1)),
                "bounds": (int(m.group(2)), int(m.group(3)), int(m.group(4)), int(m.group(5))),
            }
        elif current_loc and line.strip():
            paragraphs.append({**current_loc, "text": line.strip()})
            current_loc = None
    return paragraphs

paragraphs = to_markdown_with_locations("report.pdf")
for p in paragraphs[:3]:
    print(f"Page {p['page']} {p['bounds']}: {p['text'][:60]}")
# Page 1 (100, 257, 1284, 683): # How Hyland Won Erie Insurance...
```

## Extract as JSON

JSON output includes paragraphs, runs, font data, hyperlinks, and optionally word-level bounding boxes — useful for spatial queries, layout analysis, and form field extraction.

```python
import os, io, json
from DocumentFilters import *

api = DocumentFilters()
api.Initialize(os.environ.get("DF_LICENSE_KEY", ""), ".")

def to_json(input_path, output_path=None, include_words=False, pretty=False):
    """
    Export document structure as JSON.
    Returns the parsed dict (always in-memory via BytesIO).
    When output_path is given, also writes the JSON file to disk.
    Note: MakeOutputCanvas accepts either a file path or a stream — passing
    io.BytesIO() keeps everything in memory, which is ideal for pipelines that
    load documents from S3 or database blobs without touching the filesystem.
    Note: returns an empty dict for image-only or unsupported formats; use
    OCR pre-processing for scanned PDFs.
    """
    canvas_opts = (
        f"JSON_FORMAT_OUTPUT={str(pretty).lower()};"
        f"JSON_INCLUDE_WORDS={str(include_words).lower()};"
        "JSON_SKIP_EMPTY_PARAGRAPHS=true;"
    )
    buf = io.BytesIO()
    with api.GetExtractor(input_path) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "")
        if not doc.getSupportsText():
            return {}  # image-only or unsupported format — OCR needed for scanned PDFs
        with api.MakeOutputCanvas(buf, IGR_DEVICE_JSON, canvas_opts) as canvas:
            for page in doc.Pages:
                with page:
                    canvas.RenderPage(page)
    data = json.loads(buf.getvalue().decode("utf-8"))
    if output_path:
        with open(output_path, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2 if pretty else None)
    return data

structure = to_json("report.pdf", pretty=True)
structure_with_words = to_json("invoice.pdf", include_words=True)

# In-memory only (e.g. document bytes loaded from S3 or a database blob):
# buf = io.BytesIO(document_bytes)
# structure = to_json(buf, include_words=True)
```

### JSON Canvas Options

| Option                       | Values          | Default | Notes                                      |
|------------------------------|-----------------|---------|--------------------------------------------|
| `JSON_FORMAT_OUTPUT`         | `true`, `false` | `false` | Pretty-print with newlines and indentation |
| `JSON_INCLUDE_WORDS`         | `true`, `false` | `false` | Include word-level bounding box data       |
| `JSON_INCLUDE_WHITESPACE`    | `true`, `false` | `false` | Include whitespace-only word entries       |
| `JSON_SKIP_EMPTY_PARAGRAPHS` | `true`, `false` | `false` | Omit paragraphs with no text content       |

## Word-Level Bounding Boxes

Access word coordinates directly via the page API — no need to go through the JSON device.

```python
import os
from DocumentFilters import *

api = DocumentFilters()
api.Initialize(os.environ.get("DF_LICENSE_KEY", ""), ".")

def extract_words(path):
    """Return all words with page coordinates (in points, 1/72 inch)."""
    words = []
    with api.GetExtractor(path) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "")
        for page_idx, page in enumerate(doc.Pages):
            with page:
                for word in page.Words:
                    words.append({
                        "page": page_idx + 1,
                        "text": word.Text,
                        "x": word.X,
                        "y": word.Y,
                        "width": word.Width,
                        "height": word.Height,
                    })
    return words
```

## Page Statistics

```python
# Assumes api initialized as shown above
def page_stats(path):
    results = []
    with api.GetExtractor(path) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "")
        for i, page in enumerate(doc.Pages):
            with page as p:
                results.append({
                    "page": i + 1,
                    "width_pts": p.Width,
                    "height_pts": p.Height,
                    "word_count": p.WordCount,
                })
    return results
```

## When to Use Each Format

| Use case | Recommended format |
|---|---|
| LLM ingestion, RAG chunking, summarization | Markdown (`MARKDOWN_FLAVOR=GPT`, `MARKDOWN_INCLUDE_IMAGES=off`) |
| Citation-aware RAG (resolve chunk back to page/coordinates) | Markdown with `MARKDOWN_INCLUDE_LOCATIONS=on` |
| Table extraction from PDFs | Markdown with `PDF_TABLE_DETECTION=on` |
| Form field extraction, layout analysis | JSON with `JSON_INCLUDE_WORDS=true` |
| Spatial queries (find text in a region) | Word bounding boxes via `page.Words` |
| Heading/style detection, classification | JSON (includes paragraph style and run-level font data) |

## Notes

- Word coordinates are in points (1/72 inch) from the top-left of the page; cast to `int()` when passing to `canvas.Rect()`
- `page.Words` and `page.WordCount` require `IGR_FORMAT_IMAGE` to be included in the flags passed to `doc.Open()` — use `IGR_BODY_AND_META | IGR_FORMAT_IMAGE`
- The JSON device includes hyperlinks and run-level font attributes (bold, italic, size, color) — useful for heading detection without style names
- `doc.getSupportsText()` returns `False` for image-only PDFs and unsupported formats — both `to_markdown` and `to_json` guard on this; use an OCR pre-processing step (e.g. Tesseract) for scanned PDFs before passing to Document Filters
- Eval mode limits output to the first few pages with a watermark; `IGR_DEVICE_JSON` (the JSON canvas) requires a valid license — JSON extraction will fail in evaluation mode. The Markdown canvas works in eval mode (with the page limit and watermark). Set `DF_LICENSE_KEY` for full access to all features.

## Reference

https://hyland.github.io/DocumentFilters-Docs/latest/
