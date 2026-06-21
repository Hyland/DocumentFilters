---
name: summarize-document
description: Extract text from any document and produce a summary using an LLM
---

# Summarize a Document

Hyland Document Filters extracts text from 600+ file formats — Word, PDF, Excel, PowerPoint, email, archives, and more — without conversion servers or cloud dependencies. This skill uses it to extract text from any supported file and pass it to an LLM for summarization.

## Setup

```bash
pip install document-filters anthropic
```

Native runtime binaries (~60 MB) are downloaded automatically on first use. Binaries are cached in the package directory after the first download and do not re-download on subsequent runs. Set `DF_LICENSE_KEY` in your environment for full features — omitting it runs in evaluation mode (watermarked output, limited pages).

## Step 1 — Extract the text

```python
import os
from DocumentFilters import *

api = DocumentFilters()
api.Initialize(os.environ.get("DF_LICENSE_KEY", ""), ".")  # set DF_LICENSE_KEY env var; "" = eval mode

def get_document_text_from_extractor(doc, max_chars=20000):
    parts = []
    total = 0
    if doc.getSupportsText():
        doc.Open(IGR_BODY_AND_META, "")
        while not doc.getEOF() and total < max_chars:
            chunk = doc.GetText(4096, stripControlCodes=True)
            remaining = max_chars - total
            parts.append(chunk[:remaining])
            total += min(len(chunk), remaining)
            if total >= max_chars:
                break
    if doc.getSupportsSubFiles():
        for child in doc.SubFiles:
            with child:
                parts.append(get_document_text_from_extractor(child, max(0, max_chars - total)))
    return "".join(parts)

def get_document_text(filename, max_chars=20000):
    with api.GetExtractor(filename) as doc:
        return get_document_text_from_extractor(doc, max_chars)
```

## Step 2 — Summarize with an LLM

Pass the extracted text to your preferred LLM. Example using the Anthropic SDK:

```python
import anthropic

client = anthropic.Anthropic()  # uses ANTHROPIC_API_KEY env var

def summarize(filename):
    text = get_document_text(filename)
    message = client.messages.create(
        model="claude-sonnet-4-6",  # fast and cost-effective for summarization; swap for claude-opus-4-8 for higher accuracy on complex documents
        max_tokens=1024,
        messages=[{
            "role": "user",
            "content": f"Summarize this document in 3-5 bullet points:\n\n{text}"
        }]
    )
    return message.content[0].text

print(summarize("quarterly_report.pdf"))
```

## Alternative — Render to Markdown for richer LLM input

For documents with headings, tables, or complex structure, rendering to Markdown preserves that structure and improves LLM summarization quality:

```python
def get_document_markdown(filename, output_path="output.md"):
    with api.GetExtractor(filename) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "PDF_LAYOUT_DETECTION=on;PDF_TABLE_DETECTION=on")  # improve table fidelity for PDFs; safe to pass for non-PDF formats
        canvas_options = "MARKDOWN_FLAVOR=GPT"
        with api.MakeOutputCanvas(output_path, IGR_DEVICE_MARKDOWN, canvas_options) as canvas:
            for page in doc.Pages:
                with page:
                    canvas.RenderPage(page)
    with open(output_path, "r", encoding="utf-8") as f:
        return f.read()
```

Pass the result of `get_document_markdown(filename)` to the LLM instead of `get_document_text(filename)`.

**Note:** `output_path` is a required intermediate file written to disk and is not automatically deleted. Either pass a path inside a temporary directory or delete it after use. To avoid residual files, use `tempfile.NamedTemporaryFile(suffix=".md", delete=False)` and remove the file explicitly once its contents have been read.

## Notes

- `max_chars=20000` is a safe default; increase for longer documents if your LLM's context allows
- Eval mode limits extraction to the first few pages
- Document Filters supports 600+ formats: Word, PDF, Excel, PowerPoint, email, archives, and more
- For PDFs with tables, pass `"PDF_LAYOUT_DETECTION=on;PDF_TABLE_DETECTION=on"` as the options string to `doc.Open()` to improve table extraction fidelity; `PDF_TABLE_DETECTION` requires `PDF_LAYOUT_DETECTION=on` to be set first
- To cite page numbers in summaries, modify the `canvas_options` string in `get_document_markdown` to include `MARKDOWN_INCLUDE_LOCATIONS=on` (e.g., `canvas_options = "MARKDOWN_FLAVOR=GPT;MARKDOWN_INCLUDE_LOCATIONS=on"`) — DF injects `<!-- LOC: page, (l,t,r,b) -->` before each paragraph, letting the LLM reference exact source locations in its output
- The Markdown canvas path (`get_document_markdown`) requires a valid `DF_LICENSE_KEY` — canvas devices are not available in evaluation mode. Plain text extraction via `get_document_text` works in eval mode (first few pages only)
- For container formats (ZIP, PST, MBOX, EML), `get_document_markdown` returns empty output — these formats have no pages, so the canvas renders nothing. Use `get_document_text` for archives and email, or call `doc.getSupportsSubFiles()` to detect container formats and select the appropriate extraction path

## Reference

https://hyland.github.io/DocumentFilters-Docs/latest/
