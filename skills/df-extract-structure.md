# Document Filters: Extract Structure

Use this command to pull out a document's structure — headings, paragraphs, tables — either as readable Markdown or as machine-readable JSON for programmatic processing.

Extract the structural content of a document as Markdown or JSON. Unlike `df convert-to-markdown`, this command is explicitly oriented around document structure — headings, paragraphs, tables, lists — rather than visual fidelity. Markdown output uses GPT flavor (same underlying canvas device as `convert-to-markdown --flavor GPT`). JSON output produces a nested `kids` tree (page -> section -> column -> paragraph nodes) with optional word-level bounding boxes.

Based on: `skills/cli/src/document_filters_cli/main.py` (`cmd_extract_structure`)

> **Agent note**: Always pass `--output` to write to a file. The command writes `Wrote <path>` to stdout and nothing else — the structured content goes to the output file. Do NOT read JSON output back into context — JSON output for any multi-page document can be many megabytes, especially with --words. Always pass --output and report only the file path.

## Common uses
- Pull the chapter/heading outline from a Word document or PDF for a table of contents
- Extract all tables from a report into structured data
- Get a clean, LLM-ready version of a document's content for summarization or Q&A
- Build a search index that maps each paragraph back to its exact page and location in the source file (use --format json --words)
- Pull the outline of a legal contract or policy document to quickly find the sections you need
- Get all table data from a financial report as structured JSON for further analysis in Excel or Python

## Prerequisites

`df` CLI must be installed and the runtime downloaded. If `df info` fails, install first:
```bash
pip install "git+https://github.com/Hyland/DocumentFilters.git@master#subdirectory=skills/cli"
df init
```

## Usage

```bash
# Extract document structure as Markdown (default)
df extract-structure report.docx -o report.md

# Extract structure from every PDF in a folder (shell loop)
for f in *.pdf; do
  df extract-structure "$f" --format json --words --output "${f%.pdf}.json"
done

# Embed source-location comments for traceability (default format is markdown)
df extract-structure report.docx --locations --output report.md

# Extract structure as JSON (paragraph/word tree)
df extract-structure report.docx --format json --output report.json

# Extract JSON including word-level bounding boxes (for layout analysis)
df extract-structure report.docx --format json --words --output report.json

# Extract structure from an email (.eml) as Markdown
df extract-structure message.eml --output message.md

# Extract structure from HTML as JSON with word positions
df extract-structure page.html --format json --words --output page.json

# MDAST schema — native input for unified/remark/MDX JS toolchains
df extract-structure report.pdf --format json --json-schema MDAST --output report.mdast.json

# PIPELINE schema — produces [{type, text, metadata}] array, direct input for
# LangChain Document objects and Unstructured.io compatible loaders
df extract-structure report.pdf --format json --json-schema PIPELINE --output report_pipeline.json
```

## Flags

| Flag | Description |
|------|-------------|
| `--output / -o` | Output file path. Default: `<input-stem>.markdown` or `<input-stem>.json` depending on format (e.g. `report.markdown` or `report.json`). Note: the default `.markdown` extension is non-standard — pass `--output report.md` explicitly to get a standard `.md` file. |
| `--format / -f` | Output format: `markdown` (default) or `json`. |
| `--words` | Include word-level bounding boxes in JSON output. No effect in Markdown mode. |
| `--locations` | Embed location comments (page/paragraph references) in Markdown output. No effect in JSON mode — use `--words` instead for coordinate-level attribution in JSON. |
| `--json-schema` | JSON output schema: `FULL` (default, Document Filters native), `MDAST` (Markdown AST — compatible with unified/remark/MDX JS toolchain), `PIPELINE` (array of `{type, text, metadata}` — direct input for LangChain `Document` objects and Unstructured.io loaders). Only applies to `--format json`. |

## Output formats

**Markdown** (`--format markdown`, default): Uses the `IGR_DEVICE_MARKDOWN` canvas with `MARKDOWN_FLAVOR=GPT`. The output is structurally identical to `df convert-to-markdown --flavor GPT`. Headings, paragraphs, tables, and lists are preserved. Use `--locations` to embed source-location comments for traceability. When --locations is set, each paragraph is preceded by an HTML comment of the form `<!-- LOC: <page>, (<left>,<top>,<right>,<bottom>) -->` (1-based page number, coordinates in document units). These comments are safe to include in LLM context; most models ignore HTML comments.

**Heading output by format:**
- **DOCX / PPTX**: Standard `# Heading` markers (not escaped) — headings render correctly in all Markdown renderers and LLM contexts.
- **PDF**: Headings are rendered as `**bold**` text, not `#` markers — PDFs have no heading semantics in their text stream. Tables in PDFs may also appear as unstructured paragraph text; use `df convert-to-markdown --pdf-tables` for PDF table extraction.
- **Plain-text `.txt`**: Both `#` and `\#` characters are escaped as `\#` — plain text has no heading semantics, so the converter cannot distinguish a heading from a literal `#` character. This is by design.

If your pipeline requires standard heading markers from `.txt` input, post-process to strip the backslash:
```python
import re
cleaned = re.sub(r'^\\(#+)', r'\1', content, flags=re.MULTILINE)
```

**JSON** (`--format json`): Uses the `IGR_DEVICE_JSON` canvas. The default `--json-schema FULL` output is a nested tree with top-level keys `metadata` and `pages`. Each page object has `type`, `bounds`, `index`, and a `kids` array of intermediate structural nodes — not a flat paragraph list. Node types observed in practice: `page`, `section`, `column`, `footer`, `p` (paragraph), `l` (line), `r` (run). Leaf paragraph nodes have type `"p"` and carry a `styles` object of font and layout properties (e.g. `fontSize`, `fontName`, `bold`). There is no top-level semantic style name string (like `"Heading1"`) on paragraph nodes. Empty paragraphs are skipped. Add `--words` to include per-word bounding boxes; `word` nodes do not appear without `--words`. Word entries use a nested `bounds` object (`left`, `top`, `width`, `height`) and an explicit `type: "word"` field. Each page object includes dimensions (e.g. `816x1056` for letter-size). The top-level `metadata` array is empty for formats with no embedded document properties (HTML, plain text).

Actual JSON structure (representative skeleton):
```json
{
  "pages": [
    {
      "type": "page",
      "bounds": { "left": 0, "top": 0, "width": 816, "height": 1056 },
      "kids": [
        {
          "type": "section",
          "kids": [
            {
              "type": "column",
              "kids": [
                {
                  "type": "p",
                  "styles": { "fontSize": 18, "fontName": "Arial", "bold": 1 },
                  "kids": [
                    {
                      "type": "l",
                      "kids": [
                        {
                          "type": "r",
                          "styles": { "fontSize": 18, "fontName": "Arial", "bold": 1 },
                          "kids": [
                            // word nodes only present when --words is passed
                            { "type": "word", "text": "Introduction", "bounds": { "left": 72, "top": 96, "width": 120, "height": 18 } }
                          ]
                        }
                      ]
                    }
                  ]
                }
              ]
            }
          ]
        }
      ]
    }
  ],
  "metadata": []
}
```
To confirm the exact schema for your document type, run `df extract-structure sample.pdf --format json --output sample.json` and inspect the output directly.

## When to use extract-structure vs convert-to-markdown

- For most RAG or summarization use cases with Office documents, `df convert-to-markdown --flavor GPT` and `df extract-structure --format markdown` produce equivalent Markdown output. For PDFs with tables or columns, use `df convert-to-markdown --pdf-tables` instead — `df extract-structure` does not enable layout or table detection, so tables in PDFs may appear as unstructured paragraph text or be partially lost.
- Use `df extract-structure --format json` when you need a machine-readable structural representation (paragraph styles, word positions, bounding boxes) rather than human-readable Markdown.
- Use `--locations` in Markdown mode when you need to trace output back to specific pages or paragraphs in the source document.
- Use `--format json --words` when building retrieval-augmented pipelines that need source attribution — each word entry includes a `bounds` object with `left`, `top`, `width`, `height` coordinates, so you can highlight the exact source region in the original document when a chunk is retrieved.

## License

With `DF_LICENSE_KEY` set, all 600+ formats are available.

**Markdown format** (`--format markdown`) works in evaluation mode for PDF, Office (DOCX, PPTX, XLSX), ODF, and plain text with clean output — no watermark text is injected. HTML and email formats require a license key and return error 4015 without one.

**JSON format** (`--format json`, all `--json-schema` values) requires a license key. Without one, the CLI returns `error 14` (which means "feature requires license"). Set `DF_LICENSE_KEY` to resolve:

```bash
export DF_LICENSE_KEY="your-key-here"
df extract-structure report.pdf --format json --output report.json
```

Run `df info` to check your license status before running JSON extractions.
