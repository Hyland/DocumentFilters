# Document Filters Python Samples

This repository contains samples and utilities for Document Filters, a set of
tools for converting and processing various document formats. Explore the
following directories and files to understand and use the capabilities of
Document Filters.

## Summary

The Document Filters Sample GitHub Repository includes:

- Samples for converting documents to different formats such as PDF, PNG, SVG, and more.
- Utilities for common tasks like extracting words from documents.
- A shared common library for Document Filters samples.

Explore the contents to leverage the power of Document Filters in your document processing projects.

To get started on your own project, check out the [Getting
Started](https://docs.hyland.com/DocumentFilters/en_US/Print/getting_started_with_document_filters/create_a_python_api_application.html)
section in the documentation.

## Projects and Files

| Name                                                                       | Description                                                                                                           |
| -------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------- |
| [CombineDocuments.py](./CombineDocuments.py)                               | Combines multiple documents into a single multi-page document, with the ability to create an optional thumbnail page. |
| [ConvertDocumentToHDHTML.py](./ConvertDocumentToHDHTML.py)                 | Converts documents to high-definition, paginated HTML.                                                                |
| [ConvertDocumentToPDF.py](./ConvertDocumentToPDF.py)                       | Converts documents to PDF format.                                                                                     |
| [ConvertDocumentToPNG.py](./ConvertDocumentToPNG.py)                       | Converts documents to PNG format.                                                                                     |
| [ConvertDocumentToStructuredXML.py](./ConvertDocumentToStructuredXML.py)   | Converts documents to structured XML.                                                                                 |
| [ConvertDocumentToTIFF.py](./ConvertDocumentToTIFF.py)                     | Converts documents to TIFF format.                                                                                    |
| [ConvertDocumentToTIFFFromMemory.py](./ConvertDocumentToTIFFFromMemory.py) | Converts documents to TIFF format from memory.                                                                        |
| [ConvertDocumentToTIFFStream.py](./ConvertDocumentToTIFFStream.py)         | Converts documents to TIFF format using streams.                                                                      |
| [ConvertDocumentToUTF8.py](./ConvertDocumentToUTF8.py)                     | Converts documents to either text or flow-html. Additionally, sub-files can also be processed.                        |
| [CreateBarcode.py](./CreateBarcode.py)                                     | Demonstrates create 1D and 2D bar codes                                                                               |
| [ExtractAnnotations.py](./ExtractAnnotations.py)                           | Extracts annotations from documents.                                                                                  |
| [ExtractDocumentWords.py](./ExtractDocumentWords.py)                       | Retrieves words from documents.                                                                                       |
| [WatermarkDocument.py](./WatermarkDocument.pt)                             | Converts a document to PDF and applies a watermark.                                                                   |
| [DocumentFiltersLicense.py](./DocumentFiltersLicense.py)                   | Update with your Document Filters License Code                                                                        |
| [DocumentFiltersSample.py](./DocumentFiltersSample.py)                     | Utility functions for setup Document Filters objects.                                                                 |

## Getting Started

To run the sample applications, ensure you possess a valid Document Filters
license key. You can provide this key by either modifying the
DocumentFiltersLicense.py file or setting it in an environment variable named
DF_LICENSE_CODE. Alternatively, you can pass the license code directly to the
samples by specifying `--license-key`.

With a license key in hand, the samples can be run by passing to the python interpreter:

```bash
python ./ConvertDocumentToPDF.py "file.docx"
```

The sample will automatically download native libraries tailored for your
platform, along with font assets, and store them in a `runtimes` directory.

### Using in your application

You can install the DocumentFilters package as a fully integrated Python
package, making it accessible to any Python project. To perform the
installation, execute the following command:

```bash
python3 setup.sh install --user
```

> **_IMPORTANT:_** The package does not come bundled with the native shared
> libraries of Document Filters. It is essential to ensure that these files are
> accessible to the Operating System through its standard shared library
> resolution rules.
>
> In these samples, this requirement is addressed by downloading the binaries
> into a `runtime` directory. Subsequently, the directory name is passed as the
> `dll_path` parameter during the invocation of the `Initialize` function.


<!-- 

You must install the python bindings before you can run the samples.

```bash
python setup.sh install --user
``` -->

## Troubleshooting

`libISYS11df.so: cannot open shared object file: No such file or directory`

If you see an error similar to above, it means that Python was unable to locate the Document Filters Shared Libraries. Standard dlopen rules are used to locate the libraries.

This can often be worked around by adding the path containing the libraries to the LD_LIBRARY_PATH environment variable.
