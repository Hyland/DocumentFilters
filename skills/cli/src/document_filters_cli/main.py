"""
df — Document Filters CLI

Usage:
  df init [--ocr]
  df extract-text <input> [--output <file>] [--ocr] [--ocr-language <lang>] [--ocr-min-width <n>] [--subfiles] [--slide-notes]
  df convert-to-markdown <input> [--output <file>] [--pdf-tables] [--ocr-images] [--ocr-language <lang>] [--ocr-min-width <n>] [--flavor <flavor>] [--locations] [--metadata [--metadata-format YAML|TOML|JSON|COMMENTS]] [--no-headers] [--no-footers] [--normalize]
  df convert-to-pdf <input> [--output <file>] [--bookmarks]
  df convert-to-html <input> [--output <file>] [--no-inline-images]
  df convert-to-png <input> [--output-prefix <prefix>] [--dpi <n>]
  df extract-metadata <input> [--json]
  df extract-structure <input> [--output <file>] [--format <fmt>] [--words] [--locations]
  df redact <input> <pattern>... [--output <file>]
  df compare <left> <right> [--output-left <file>] [--output-right <file>] [--ignore-case]
  df info
"""

import argparse
import io
import json
import os
import re
import sys


def _api():
    from .runtime import make_api
    return make_api()


# ── Commands ──────────────────────────────────────────────────────────────────

def cmd_init(args):
    """Download the native runtime binaries to ~/.document-filters/"""
    import io
    import urllib.request
    import zipfile
    from .runtime import runtime_dir, DF_VERSION, RELEASES_BASE
    path = runtime_dir(quiet=False)
    print(f"Runtime ready: {path}")

    if getattr(args, "ocr", False):
        dat_path = os.path.join(path, "ISYSreadersocr.dat")
        if os.path.exists(dat_path):
            print(f"OCR data already installed: {dat_path}")
        else:
            url = f"{RELEASES_BASE}/assets.zip"
            print(f"Downloading OCR data from {url} ...")
            req = urllib.request.Request(url, headers={"User-Agent": "df-cli"})
            with urllib.request.urlopen(req) as resp:
                data = resp.read()
            installed = []
            with zipfile.ZipFile(io.BytesIO(data)) as zf:
                for name in zf.namelist():
                    if name.endswith(".dat"):
                        target = os.path.join(path, name.split("/")[-1])
                        with zf.open(name) as src, open(target, "wb") as dst:
                            dst.write(src.read())
                        installed.append(target)
            if installed:
                for t in installed:
                    print(f"Installed: {t}")
            else:
                print("Warning: no .dat files found in assets.zip")

    print(f"Document Filters {DF_VERSION} is installed and ready.")


def cmd_extract_text(args):
    from DocumentFilters import IGR_BODY_AND_META, IGR_FORMAT_LONG_NAME

    api = _api()
    output = args.output

    def _extract(doc, depth=0):
        parts = []
        if doc.getSupportsText():
            opt_parts = []
            if args.ocr:
                opt_parts.append("OCR=on;OCR_REORIENT_PAGES=on")
            if args.ocr_language:
                opt_parts.append(f"OCR_LANGUAGE={args.ocr_language}")
            if args.ocr_min_width is not None:
                opt_parts.append(f"OCR_MIN_WIDTH={args.ocr_min_width}")
            if args.slide_notes:
                opt_parts.append("SLIDE_NOTES=on")
            opts = ";".join(opt_parts)
            doc.Open(IGR_BODY_AND_META, opts)
            while not doc.getEOF():
                parts.append(doc.GetText(4096, stripControlCodes=True))
        if args.subfiles and doc.getSupportsSubFiles():
            for sub in doc.SubFiles:
                with sub:
                    parts.extend(_extract(sub, depth + 1))
        return parts

    with api.GetExtractor(args.input) as doc:
        text = "".join(_extract(doc))

    if output:
        with open(output, "w", encoding="utf-8") as f:
            f.write(text)
        print(f"Wrote {len(text)} characters to {output}")
    else:
        sys.stdout.buffer.write(text.encode("utf-8", errors="replace"))


def cmd_convert_to_markdown(args):
    from DocumentFilters import (
        IGR_BODY_AND_META, IGR_FORMAT_IMAGE, IGR_DEVICE_MARKDOWN
    )

    api = _api()
    output = args.output or (os.path.splitext(args.input)[0] + ".md")

    doc_opt_parts = []
    if args.pdf_tables:
        doc_opt_parts.append("PDF_LAYOUT_DETECTION=on;PDF_TABLE_DETECTION=on;PDF_LIST_DETECTION=on")
    if args.ocr_images:
        doc_opt_parts.append("OCR=on;OCR_REORIENT_PAGES=on;OCR_INLINE_IMAGES=on")
        # Office documents render embedded images at screen DPI (~96), making them
        # typically 400-600px wide. Lower the default threshold so they aren't skipped.
        min_width = args.ocr_min_width if args.ocr_min_width is not None else 100
        doc_opt_parts.append(f"OCR_MIN_WIDTH={min_width}")
    elif args.ocr_min_width is not None:
        doc_opt_parts.append(f"OCR_MIN_WIDTH={args.ocr_min_width}")
    if args.ocr_language:
        doc_opt_parts.append(f"OCR_LANGUAGE={args.ocr_language}")
    doc_opts = ";".join(doc_opt_parts)

    canvas_parts = [f"MARKDOWN_FLAVOR={args.flavor.upper()}"]
    if args.locations:
        canvas_parts.append("MARKDOWN_INCLUDE_LOCATIONS=on")
    if args.metadata:
        canvas_parts.append("MARKDOWN_INCLUDE_METADATA=on")
        canvas_parts.append(f"MARKDOWN_METADATA_FORMAT={args.metadata_format.upper()}")
    if args.no_headers:
        canvas_parts.append("MARKDOWN_INCLUDE_HEADERS=off")
    if args.no_footers:
        canvas_parts.append("MARKDOWN_INCLUDE_FOOTERS=off")
    if args.normalize:
        canvas_parts.append("MARKDOWN_CLEAN_CONTENT=normalize_quotes,normalize_dashes")
    canvas_opts = ";".join(canvas_parts)

    buf = io.BytesIO()
    with api.GetExtractor(args.input) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, doc_opts)
        with api.MakeOutputCanvas(buf, IGR_DEVICE_MARKDOWN, canvas_opts) as canvas:
            for page in doc.Pages:
                with page:
                    canvas.RenderPage(page)

    content = buf.getvalue().decode("utf-8")
    with open(output, "w", encoding="utf-8") as f:
        f.write(content)
    print(f"Wrote {output}")


def cmd_convert_to_pdf(args):
    from DocumentFilters import (
        IGR_BODY_AND_META, IGR_FORMAT_IMAGE, IGR_DEVICE_IMAGE_PDF
    )

    api = _api()
    output = args.output or (os.path.splitext(args.input)[0] + ".pdf")

    doc_opts = f"PDF_BOOKMARKS={'true' if args.bookmarks else 'false'}"

    with api.GetExtractor(args.input) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, doc_opts)
        with api.MakeOutputCanvas(output, IGR_DEVICE_IMAGE_PDF, "") as canvas:
            for page in doc.Pages:
                with page:
                    canvas.RenderPage(page)
    print(f"Wrote {output}")


def cmd_convert_to_html(args):
    from DocumentFilters import (
        IGR_BODY_AND_META, IGR_FORMAT_IMAGE, IGR_DEVICE_HTML
    )

    api = _api()
    output = args.output or (os.path.splitext(args.input)[0] + ".html")

    inline = "off" if args.no_inline_images else "on"
    canvas_opts = f"HTML_INLINE_IMAGES={inline}"

    with api.GetExtractor(args.input) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "")
        with api.MakeOutputCanvas(output, IGR_DEVICE_HTML, canvas_opts) as canvas:
            for page in doc.Pages:
                with page:
                    canvas.RenderPage(page)
    print(f"Wrote {output}")


def cmd_convert_to_png(args):
    from DocumentFilters import (
        IGR_BODY_AND_META, IGR_FORMAT_IMAGE, IGR_DEVICE_IMAGE_PNG
    )

    api = _api()
    prefix = args.output_prefix or os.path.splitext(args.input)[0]
    doc_opts = f"GRAPHIC_DPI={args.dpi}" if args.dpi else ""

    written = []
    with api.GetExtractor(args.input) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, doc_opts)
        for i, page in enumerate(doc.Pages, start=1):
            with page:
                out = f"{prefix}_page{i}.png"
                with api.MakeOutputCanvas(out, IGR_DEVICE_IMAGE_PNG, "") as canvas:
                    canvas.RenderPage(page)
                written.append(out)

    for f in written:
        print(f"Wrote {f}")
    print(f"Total: {len(written)} page(s)")


def cmd_extract_metadata(args):
    from DocumentFilters import IGR_BODY_AND_META, IGR_FORMAT_IMAGE, IGR_FORMAT_LONG_NAME

    api = _api()
    with api.GetExtractor(args.input) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "")
        result = {
            "path": args.input,
            "file_type_id": doc.getFileType(),
            "file_type_name": doc.getFileType(IGR_FORMAT_LONG_NAME),
            "page_count": doc.GetPageCount(),
            "supports_text": doc.getSupportsText(),
            "supports_subfiles": doc.getSupportsSubFiles(),
        }

    if args.json:
        print(json.dumps(result, indent=2))
    else:
        for k, v in result.items():
            print(f"{k}: {v}")


def cmd_extract_structure(args):
    from DocumentFilters import (
        IGR_BODY_AND_META, IGR_FORMAT_IMAGE, IGR_DEVICE_MARKDOWN, IGR_DEVICE_JSON
    )

    api = _api()
    fmt = args.format.lower()
    output = args.output or (os.path.splitext(args.input)[0] + ("." + fmt))

    if fmt == "markdown":
        canvas_parts = ["MARKDOWN_FLAVOR=GPT"]
        if args.locations:
            canvas_parts.append("MARKDOWN_INCLUDE_LOCATIONS=on")
        canvas_opts = ";".join(canvas_parts)

        buf = io.BytesIO()
        with api.GetExtractor(args.input) as doc:
            doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "")
            with api.MakeOutputCanvas(buf, IGR_DEVICE_MARKDOWN, canvas_opts) as canvas:
                for page in doc.Pages:
                    with page:
                        canvas.RenderPage(page)
        content = buf.getvalue().decode("utf-8")
        with open(output, "w", encoding="utf-8") as f:
            f.write(content)

    elif fmt == "json":
        canvas_opts = (
            f"JSON_OUTPUT_SCHEMA={args.json_schema.upper()};"
            "JSON_FORMAT_OUTPUT=true;"
            f"JSON_INCLUDE_WORDS={'true' if args.words else 'false'};"
            "JSON_SKIP_EMPTY_PARAGRAPHS=true"
        )
        with api.GetExtractor(args.input) as doc:
            doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "")
            with api.MakeOutputCanvas(output, IGR_DEVICE_JSON, canvas_opts) as canvas:
                for page in doc.Pages:
                    with page:
                        canvas.RenderPage(page)
    else:
        print(f"Unknown format: {fmt}. Use 'markdown' or 'json'.", file=sys.stderr)
        sys.exit(1)

    print(f"Wrote {output}")


def cmd_redact(args):
    from DocumentFilters import (
        IGR_BODY_AND_META, IGR_FORMAT_IMAGE, IGR_DEVICE_IMAGE_PDF
    )

    api = _api()
    output = args.output or (os.path.splitext(args.input)[0] + "_redacted.pdf")
    compiled = [re.compile(p, re.IGNORECASE) for p in args.pattern]

    with api.GetExtractor(args.input) as doc:
        doc.Open(IGR_BODY_AND_META | IGR_FORMAT_IMAGE, "")
        with api.MakeOutputCanvas(output, IGR_DEVICE_IMAGE_PDF, "") as canvas:
            for page in doc.Pages:
                with page:
                    canvas.RenderPage(page)
                    for i in range(page.WordCount):
                        word = page.GetWord(i)
                        for pattern in compiled:
                            if pattern.search(word.Text):
                                canvas.SetBrush(0x000000, 0)
                                canvas.SetPen(0, 0, 0)
                                canvas.Rect(
                                    int(word.X), int(word.Y),
                                    int(word.X + word.Width),
                                    int(word.Y + word.Height),
                                )
                                break
    print(f"Wrote {output}")


def cmd_compare(args):
    from DocumentFilters import (
        IGR_FORMAT_IMAGE, IGR_DEVICE_IMAGE_PDF,
        IGR_COMPARE_DOCUMENTS_COMPARE_WORDS,
        IGR_COMPARE_DOCUMENTS_FLAGS_NO_CASE,
        IGR_COMPARE_DOCUMENTS_DIFFERENCE_EQUAL,
        IGR_COMPARE_DOCUMENTS_DIFFERENCE_NEXT_BATCH,
        DocumentFilters,
    )

    api = _api()

    settings = DocumentFilters.CompareSettings()
    settings.CompareType = IGR_COMPARE_DOCUMENTS_COMPARE_WORDS
    settings.Flags = IGR_COMPARE_DOCUMENTS_FLAGS_NO_CASE if args.ignore_case else 0

    with api.GetExtractor(args.left) as left:
        left.Open(IGR_FORMAT_IMAGE, "")
        with api.GetExtractor(args.right) as right:
            right.Open(IGR_FORMAT_IMAGE, "")
            with left.Compare(otherDocument=right, compareSettings=settings) as results:
                diffs = []
                while results.MoveNext():
                    diff = results.Current
                    if diff.Type in (
                        IGR_COMPARE_DOCUMENTS_DIFFERENCE_EQUAL,
                        IGR_COMPARE_DOCUMENTS_DIFFERENCE_NEXT_BATCH,
                    ):
                        continue
                    diffs.append({
                        "type": str(diff.Type),
                        "originalPage": diff.OriginalPageIndex + 1,
                        "revisedPage": diff.RevisedPageIndex + 1,
                        "parts": [
                            {
                                "text": hit.Text,
                                "page": hit.PageIndex + 1,
                                "bounds": {
                                    "l": hit.Bounds.left, "t": hit.Bounds.top,
                                    "r": hit.Bounds.right, "b": hit.Bounds.bottom,
                                },
                            }
                            for hit in diff.Details
                        ],
                    })

    output = {"left": args.left, "right": args.right, "differences": len(diffs), "diffs": diffs}
    print(json.dumps(output, indent=2))

    if args.output_left:
        _render_diff_pdf(api, args.left, args.output_left)
    if args.output_right:
        _render_diff_pdf(api, args.right, args.output_right)


def _render_diff_pdf(api, path, output):
    from DocumentFilters import IGR_FORMAT_IMAGE, IGR_DEVICE_IMAGE_PDF
    with api.GetExtractor(path) as doc:
        doc.Open(IGR_FORMAT_IMAGE, "")
        with api.MakeOutputCanvas(output, IGR_DEVICE_IMAGE_PDF, "") as canvas:
            for page in doc.Pages:
                with page:
                    canvas.RenderPage(page)
    print(f"Wrote {output}", file=sys.stderr)


def cmd_info(args):
    from .runtime import DF_VERSION, CACHE_DIR, _artifact_name
    print(f"Document Filters CLI")
    print(f"  SDK version : {DF_VERSION}")
    print(f"  Platform    : {_artifact_name()}")
    print(f"  Cache dir   : {CACHE_DIR}")
    runtime = CACHE_DIR / _artifact_name()
    print(f"  Runtime     : {'installed' if runtime.exists() else 'not downloaded (run: df init)'}")
    key = os.environ.get("DF_LICENSE_KEY", "")
    print(f"  License key : {'set' if key else 'not set (evaluation mode)'}")


# ── Argument parser ────────────────────────────────────────────────────────────

def build_parser():
    parser = argparse.ArgumentParser(
        prog="df",
        description="Document Filters CLI — extract, convert, and process 600+ document formats",
    )
    sub = parser.add_subparsers(dest="command", required=True)

    # init
    p_init = sub.add_parser("init", help="Download native runtime binaries")
    p_init.add_argument("--ocr", action="store_true", help="Also download OCR data (ISYSreadersocr.dat) required for --ocr and --ocr-images")

    # info
    sub.add_parser("info", help="Show version, platform, and runtime status")

    # extract-text
    p = sub.add_parser("extract-text", help="Extract plain text from a document")
    p.add_argument("input", help="Input file path")
    p.add_argument("--output", "-o", help="Output file (default: stdout)")
    p.add_argument("--ocr", action="store_true", help="Enable OCR for fully scanned/image-only documents")
    p.add_argument("--ocr-language", metavar="LANG",
                   help="3-letter ISO 639-2 language code for OCR (e.g. eng, fra, deu, spa). Default: eng")
    p.add_argument("--ocr-min-width", type=int, metavar="N",
                   help="Minimum image width in pixels to attempt OCR (default: 1000). Lower to catch smaller pages.")
    p.add_argument("--subfiles", action="store_true", help="Recurse into archives and email attachments")
    p.add_argument("--slide-notes", action="store_true", help="Include PowerPoint speaker notes in output")

    # convert-to-markdown
    p = sub.add_parser("convert-to-markdown", help="Convert a document to Markdown")
    p.add_argument("input", help="Input file path")
    p.add_argument("--output", "-o", help="Output .md file (default: <input>.md)")
    p.add_argument("--pdf-tables", action="store_true", help="Enable PDF layout and table detection")
    p.add_argument("--flavor", default="GPT", choices=["GPT", "GFM"],
                   help="Markdown flavor: GPT (LLM-optimised) or GFM (default: GPT)")
    p.add_argument("--locations", action="store_true",
                   help="Embed <!-- LOC: page, (l,t,r,b) --> location comments")
    p.add_argument("--metadata", action="store_true", help="Include document metadata block")
    p.add_argument("--metadata-format", default="YAML",
                   choices=["YAML", "TOML", "JSON", "COMMENTS"],
                   help="Metadata block format (default: YAML)")
    p.add_argument("--ocr-images", action="store_true",
                   help="OCR images embedded within otherwise-text documents (e.g. scanned figures in a Word doc)")
    p.add_argument("--ocr-language", metavar="LANG",
                   help="3-letter ISO 639-2 language code for OCR (e.g. eng, fra, deu, spa). Default: eng")
    p.add_argument("--ocr-min-width", type=int, metavar="N",
                   help="Minimum image width in pixels to attempt OCR (default: 1000). Lower to catch smaller figures.")
    p.add_argument("--no-headers", action="store_true",
                   help="Exclude page headers from output (reduces noise in RAG pipelines)")
    p.add_argument("--no-footers", action="store_true",
                   help="Exclude page footers from output (reduces noise in RAG pipelines)")
    p.add_argument("--normalize", action="store_true",
                   help="Normalize smart quotes and em-dashes to ASCII equivalents")

    # convert-to-pdf
    p = sub.add_parser("convert-to-pdf", help="Convert a document to PDF")
    p.add_argument("input", help="Input file path")
    p.add_argument("--output", "-o", help="Output .pdf file (default: <input>.pdf)")
    p.add_argument("--bookmarks", action="store_true", help="Generate PDF bookmarks from headings")

    # convert-to-html
    p = sub.add_parser("convert-to-html", help="Convert a document to HD HTML5")
    p.add_argument("input", help="Input file path")
    p.add_argument("--output", "-o", help="Output .html file (default: <input>.html)")
    p.add_argument("--no-inline-images", action="store_true",
                   help="Write images as separate files instead of base64 inline")

    # convert-to-png
    p = sub.add_parser("convert-to-png", help="Render each page as a PNG image")
    p.add_argument("input", help="Input file path")
    p.add_argument("--output-prefix", help="Output filename prefix (default: <input stem>)")
    p.add_argument("--dpi", type=int, help="Rendering resolution in DPI (default: 96)")

    # extract-metadata
    p = sub.add_parser("extract-metadata", help="Show file type, page count, and properties")
    p.add_argument("input", help="Input file path")
    p.add_argument("--json", action="store_true", help="Output as JSON")

    # extract-structure
    p = sub.add_parser("extract-structure", help="Extract document structure as Markdown or JSON")
    p.add_argument("input", help="Input file path")
    p.add_argument("--output", "-o", help="Output file (default: <input>.<format>)")
    p.add_argument("--format", "-f", default="markdown", choices=["markdown", "json"],
                   help="Output format (default: markdown)")
    p.add_argument("--words", action="store_true", help="Include word-level bounding boxes (JSON only)")
    p.add_argument("--locations", action="store_true",
                   help="Embed location comments in Markdown output")
    p.add_argument("--json-schema", default="FULL", choices=["FULL", "MDAST", "PIPELINE"],
                   help="JSON output schema: FULL (default), MDAST (AST-compatible), PIPELINE (Hyland pipeline format)")

    # redact
    p = sub.add_parser("redact", help="Redact text matching patterns and output as PDF")
    p.add_argument("input", help="Input file path")
    p.add_argument("pattern", nargs="+", help="Regex pattern(s) to redact")
    p.add_argument("--output", "-o", help="Output .pdf file (default: <input>_redacted.pdf)")

    # compare
    p = sub.add_parser("compare", help="Compare two documents and report differences as JSON")
    p.add_argument("left", help="Original document")
    p.add_argument("right", help="Revised document")
    p.add_argument("--output-left", help="Write PDF rendering of the left document")
    p.add_argument("--output-right", help="Write PDF rendering of the right document")
    p.add_argument("--ignore-case", action="store_true", help="Case-insensitive comparison")

    return parser


def main():
    parser = build_parser()
    args = parser.parse_args()

    dispatch = {
        "init": cmd_init,
        "info": cmd_info,
        "extract-text": cmd_extract_text,
        "convert-to-markdown": cmd_convert_to_markdown,
        "convert-to-pdf": cmd_convert_to_pdf,
        "convert-to-html": cmd_convert_to_html,
        "convert-to-png": cmd_convert_to_png,
        "extract-metadata": cmd_extract_metadata,
        "extract-structure": cmd_extract_structure,
        "redact": cmd_redact,
        "compare": cmd_compare,
    }

    try:
        dispatch[args.command](args)
    except KeyboardInterrupt:
        sys.exit(130)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
