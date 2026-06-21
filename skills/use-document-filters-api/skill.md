---
name: use-document-filters-api
description: Initialize and use the Hyland Document Filters SDK in Python, C#, Java, or C++
---

# Using the Document Filters API

Document Filters turns any file — Word, PDF, email, archive, CAD drawing, and 600+ more formats — into clean text or structured output ready for LLMs, RAG pipelines, and document AI, running entirely on your own infrastructure with no conversion servers or cloud dependencies.

## Setup (Python)

Install the package:
```bash
pip install document-filters
```

Initialize the API:
```python
import os
from DocumentFilters import *

api = DocumentFilters()
# The Python SDK does NOT auto-read any env var — pass the key explicitly.
# Empty string = evaluation mode (limited pages, watermarked output).
api.Initialize(os.environ.get("DF_LICENSE_KEY", ""), ".")
```

Set `DF_LICENSE_KEY` in your environment or `.env` file. The `dll_path` third argument defaults to `None` (auto-detect) and can be omitted. When using the pip package, binaries are bundled. When running the GitHub samples directly, they are downloaded on first use.

## Core Pattern

All Document Filters operations follow the same pattern: get an extractor for a file, open it in the desired mode, then process pages or text.

```python
import os
from DocumentFilters import *

api = DocumentFilters()
api.Initialize(os.environ.get("DF_LICENSE_KEY", ""), ".")

with api.GetExtractor("input.docx") as doc:
    # Check what the file supports
    print(doc.getFileType(IGR_FORMAT_LONG_NAME))   # e.g. "Microsoft Word 2007"
    print(doc.getSupportsText())                   # True if text extraction works
    print(doc.getSupportsSubFiles())               # True for archives, emails, etc.

    # Open for text extraction
    doc.Open(IGR_BODY_AND_META, "")

    # Open for rendering (pages to images, PDF, HTML, etc.)
    doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "")
```

### Text Extraction

```python
with api.GetExtractor("input.docx") as doc:
    doc.Open(IGR_BODY_AND_META, "")
    if not doc.getSupportsText():
        # File does not support text extraction (e.g. image-only PDF, CAD drawing)
        return
    text = []
    while not doc.getEOF():
        chunk = doc.GetText(4096, stripControlCodes=True)
        text.append(chunk)
    full_text = "".join(text)
```

### Page Rendering

```python
with api.GetExtractor("input.docx") as doc:
    doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "")
    with api.MakeOutputCanvas("output.pdf", IGR_DEVICE_IMAGE_PDF, "") as canvas:
        for page in doc.Pages:
            with page:
                canvas.RenderPage(page)
```

### Subfile Traversal (archives, emails)

```python
with api.GetExtractor("input.zip") as doc:
    if doc.getSupportsSubFiles():
        for child in doc.SubFiles:
            with child:
                child.Open(IGR_BODY_AND_META, "")
                if not child.getSupportsText():
                    continue
                while not child.getEOF():
                    chunk = child.GetText(4096, stripControlCodes=True)
                    # process chunk
```

## Putting It Together

The snippet below processes a folder of mixed-format files — PDFs, Word documents, emails, and archives — and converts each one to Markdown for RAG ingestion. It handles subfile traversal for archives and emails, skips image-only files gracefully, and uses `MARKDOWN_FLAVOR=GPT` for cleaner LLM input.

```python
import os
from pathlib import Path
from DocumentFilters import *

api = DocumentFilters()
api.Initialize(os.environ.get("DF_LICENSE_KEY", ""), ".")

OPEN_OPTS = "PDF_LAYOUT_DETECTION=ON;PDF_TABLE_DETECTION=ON"
MD_OPTS   = "MARKDOWN_FLAVOR=GPT;MARKDOWN_INCLUDE_IMAGES=false;MARKDOWN_INCLUDE_LOCATIONS=true"

def extract_to_markdown(extractor, canvas):
    """Render one extractor (file or subfile) to the shared canvas."""
    extractor.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, OPEN_OPTS)
    if not extractor.getSupportsText():
        return  # image-only file — skip silently
    for page in extractor.Pages:
        with page:
            canvas.RenderPage(page)

def process_file(path: str, output_dir: str):
    out_path = Path(output_dir) / (Path(path).stem + ".md")
    with api.GetExtractor(path) as doc:
        with api.MakeOutputCanvas(str(out_path), IGR_DEVICE_MARKDOWN, MD_OPTS) as canvas:
            if doc.getSupportsSubFiles():
                # Archives, ZIP, EML, MSG — recurse into attachments
                for child in doc.SubFiles:
                    with child:
                        extract_to_markdown(child, canvas)
            else:
                extract_to_markdown(doc, canvas)
    return out_path

input_folder  = "docs/"
output_folder = "output_md/"
Path(output_folder).mkdir(exist_ok=True)

for file in Path(input_folder).iterdir():
    if file.is_file():
        try:
            result = process_file(str(file), output_folder)
            print(f"OK  {file.name} -> {result.name}")
        except IGRException as e:
            print(f"ERR {file.name}: {e}")
```

Key points: `getSupportsSubFiles()` is checked before iterating children so plain files never trigger subfile logic. `getSupportsText()` guards against image-only PDFs and CAD drawings. A single `MakeOutputCanvas` per input file keeps all pages and attachments in one output document.

## Open Modes

| Flag | Meaning |
|---|---|
| `IGR_BODY_AND_META` | Extract text and metadata |
| `IGR_FORMAT_IMAGE` | Enable page rendering |

## Output Devices (for rendering)

| Constant | Output | Notes |
|---|---|---|
| `IGR_DEVICE_IMAGE_PDF` | PDF | |
| `IGR_DEVICE_HTML` | HD HTML5 | Requires a full license; not available in evaluation mode. |
| `IGR_DEVICE_IMAGE_PNG` | PNG (one file per page) | |
| `IGR_DEVICE_IMAGE_TIF` | TIFF | |
| `IGR_DEVICE_MARKDOWN` | Markdown | |
| `IGR_DEVICE_XML` | XML | |
| `IGR_DEVICE_JSON` | JSON | |

### Markdown Canvas Options

When using `IGR_DEVICE_MARKDOWN`, pass a semicolon-separated options string to `MakeOutputCanvas`:

| Option | Values | Description |
|---|---|---|
| `MARKDOWN_FLAVOR` | `GPT`, `GFM` | `GPT` is LLM-optimized (cleaner structure, reduced noise); recommended for AI pipelines |
| `MARKDOWN_HEADERS_STYLE` | `ATX`, `SETEXT` | |
| `MARKDOWN_SIMPLE_TABLE_STYLE` | `PIPE`, `GRID`, `HTML`, `PIPE_WITH_HTML` | |
| `MARKDOWN_COMPLEX_TABLE_STYLE` | `PIPE`, `GRID`, `HTML`, `PIPE_WITH_HTML` | |
| `MARKDOWN_INCLUDE_HEADERS` | `on`, `off` | |
| `MARKDOWN_INCLUDE_FOOTERS` | `on`, `off` | |
| `MARKDOWN_INCLUDE_IMAGES` | `on`, `off` | |
| `MARKDOWN_INCLUDE_LOCATIONS` | `on`, `off` | Injects `<!-- LOC: page, (l,t,r,b) -->` before each paragraph — links text back to its source page and bounding box; useful for citation and grounding in RAG pipelines |

Use `MARKDOWN_FLAVOR=GPT` when feeding Document Filters output into an LLM or AI agent pipeline.

> Boolean option values accept `true`/`false`, `on`/`off`, and `1`/`0` interchangeably (case-insensitive).

Example:
```python
opts = "MARKDOWN_FLAVOR=GPT;MARKDOWN_SIMPLE_TABLE_STYLE=PIPE;MARKDOWN_INCLUDE_IMAGES=false"
with api.MakeOutputCanvas("output.md", IGR_DEVICE_MARKDOWN, opts) as canvas:
    for page in doc.Pages:
        with page:
            canvas.RenderPage(page)
```

## Doc-Open Options

Pass a semicolon-separated options string as the second argument to `doc.Open()` to control document processing. For PDFs, enabling layout and table detection significantly improves table extraction fidelity:

```python
options = "PDF_LAYOUT_DETECTION=ON;PDF_TABLE_DETECTION=ON"
doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, options)
```

| Option | Values | Effect |
|---|---|---|
| `PDF_LAYOUT_DETECTION` | `ON`, `OFF` | Improves column and layout detection in PDFs |
| `PDF_TABLE_DETECTION` | `ON`, `OFF` | Improves table structure detection in PDFs |
| `PDF_BOOKMARKS` | `true`, `false` | Includes PDF bookmark headings in extracted text and rendered output. Must be passed to `doc.Open()`, not `MakeOutputCanvas()`. |

> Option values are case-insensitive; `ON`/`OFF`, `true`/`false`, and `1`/`0` are all accepted synonyms.

## Document Comparison

Document comparison requires a full license (not available in evaluation mode).

```python
with api.GetExtractor("original.docx") as doc1:
    with api.GetExtractor("revised.docx") as doc2:
        doc1.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "")
        doc2.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "")

        settings = CompareSettings()
        settings.CompareType = IGR_COMPARE_DOCUMENTS_COMPARE_WORDS

        results = doc1.Compare(doc2, compareSettings=settings)
        while results.MoveNext():
            diff = results.Current
            for hit in diff.Details:
                print(diff.Type, hit.Text)
```

`diff.Type` indicates the kind of change (insertion, deletion, move, etc.). `diff.Details` is a list of `CompareResultDifferenceDetail` objects; each has `.Text`, `.Bounds`, and `.PageIndex`. Use `diff.GetText()` to join all detail texts into a single string.

Additional comparison flags can be OR-combined with `IGR_COMPARE_DOCUMENTS_FLAGS_NO_CASE` to suppress specific content from the diff: `IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS`, `NO_FOOTERS`, `NO_TEXTBOXES`, `NO_FIELDS`, `NO_FOOTNOTES`, `NO_WHITESPACE`, `NO_PUNCTUATION`, and others. See the `compare-documents` skill for the full flag table.

## Other Languages

- **C#**: Install `Hyland.DocumentFilters` NuGet package. `var df = new DocumentFilters(); df.Initialize("", ".");` — the C# SDK reads `DOCFILTERS_LICENSE_KEY` automatically when an empty string is passed (note: different env var name from Python's `DF_LICENSE_KEY`).
- **Java**: Add the DocumentFilters JAR. API is `DocumentFilters api = new DocumentFilters(); api.Initialize(licenseKey, ".");`
- **C++**: Link against `ISYS11df`. Include `DocumentFiltersObjects.h`.

## Notes

- **`GetExtractor()` defers errors to `Open()`**: `api.GetExtractor()` does not raise for missing or unreadable files. Errors (file not found, unsupported format, permission denied) surface when `doc.Open()` is called. Always wrap `Open()` — not just `GetExtractor()` — in `try/except IGRException`.
- **Python vs C# env var names differ**: The Python SDK reads `DF_LICENSE_KEY`; the C# SDK reads `DOCFILTERS_LICENSE_KEY`. Setting the wrong variable leaves the SDK in evaluation mode with no error.
- **Evaluation mode injects watermarks into extracted content**: When initialized with an empty string license key, Document Filters inserts watermark text into extracted text, Markdown output, and HTML output. This makes evaluation-mode output unsuitable for production AI pipelines — watermark strings will appear verbatim in LLM inputs, embeddings, and search indexes. Set `DF_LICENSE_KEY` to a valid key for any pipeline that processes extracted content programmatically.

## Reference

Full API docs: https://hyland.github.io/DocumentFilters-Docs/latest/
