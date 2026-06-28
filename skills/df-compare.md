# Document Filters: Compare Documents

Compare two document versions word-by-word and report insertions, deletions, and formatting changes as JSON. Useful for contract review, policy change tracking, and regulatory document management.

Based on: `samples/python/CompareDocuments.py`

## Prerequisites

Run `df info` to check both installation and license status. If the command is not found, install with:
```bash
pip install "git+https://github.com/Hyland/DocumentFilters.git@master#subdirectory=skills/cli"
df init
```

If `License key: not set` appears, export `DF_LICENSE_KEY="your-key-here"` before comparing — without it, DOCX/PDF/TXT work normally in evaluation mode. HTML, EML/MSG, and other licensed-only formats return error 4015 without a key.

## Usage

```bash
# Write diff to a file — output can be large for heavily-modified documents
df compare original.docx revised.docx > diff.json

# Case-insensitive comparison
df compare original.docx revised.docx --ignore-case > diff.json

# Also render the original documents as PDFs for side-by-side review alongside the diff JSON
df compare original.docx revised.docx \
  --output-left  original_rendered.pdf \
  --output-right revised_rendered.pdf \
  > diff.json
```

**Always redirect stdout (`> diff.json`) when using `--output-left` or `--output-right`.**  
CRITICAL: When either rendering flag is present, the CLI appends `Wrote <path>` status lines to stdout **after** the JSON block. Without the redirect, stdout is a mix of JSON and status text — `JSON.parse()` and any downstream consumer will fail. The redirect is not optional; omitting it produces broken output even when the PDFs are written successfully.

Renders each document as a plain PDF (no visual diff markup — the diff data is in diff.json, not drawn on the pages).

### Comparison flags (Python SDK)

The `--ignore-case` flag is the only comparison option exposed by the CLI. For additional filtering, use the Python SDK directly via `DocumentFilters.CompareSettings.Flags` with the `IGR_COMPARE_DOCUMENTS_FLAGS_*` constants:

| Constant | Effect |
|---|---|
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_CASE` | Ignore letter case differences |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_WHITESPACE` | Ignore whitespace-only differences |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_PUNCTUATION` | Ignore punctuation differences |
| `IGR_COMPARE_DOCUMENTS_FLAGS_FORMATTING` | Include formatting changes in output |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_COMMENTS` | Ignore comment annotations |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_TABLES` | Ignore changes inside tables |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS` | Ignore header content |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTERS` | Ignore footer content |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTNOTES` | Ignore footnote content |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_TEXTBOXES` | Ignore text box content |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_FIELDS` | Ignore field codes and values |
| `IGR_COMPARE_DOCUMENTS_FLAGS_MOVES` | Detect moved blocks instead of reporting them as delete+insert |
| `IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS_FOOTERS` | Convenience: ignore both headers and footers |
| `IGR_COMPARE_DOCUMENTS_FLAGS_EQUALS` | Include equal (unchanged) regions in the output — useful for full-document structured diffing with a downstream parser |

Example: to compare a contract while ignoring headers, footers, and whitespace normalization, set `CompareSettings.Flags` to the bitwise OR of `IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS | IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTERS | IGR_COMPARE_DOCUMENTS_FLAGS_NO_WHITESPACE` before calling `CompareDocuments()`.

## Typical workflows

**Contract redline review:** `df compare contract_v1.docx contract_v2.docx > diff.json` — then ask your AI agent to summarise the insertions and deletions by clause.

**Policy update audit:** `df compare policy_2024.pdf policy_2025.pdf --ignore-case > diff.json` — use `--ignore-case` when reformatting may have changed capitalisation without changing meaning.

**Everyday document comparison:** `df compare lease_jan.pdf lease_feb.pdf > diff.json` — then ask your AI assistant: "Summarise what changed in plain English." It will describe each insertion and deletion by page without requiring you to read the JSON.

**Email compliance check:** `df compare message_original.eml message_forwarded.eml > diff.json` — detects whether a forwarded email has been altered from the original, useful for legal hold and compliance review.

## Output format

```json
{
  "left": "original.docx",
  "right": "revised.docx",
  "differences": 3,
  "diffs": [
    {
      "type": "2",
      "originalPage": 1,
      "revisedPage": 1,
      "parts": [{ "text": "old clause", "page": 1, "bounds": { "l": 72, "t": 144, "r": 540, "b": 162 } }]
    },
    {
      "type": "1",
      "originalPage": 1,
      "revisedPage": 1,
      "parts": [{ "text": "new clause", "page": 1, "bounds": { "l": 72, "t": 144, "r": 540, "b": 162 } }]
    }
  ]
}
```

Diff type values: `"0"` = equal, `"1"` = insert, `"2"` = delete, `"3"` = formatting change — the text content did not change but its style (bold, italic, font size, spacing) did. The `parts[]` array contains the affected words so you can show the user which text changed visually. Join `parts[].text` to reconstruct the phrase. Each diff includes word-level bounding boxes for every changed token.

Note: `type` values are JSON strings (e.g. `"1"`), not integers. Compare with `diff["type"] == "1"` not `== 1`.

Coordinates are in typographic points (1/72 inch), measured from the top-left corner of the page. Use `originalPage`/`revisedPage` together with `bounds` to map a change back to a specific source paragraph or RAG chunk. For word-level granularity within a chunk, use `df extract-structure --words` on the same document and join on page + bounding-box overlap.

The CLI filters out type `"0"` (equal) and type `"255"` (internal batch marker) — only insertions, deletions, and formatting changes appear in the output.

## Summarising for non-technical users

After writing diff.json, read it and report a plain-language summary:

```
Found N changes between original.docx and revised.docx:
- Page 1: "old clause text" was removed
- Page 2: "new clause text" was inserted
- Page 3: formatting changed on "section heading"
(+ N more changes)
```

Use the `parts[].text` tokens joined by spaces to reconstruct the changed phrase. Type `"1"` = added content, `"2"` = removed content, `"3"` = formatting only.

## RAG pipeline integration

The diff JSON maps each changed word to its page and bounding box, letting you update only the affected chunks in a vector store rather than re-indexing the whole document. Workflow: (1) run `df extract-structure --words --format json` on both versions to get chunk boundaries; (2) run `df compare` to get the diff; (3) join on page + bounding-box overlap to find which chunks contain changes; (4) re-embed only those chunks. This is significantly cheaper than full re-indexing for large documents with minor revisions.

## Supported formats

Any renderable format supported by Document Filters — DOCX, DOC, PDF, XLSX, PPTX, RTF, ODT, HTML, EML/MSG, TXT, and hundreds more with a license key. Formats without text content — plain raster images (TIFF, BMP, PNG with no embedded text), spreadsheet-only formats (CSV), and archives (ZIP) — will produce an empty or near-empty diff because there is no text layout to compare. For scanned image documents, run OCR first with `df extract-text --ocr`, or use `df compare` on a PDF/DOCX version of the scan.
