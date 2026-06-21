---
name: process-documents-for-ai
description: Extract and chunk text from documents for LLM ingestion using Document Filters
---

# Process Documents for AI Pipelines

Hyland Document Filters runs entirely on-premise — no conversion servers, no data leaving your environment — and extracts clean text from 600+ file formats (Word, PDF, email archives, ZIP containers, and more) through a single API call. This skill shows how to extract and chunk document text for feeding into an LLM or vector store.

## Setup

```bash
pip install document-filters
```

Native runtime binaries (~60 MB) are downloaded automatically on first use. Set `DF_LICENSE_KEY` in your environment for full features — omitting it runs in evaluation mode. Evaluation mode inserts watermark strings directly into extracted text — do not use for production AI pipelines. Set `DF_LICENSE_KEY` to a valid key before extracting text that will be passed to an LLM or embedding model.

## Extract Text and Split into Chunks

```python
import os
from DocumentFilters import *

api = DocumentFilters()
api.Initialize(os.environ.get("DF_LICENSE_KEY", ""), ".")  # set DF_LICENSE_KEY env var; "" = eval mode

def extract_chunks(filename, chunk_size=1000, overlap=100):
    """
    Extract text from a document and split into overlapping chunks.
    Returns a list of dicts: {text, source, chunk_index}
    """
    if overlap >= chunk_size:
        raise ValueError("overlap must be less than chunk_size")
    with api.GetExtractor(filename) as doc:
        if not doc.getSupportsText():
            return []
        doc.Open(IGR_BODY_AND_META, "PDF_TABLE_DETECTION=on;PDF_LAYOUT_DETECTION=on")
        parts = []
        while not doc.getEOF():
            parts.append(doc.GetText(4096, stripControlCodes=True))
        full_text = "".join(parts)

    chunks = []
    start = 0
    idx = 0
    while start < len(full_text):
        end = min(start + chunk_size, len(full_text))
        chunks.append({
            "text": full_text[start:end],
            "source": filename,
            "chunk_index": idx,
        })
        step = chunk_size - overlap
        start += step
        idx += 1

    return chunks

# Usage
for chunk in extract_chunks("report.pdf", chunk_size=500, overlap=50):
    print(f"Chunk {chunk['chunk_index']}: {chunk['text'][:80]}...")
    # Pass chunk['text'] to your embedding model or LLM
```

## Convert to LLM-optimized Markdown

For documents with headings, tables, or structured lists, rendering to Markdown preserves that structure and provides richer context to the LLM than plain `GetText()` extraction, which flattens all formatting. Canvas-based markdown output uses `MARKDOWN_FLAVOR=GPT` for LLM-optimized formatting. This requires a valid `DF_LICENSE_KEY` — canvas devices are not available in evaluation mode.

```python
import io
import os
from DocumentFilters import *

api = DocumentFilters()
api.Initialize(os.environ.get("DF_LICENSE_KEY", ""), ".")

def convert_to_llm_markdown(filename, out_path):
    with api.GetExtractor(filename) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "PDF_TABLE_DETECTION=on;PDF_LAYOUT_DETECTION=on")
        with api.MakeOutputCanvas(out_path, IGR_DEVICE_MARKDOWN, "MARKDOWN_FLAVOR=GPT") as canvas:
            for page in doc.Pages:
                with page:
                    canvas.RenderPage(page)

def convert_to_llm_markdown_string(filename):
    buf = io.BytesIO()
    with api.GetExtractor(filename) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "PDF_TABLE_DETECTION=on;PDF_LAYOUT_DETECTION=on")
        with api.MakeOutputCanvas(buf, IGR_DEVICE_MARKDOWN, "MARKDOWN_FLAVOR=GPT") as canvas:
            for page in doc.Pages:
                with page:
                    canvas.RenderPage(page)
    return buf.getvalue().decode("utf-8")
```

## Processing Multiple Files

```python
import glob
import os
import sys

def process_folder(folder, chunk_size=1000, overlap=100):
    all_chunks = []
    for path in glob.glob(f"{folder}/**/*", recursive=True):
        if not os.path.isfile(path):
            continue
        try:
            chunks = extract_chunks(path, chunk_size, overlap)
            all_chunks.extend(chunks)
        except Exception as e:
            print(f"Skipping {path}: {e}", file=sys.stderr)
    return all_chunks
```

## SubFiles (Archives and Email Folders)

For compound formats such as `.zip`, `.pst`, and `.mbox`, iterate child documents via `SubFiles`. Each child must be opened individually before text can be extracted. Attachments can themselves be archives (e.g., a `.pst` message with an attached `.zip`), so the helper below recurses until it reaches leaf documents. The function returns a flat list of chunks — one entry per message or attachment — which maps directly to documents in a vector store. Each chunk carries the attachment name as `source`, enabling source citation in RAG responses.

```python
def _chunk_text(text, source, chunk_size, overlap):
    """Split text into overlapping chunks. Returns list of {text, source, chunk_index}."""
    chunks = []
    start = 0
    idx = 0
    step = chunk_size - overlap
    while start < len(text):
        chunks.append({
            "text": text[start:min(start + chunk_size, len(text))],
            "source": source,
            "chunk_index": idx,
        })
        start += step
        idx += 1
    return chunks

def _extract_from_doc(doc, source_name, chunk_size, overlap, all_chunks):
    """Recursively extract text from a document or container."""
    if doc.getSupportsSubFiles():
        # Containers (.zip, .pst, .mbox) do not need PDF options; open with empty options string.
        doc.Open(IGR_BODY_AND_META, "")
        for child in doc.SubFiles:
            with child:
                _extract_from_doc(child, child.getName(), chunk_size, overlap, all_chunks)
    if doc.getSupportsText():
        doc.Open(IGR_BODY_AND_META, "PDF_TABLE_DETECTION=on;PDF_LAYOUT_DETECTION=on")
        parts = []
        while not doc.getEOF():
            parts.append(doc.GetText(4096, stripControlCodes=True))
        all_chunks.extend(_chunk_text("".join(parts), source_name, chunk_size, overlap))

def extract_with_subfiles(filename, chunk_size=1000, overlap=100):
    all_chunks = []
    with api.GetExtractor(filename) as doc:
        _extract_from_doc(doc, filename, chunk_size, overlap, all_chunks)
    return all_chunks
```

## Real-World Example: RAG Pipeline over a SharePoint Export

Extract all Word and PDF files from a folder, chunk them, and load into a vector store for retrieval-augmented generation:

```python
import chromadb

collection = chromadb.Client().create_collection("policies")
chunks = process_folder("/exports/sharepoint", chunk_size=800, overlap=80)
collection.add(
    documents=[c["text"] for c in chunks],
    metadatas=[{"source": c["source"], "chunk": c["chunk_index"]} for c in chunks],
    ids=[f"{c['source']}::{c['chunk_index']}" for c in chunks],
)
# The collection is now queryable: collection.query(query_texts=["vacation policy"], n_results=5)
```

For email archives (`.pst`, `.mbox`), replace `process_folder` with `extract_with_subfiles` — it recurses through messages and attachments automatically, producing the same `{text, source, chunk_index}` shape.

## Notes

- **Prefer Markdown for structured documents:** When feeding an LLM and document structure (headings, tables, lists) matters, use `convert_to_llm_markdown()` (canvas-based, `MARKDOWN_FLAVOR=GPT`) instead of `extract_chunks()`. Plain text extraction loses all structure; Markdown preserves it. Requires a valid `DF_LICENSE_KEY`.
- Use `chunk_size` and `overlap` to match your embedding model's context window
- Canvas-based output (`IGR_DEVICE_MARKDOWN`, `IGR_DEVICE_HTML`, `IGR_DEVICE_IMAGE_PDF`, etc.) requires a valid `DF_LICENSE_KEY`. These devices are not available in evaluation mode.
- **Unsupported formats:** When `getSupportsText()` returns `False` and `getSupportsSubFiles()` returns `False`, the format is not supported for text extraction. `extract_chunks()` returns an empty list in this case — check for it before passing to an LLM.
- **Password-protected files:** `Open()` raises `IGRException` if the file requires a password and none is supplied. To provide a password programmatically, set `PasswordCallback` on the extractor before calling `Open()`:

  ```python
  with api.GetExtractor(filename) as doc:
      doc.PasswordCallback = lambda doc_id: "secret"
      doc.Open(IGR_BODY_AND_META, "")
      ...
  ```

- **Missing or unreadable files:** `Open()` raises `IGRException` for files that do not exist, cannot be read, or are corrupt. Wrap extraction calls in `try/except IGRException` when processing untrusted or user-supplied paths.

## Options

### doc.Open() options string (semicolon-separated)

| Option | Values | Description |
|---|---|---|
| `PDF_TABLE_DETECTION` | `on` / `off` | Detect table structure in PDFs; improves LLM context quality |
| `PDF_LAYOUT_DETECTION` | `on` / `off` | Detect column/layout structure in PDFs |
| `PDF_BOOKMARKS` | `true` / `false` | Include PDF bookmark/outline headings as text nodes — improves section-level context for chunking and retrieval |

> Note: `true`/`false` are the canonical values; `on`/`off` and `1`/`0` are accepted synonyms.

### MakeOutputCanvas() options string (semicolon-separated, canvas only)

| Option | Values | Description |
|---|---|---|
| `MARKDOWN_FLAVOR` | `GPT` / `GFM` | LLM-optimized markdown (`GPT`) or GitHub-Flavored Markdown |
| `MARKDOWN_HEADERS_STYLE` | `ATX` / `Setext` | Heading style in output markdown |
| `MARKDOWN_SIMPLE_TABLE_STYLE` | `PIPE` / `GRID` / `HTML` / `PIPE_WITH_HTML` | Rendering style for simple tables (default: `PIPE`) |
| `MARKDOWN_COMPLEX_TABLE_STYLE` | `PIPE` / `GRID` / `HTML` / `PIPE_WITH_HTML` | Rendering style for complex/merged-cell tables (default: `HTML`) |
| `MARKDOWN_INCLUDE_HEADERS` | `on` / `off` | Include page headers in output (default: `off`) |
| `MARKDOWN_INCLUDE_FOOTERS` | `on` / `off` | Include page footers in output (default: `off`) |
| `MARKDOWN_INCLUDE_IMAGES` | `on` / `off` | Include embedded images in output (default: `off`) |
| `MARKDOWN_INCLUDE_LOCATIONS` | `on` / `off` | Inject `<!-- LOC: page, (l,t,r,b) -->` before each paragraph — enables chunk-to-source traceability for RAG pipelines (default: `off`) |

> Note: `on`/`off` are the canonical values; `true`/`false` and `1`/`0` are accepted synonyms.

To parse LOC comments downstream and map each chunk back to its source page and bounding box:

```python
import re

LOC_RE = re.compile(r'<!-- LOC: (\d+), \((-?\d+),(-?\d+),(-?\d+),(-?\d+)\) -->')

def parse_loc_comments(markdown_text):
    """
    Return a list of (page, (left, top, right, bottom)) tuples
    for each LOC comment found in the markdown output.
    """
    results = []
    for m in LOC_RE.finditer(markdown_text):
        page = int(m.group(1))
        bbox = (int(m.group(2)), int(m.group(3)), int(m.group(4)), int(m.group(5)))
        results.append((page, bbox))
    return results
```

## Reference

https://hyland.github.io/DocumentFilters-Docs/latest/
