# Document Filters: Extract Metadata

Inspect a document's file type, page count, and capability flags without extracting any content. Returns the detected format name, numeric type ID, page count, and whether the document supports text extraction or sub-file enumeration.

Based on: `skills/cli/src/document_filters_cli/main.py` (`cmd_extract_metadata`)

> **Agent note**: Output is small and safe to read directly into context. Use `--json` when the result will be consumed programmatically or passed to another tool.

## Prerequisites

`df` CLI must be installed and the runtime downloaded. If `df info` fails, install first:
```bash
pip install "git+https://github.com/Hyland/DocumentFilters.git@master#subdirectory=skills/cli"
df init
```

## Usage

```bash
# Human-readable key-value output (default)
df extract-metadata report.docx

# Machine-readable JSON
df extract-metadata report.docx --json

# Inspect a plain-text file
df extract-metadata sample.txt --json
```

### Example output (`--json`)

```json
{
  "path": "sample.txt",
  "file_type_id": 2,
  "file_type_name": "Text (ASCII)",
  "page_count": 1,
  "supports_text": true,
  "supports_subfiles": false
}
```

### Example output (default key-value)

```
path: sample.txt
file_type_id: 2
file_type_name: Text (ASCII)
page_count: 1
supports_text: True
supports_subfiles: False
```

## Flags

| Flag | Description |
|------|-------------|
| `--json` | Output as JSON instead of key-value pairs. |

## Output fields

| Field | Description |
|-------|-------------|
| `path` | Input file path as supplied. |
| `file_type_id` | Numeric Document Filters format ID for the detected file type. |
| `file_type_name` | Human-readable format name (e.g. `"PDF"`, `"Microsoft Word 2007+"`, `"Text (ASCII)"`). |
| `page_count` | Number of pages (or slides, sheets, messages, etc.) in the document. |
| `supports_text` | `true` if the format supports text extraction via `df extract-text`. |
| `supports_subfiles` | `true` if the format contains embedded sub-files (e.g. email archives, ZIP containers) accessible via `--subfiles`. |

## When to use extract-metadata

- **Before extraction**: confirm a file is the expected format and has the expected page count before running a longer operation.
- **Format detection**: identify what type of file you are dealing with when the extension is missing or unreliable.
- **Pipeline branching**: use `--json` to feed `supports_text` or `supports_subfiles` into a script that decides which `df` command to run next.

## License

With `DF_LICENSE_KEY` set, all 600+ formats are detected correctly. In evaluation mode (no key), format detection works for PDF, Office (DOCX, XLSX, PPTX), ODF, and plain text. HTML, email (PST, OST, MBOX, EML, MSG), and other licensed-only formats return **error 4015** without a key — the format cannot be inspected at all, not just extraction. Set `DF_LICENSE_KEY` to resolve it:

```bash
export DF_LICENSE_KEY="your-key-here"
df extract-metadata sample.eml --json
```
