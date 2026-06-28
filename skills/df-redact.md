# Document Filters: Redact Document

Redact text matching regex patterns and output a burned-in PDF — every page is rendered as a picture, so the hidden text underneath cannot be copied, searched, or recovered. Both the visible text and the underlying content stream are removed — the result cannot be un-redacted.

Based on: `samples/csharp/RedactionDemo`

**When to use this**: removing personal information (names, IDs, phone numbers) before sharing a document externally; preparing contracts or medical records for review by stripping confidential fields; sanitizing documents before they are indexed into a search system or AI pipeline.

## Prerequisites

`df` CLI must be installed and the runtime downloaded. If `df info` fails, install first:
```bash
pip install "git+https://github.com/Hyland/DocumentFilters.git@master#subdirectory=skills/cli"
df init
```

## Usage

```bash
# Redact by pattern (output: <stem>_redacted.pdf)
df redact contract.pdf "\b\d{3}-\d{2}-\d{4}\b"

# Multiple patterns at once
df redact contract.pdf "\b\d{3}-\d{2}-\d{4}\b" "\b[\w._%+-]+@[\w.-]+\.[a-zA-Z]{2,}\b"

# Specify output file (--output and -o are equivalent)
df redact contract.pdf "Confidential" -o clean.pdf

# Input can be any supported format; output is always a PDF
df redact report.docx "\bProject\s+Alpha\b" --output report_redacted.pdf
```

All pages in the document are processed — there is no option to limit redaction to specific pages via the CLI.

## Real-world examples

- **Sharing a medical intake form with a billing vendor**: redact patient SSN (`\b\d{3}-\d{2}-\d{4}\b`) and phone number (`\b\d{3}[-.]?\d{3}[-.]?\d{4}\b`) before sending.
- **Sending a contract draft to an external reviewer**: redact employee names and salary figures by pattern before sharing the PDF.
- **Preparing support tickets for a public bug report**: redact customer email addresses (`\b[\w._%+-]+@[\w.-]+\.[a-zA-Z]{2,}\b`) before publishing.

## Common PII patterns

| Data | Pattern |
|------|---------|
| SSN | `\b\d{3}-\d{2}-\d{4}\b` |
| Email | `\b[\w._%+-]+@[\w.-]+\.[a-zA-Z]{2,}\b` |
| US phone | `\b\d{3}[-.]?\d{3}[-.]?\d{4}\b` |
| Credit card | `\b\d{4}[-]?\d{4}[-]?\d{4}[-]?\d{4}\b` |

> **Credit card note**: the pattern above matches compact (`4111-1111-1111-1111`) and unformatted (`4111111111111111`) forms. Space-separated card numbers (`4111 1111 1111 1111`) cannot be matched because the engine tokenizes text by whitespace — patterns that span a space boundary will never match because each whitespace-separated token is matched individually. Patterns with internal punctuation (dashes, dots) work fine because the token includes punctuation. If your documents contain space-separated card numbers, extract the text first (`df extract-text`), apply a full-line regex in your own script to find the positions, then use the Python SDK directly to redact by bounding box.

> **SSN pattern note**: the SSN pattern above matches the number format (e.g. `123-45-6789`). To redact the label "SSN" instead, use `\bSSN\b`.

## Important notes

- The engine tokenizes text by whitespace — patterns that span a space boundary will never match because each whitespace-separated token is matched individually. Patterns with internal punctuation (dashes, dots) work fine because the token includes punctuation. To redact a full name like `John Smith`, use two separate patterns: `John` and `Smith`, or use `\bJohn\b` and `\bSmith\b` to avoid partial matches.
- Pattern matching is always case-insensitive — `Confidential`, `CONFIDENTIAL`, and `confidential` all match the pattern `confidential`.
- Preview before redacting (redaction is permanent): `df extract-text contract.pdf --output df_preview.txt` then search `df_preview.txt` for your pattern to confirm matches. Never pipe extract-text output directly into context on large documents. (On Windows, `/tmp/` does not exist by default — write to the current directory instead, as shown here.)
- Redaction is permanent and irreversible.

## Supported input formats

With `DF_LICENSE_KEY` set, all 600+ input formats are supported with no watermarks. In evaluation mode (no key), PDF, plain-text, and Office formats (DOCX, XLSX, PPTX) are available and produce clean PDF output with no watermark text. HTML, email, and CAD formats require a license and return error 4015.

## AI / RAG pipeline notes

- **Pre-chunk redaction**: run `df redact` on the source file to produce a rasterized PDF for human review or archiving. For indexing into a vector store, extract text from the original source file using `df extract-text` or `df convert-to-markdown`, then strip the PII from the extracted text in your pipeline before embedding. Do not extract text from the redacted PDF — it is a rasterized image and will yield no text without OCR.
- **Batch processing**: `for f in docs/*; do df redact "$f" "\b\d{3}-\d{2}-\d{4}\b"; done`  # processes all 600+ formats; non-document files are skipped with an error message
- **Match count**: the CLI outputs only `Wrote <path>` — it does not report how many matches were redacted. To get a count before redacting, run `df extract-text` and count pattern occurrences with `grep -oP` or Python `re.findall` — `grep -c` counts matching lines, not total occurrences, so use `grep -o` for an accurate redaction count.
- **Redaction manifest**: the CLI does not emit a JSON report of redacted positions. If you need bounding-box records for audit trails, use the Python SDK directly (`page.GetWord(i)` exposes `.X`, `.Y`, `.Width`, `.Height`, `.Text`). The example below collects coordinates only — it does not produce a redacted file. Bounding-box-driven redaction requires the lower-level SDK (`IGR_Render_Page_Redactions`) beyond what the CLI exposes; see the SDK reference for `DocumentFiltersAPI.py`.

**Audit manifest only — this does not produce a redacted file:**

```python
import re, json
from DocumentFilters import DocumentFilters, IGR_BODY_AND_META, IGR_FORMAT_IMAGE
api = DocumentFilters()
pattern = re.compile(r'\b\d{3}-\d{2}-\d{4}\b', re.IGNORECASE)
manifest = []
with api.GetExtractor('contract.pdf') as doc:
    doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, '')
    for page in doc.Pages:
        with page:
            for i in range(page.WordCount):
                w = page.GetWord(i)
                if pattern.search(w.Text):
                    manifest.append({'text': w.Text, 'x': w.X, 'y': w.Y, 'w': w.Width, 'h': w.Height})
print(json.dumps(manifest, indent=2))
```

To act on these coordinates and produce a redacted output, pass the bounding boxes to `IGR_Render_Page_Redactions` via the lower-level SDK API exposed in `DocumentFiltersAPI.py`.
