# Document Filters: Convert to PNG

Each output PNG is a standard image file (like a photo) that can be opened in any image viewer, attached to an email, embedded in a web page, or passed to a vision AI model. Convert any document — PDF, Word, PowerPoint, Excel, email (.eml/.msg), HTML, and 600+ other formats — into a set of PNG image files — one image per page. Use this to convert a contract or report to images you can attach to a Slack message or email, create page thumbnails, generate previews for a web gallery, feed document pages to a vision AI model, or visually inspect a redacted document.

> **Agent note**: Do NOT read, display, or base64-encode the output PNGs. Report only the file paths, total page count, and approximate file sizes. PNG files range from ~75 KB (96 DPI thumbnail) to ~3 MB (300 DPI print quality); loading even one into context wastes tokens and may exceed limits.

## Prerequisites

`df` CLI must be installed and the runtime downloaded. If `df info` fails, install first:
```bash
pip install "git+https://github.com/Hyland/DocumentFilters.git@master#subdirectory=skills/cli"
df init
```

## Usage

```bash
# Render all pages of a PDF at default resolution
df convert-to-png report.pdf

# Specify output prefix (files will be renders/slide_page1.png, renders/slide_page2.png, ...)
df convert-to-png slides.pptx --output-prefix renders/slide
# Note: the output directory must already exist — create it first with `mkdir -p renders` (bash)
# or `mkdir renders` (Windows cmd) before running the command.

# Render at 150 DPI for vision-model input
df convert-to-png report.pdf --dpi 150

# Convert a Word document to page images
df convert-to-png contract.docx

# Render an email as images (useful for visual review or passing to a vision model)
df convert-to-png email-thread.eml --dpi 150

# Batch processing — render all PDFs in a directory
for f in *.pdf; do df convert-to-png "$f" --dpi 150 --output-prefix "renders/$(basename "$f" .pdf)"; done
```

Note: the `for` loop above requires bash (Git Bash on Windows). In PowerShell, use: `Get-ChildItem *.pdf | ForEach-Object { df convert-to-png $_.Name --dpi 150 --output-prefix "renders/$($_.BaseName)" }`

## Output filename pattern

Files are named `<prefix>_page1.png`, `<prefix>_page2.png`, etc. The default prefix is the input filename without its extension (e.g. `report.pdf` → `report_page1.png`, `report_page2.png`).

Example: `df convert-to-png slides.pptx --output-prefix ./renders/slide` produces `./renders/slide_page1.png`, `./renders/slide_page2.png`, etc.

> **Tip**: Avoid ending your prefix with `page` (or any word that duplicates the `_page` suffix). For example, `--output-prefix html_page` produces `html_page_page1.png`. Use a shorter prefix such as `html` or `html_thumb` instead.

## DPI guidance

| DPI | Use case | Approximate file size |
|-----|----------|-----------------------|
| 96 | Screen thumbnail, smallest file | ~75–300 KB |
| 150 | Readable on screen, good for vision-model input | ~300–600 KB |
| 300 | Print quality, suitable for downstream OCR | ~1–3 MB per page |

Omit --dpi entirely to use the 96 DPI engine default.

Higher DPI = sharper image but larger file. AI engineers feeding pages to vision models should use at least 150 DPI. 96 DPI is too blurry for OCR pre-processing or visual QA.

## AI / Vision pipeline use cases

0. **Quick visual sharing**: render any document at default DPI and attach the resulting PNG files to an email, Slack message, or presentation. No programming required — just run the command and use the output files.
1. **Feed pages to a vision LLM**: render at 150 DPI, pass file paths to the model (GPT-4o, Claude Sonnet, Gemini Vision, etc.) — do not load the PNG bytes into agent context.
2. **Visual QA on redaction**: render the redacted PDF and visually confirm black boxes appear correctly over sensitive content.
3. **Scanned document fallback**: if `df extract-text` returns blank output, render pages at 150–300 DPI and pass to a vision model for OCR-in-context.
4. **Grounded chunk attribution for RAG**: combine `df convert-to-png report.pdf --dpi 150` with `df extract-structure report.pdf --format json --words` to get both a page image and per-word bounding boxes. Pass the image to a vision model and use the JSON bounding boxes to attribute the model's answer back to a precise page region. Always write the JSON to a file (`--output report.json`) and reference it by path; do not read the JSON into agent context on multi-page documents — it can be several megabytes.

## Limitations

There is no --pages flag — the command always renders every page. To get a single page image, render all pages and use the numbered output file (e.g. `report_page3.png`) for the page you need.

For large documents at high DPI, output can be substantial — a 100-page document at 300 DPI produces roughly 100–300 MB of PNG files. Consider using 96 or 150 DPI for pipeline processing, or pre-checking page count with `df extract-metadata <file>` before rendering.

## License

With `DF_LICENSE_KEY` set, all 600+ formats are unlocked and no evaluation watermark is applied. If DF_LICENSE_KEY is present in the environment, licensed mode is active automatically — no additional configuration is needed. Run `df info` to confirm: it will print "License key : set". In evaluation mode (no key), PDF, Office, ODF, and plain text render normally — no watermark is pixel-baked into the PNG for these formats. HTML, email, and CAD formats require a license and return error 4015.
