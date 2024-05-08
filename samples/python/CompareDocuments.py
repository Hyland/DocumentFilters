#
# (c) 2024 Hyland Software, Inc. and its affiliates. All rights reserved.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
# ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
# ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#

#****************************************************************************
#* Document Filters Example - Compare Documents
#****************************************************************************/

import json, os, sys, argparse, DocumentFiltersSample
import shutil
from typing import Tuple
from DocumentFilters import *

def parse_pages(pages: str) -> Tuple[int, int]:
    if not pages:
        return (0, 0xffff)

    parts = pages.split('-')
    if len(parts) == 1 and parts[0].isdigit():
        page = int(parts[0])
        return (page, page)

    if len(parts) != 2:
        raise ValueError("Pages must be in the form of FIRST-LAST")

    first = int(parts[0]) if parts[0].isdigit() else 0
    last = int(parts[1]) if parts[1].isdigit() else 0xffff

    return (first, last)

def unpack_measurement(measurement: str) -> float:
    if not measurement:
        return 0

    if measurement.endswith("in"):
        return float(measurement[:-2]) * 72

    if measurement.endswith("cm"):
        return float(measurement[:-2]) * 28.35

    if measurement.endswith("mm"):
        return float(measurement[:-2]) * 2.835

    if measurement.endswith("pt"):
        return float(measurement[:-2])

    if measurement.endswith("px"):
        return float(measurement[:-2]) * 96 / 72

    return float(measurement)

def pack_rect(left, top, right, bottom) -> IGR_FRect:
    res = IGR_FRect()
    res.left = left
    res.top = top
    res.right = right
    res.bottom = bottom
    return res

def ensure_page(page_index: int, doc: DocumentFilters.Extractor, canvas: DocumentFilters.Canvas, current_page: int, is_left: bool):
    while current_page < page_index:
        with doc.GetPage(current_page + 1) as page:
            canvas.RenderPage(page)
        current_page += 1
    canvas.SetBrush(0x50ff0000 if is_left else 0x5000ff00, 1)
    canvas.SetPen(0, 0, 0)
    return current_page


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("LeftFile", help="Left file")
    parser.add_argument("RightFile", help="Right file")
    parser.add_argument("--ignore-case", action="store_true", help="Case insensitive comparison")
    parser.add_argument("--ignore-fields", action="store_true", help="Exclude fields from comparison")
    parser.add_argument("--ignore-footers", action="store_true", help="Exclude marked footers from comparison")
    parser.add_argument("--ignore-headers", action="store_true", help="Exclude marked headers from comparison")
    parser.add_argument("--ignore-punctuation", action="store_true", help="Exclude punctuation from comparison")
    parser.add_argument("--ignore-tables", action="store_true", help="Exclude tables from comparison")
    parser.add_argument("--ignore-textboxes", action="store_true", help="Exclude textboxes from comparison")
    parser.add_argument("--include-formatting", action="store_true", help="Include formatting in comparison")
    parser.add_argument("--left-options", help="Option string that applies to the first document")
    parser.add_argument("--left-output", help="Filename where marked up left output should be saved")
    parser.add_argument("--left-top-margin", help="The size of the top margin that should be excluded")
    parser.add_argument("--left-bottom-margin", help="The size of the bottom margin that should be excluded")
    parser.add_argument("--options", help="Option string that applies to both documents")
    parser.add_argument("--pages", help="Pages to compare in the form of FIRST-LAST")
    parser.add_argument("--right-options", help="Option string that applies to the second document")
    parser.add_argument("--right-output", help="Filename where marked up right output should be saved")
    parser.add_argument("--right-top-margin", help="The size of the top margin that should be excluded")
    parser.add_argument("--right-bottom-margin", help="The size of the bottom margin that should be excluded")
    parser.add_argument("--silent", action="store_true", help="Disable writing to console")
    parser.add_argument('-l', '--library', dest='library_path', action='store', help='Path to the Document Filters libraries')
    parser.add_argument('--license-key', dest='license_key', action='store', help='License key for Document Filters')
    args = parser.parse_args()


    api = Api()
    DocumentFiltersSample.InitializeAPI(api, args)

    if not args.LeftFile:
        print("Left file is required")
        return 1

    if not args.RightFile:
        print("Right file is required")
        return 1

    compare_settings = DocumentFilters.CompareSettings()
    compare_settings.CompareType = IGR_COMPARE_DOCUMENTS_COMPARE_WORDS
    compare_settings.Flags = 0
    if args.ignore_case:
        compare_settings.Flags |= IGR_COMPARE_DOCUMENTS_FLAGS_NO_CASE
    if args.ignore_fields:
        compare_settings.Flags |= IGR_COMPARE_DOCUMENTS_FLAGS_NO_FIELDS
    if args.ignore_footers:
        compare_settings.Flags |= IGR_COMPARE_DOCUMENTS_FLAGS_NO_FOOTERS
    if args.ignore_headers:
        compare_settings.Flags |= IGR_COMPARE_DOCUMENTS_FLAGS_NO_HEADERS
    if args.ignore_punctuation:
        compare_settings.Flags |= IGR_COMPARE_DOCUMENTS_FLAGS_NO_PUNCTUATION
    if args.ignore_tables:
        compare_settings.Flags |= IGR_COMPARE_DOCUMENTS_FLAGS_NO_TABLES
    if args.ignore_textboxes:
        compare_settings.Flags |= IGR_COMPARE_DOCUMENTS_FLAGS_NO_TEXTBOXES
    if args.include_formatting:
        compare_settings.Flags |= IGR_COMPARE_DOCUMENTS_FLAGS_FORMATTING

    pages = parse_pages(args.pages)
    left_page_settings = DocumentFilters.CompareDocumentSettings()
    left_page_settings.FirstPage = pages[0]
    left_page_settings.PageCount = pages[1] - pages[0] + 1
    left_page_settings.Margins = pack_rect(0, unpack_measurement(args.left_top_margin), 0, unpack_measurement(args.left_bottom_margin))

    right_page_settings = DocumentFilters.CompareDocumentSettings()
    right_page_settings.FirstPage = pages[0]
    right_page_settings.PageCount = pages[1] - pages[0] + 1
    right_page_settings.Margins = pack_rect(0, unpack_measurement(args.right_top_margin), 0, unpack_measurement(args.right_bottom_margin))

    with api.OpenExtractor(args.LeftFile, mode = IGR_FORMAT_IMAGE, options = (args.options or "") + ";" + (args.left_options or "")) as left:
        with api.OpenExtractor(args.RightFile, mode = IGR_FORMAT_IMAGE, options = (args.options or "") + ";" + (args.right_options or "")) as right:
            with left.Compare(thisDocumentSettings = left_page_settings, otherDocument = right, otherDocumentSettings = right_page_settings, compareSettings = compare_settings) as compare_results:

                left_output_canvas = None
                if args.left_output:
                    left_output_canvas = api.MakeOutputCanvas(args.left_output, IGR_DEVICE_IMAGE_PDF)

                right_output_canvas = None
                if args.right_output:
                    right_output_canvas = api.MakeOutputCanvas(args.right_output, IGR_DEVICE_IMAGE_PDF)

                diffs = []
                current_left = -1
                current_right = -1

                while compare_results.MoveNext():
                    diff = compare_results.Current

                    if diff.Type == IGR_COMPARE_DOCUMENTS_DIFFERENCE_EQUAL or diff.Type == IGR_COMPARE_DOCUMENTS_DIFFERENCE_NEXT_BATCH:
                        continue

                    info = {
                        "type": str(diff.Type),
                        "originalPage": diff.OriginalPageIndex + 1,
                        "revisedPage": diff.RevisedPageIndex + 1,
                        "parts": []
                    }

                    for hit in diff.Details:
                        if diff.Type == IGR_COMPARE_DOCUMENTS_DIFFERENCE_DELETE and left_output_canvas:
                            current_left = ensure_page(hit.PageIndex, left, left_output_canvas, current_left, True)
                            left_output_canvas.Rect(int(hit.Bounds.left), int(hit.Bounds.top), int(hit.Bounds.right), int(hit.Bounds.bottom))

                        if diff.Type == IGR_COMPARE_DOCUMENTS_DIFFERENCE_INSERT and right_output_canvas:
                            current_right = ensure_page(hit.PageIndex, left, right_output_canvas, current_right, False)
                            left_output_canvas.Rect(int(hit.Bounds.left), int(hit.Bounds.top), int(hit.Bounds.right), int(hit.Bounds.bottom))

                        part = {
                            "text": hit.Text,
                            "page": hit.PageIndex + 1,
                            "bounds": {
                                "l": hit.Bounds.left,
                                "t": hit.Bounds.top,
                                "r": hit.Bounds.right,
                                "b": hit.Bounds.bottom
                            }
                        }
                        info["parts"].append(part)

                    diffs.append(info)

            output = {
                "original": args.LeftFile,
                "revised": args.RightFile,
                "diffs": diffs
            }

            if left_output_canvas is not None:
                left_output_canvas.Close()
            if right_output_canvas is not None:
                right_output_canvas.Close()

            if not args.silent:
                print(json.dumps(output, indent = 4))

    return 0

if __name__ == "__main__":
    main()
