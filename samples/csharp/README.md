# Document Filters C# Samples

![NuGet Version](https://img.shields.io/nuget/vpre/Hyland.DocumentFilters)
![NuGet Downloads](https://img.shields.io/nuget/dt/Hyland.DocumentFilters)

This repository contains samples and utilities for Document Filters, a set of
tools for converting and processing various document formats. Explore the
following directories and files to understand and use the capabilities of
Document Filters.

## Summary

The Document Filters Sample GitHub Repository includes:

- Samples for converting documents to different formats such as PDF, PNG, SVG,
  and more.
- Utilities for common tasks like extracting words from documents.
- A shared common library for Document Filters samples.
- Visual Studio solution file and the license information.

Explore the contents to leverage the power of Document Filters in your document
processing projects.

To get started on your own project, check out the [Getting
Started](https://hyland.github.io/DocumentFilters-Docs/latest/getting_started_with_document_filters/getting_started_dotnet.html)
section in the documentation.

## Projects and Files

| Name                                                                 | Description                                                                                                           |
| -------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------- |
| [CombineDocuments](./CombineDocuments)                               | Combines multiple documents into a single multi-page document, with the ability to create an optional thumbnail page. |
| [ConvertDocument](./ConvertDocument)                                 | Converts documents to either text or flow-html. Additionally, sub-files can also be processed.                        |
| [ConvertDocumentToHDHTML](./ConvertDocumentToHDHTML)                 | Converts documents to high-definition, paginated HTML.                                                                |
| [ConvertDocumentToPDF](./ConvertDocumentToPDF)                       | Converts documents to PDF format.                                                                                     |
| [ConvertDocumentToPNG](./ConvertDocumentToPNG)                       | Converts documents to PNG format.                                                                                     |
| [ConvertDocumentToPostscript](./ConvertDocumentToPostscript)         | Converts documents to Postscript format.                                                                              |
| [ConvertDocumentToStructuredXML](./ConvertDocumentToStructuredXML)   | Converts documents to structured XML.                                                                                 |
| [ConvertDocumentToSVG](./ConvertDocumentToSVG)                       | Converts documents to SVG format.                                                                                     |
| [ConvertDocumentToThumbnail](./ConvertDocumentToThumbnail)           | Converts documents to thumbnail images.                                                                               |
| [ConvertDocumentToTIFF](./ConvertDocumentToTIFF)                     | Converts documents to TIFF format.                                                                                    |
| [ConvertDocumentToTIFFFromMemory](./ConvertDocumentToTIFFFromMemory) | Converts documents to TIFF format from memory.                                                                        |
| [ConvertDocumentToTIFFStream](./ConvertDocumentToTIFFStream)         | Converts documents to TIFF format using streams.                                                                      |
| [ConvertDocumentToUTF8WithOCR](./ConvertDocumentToUTF8WithOCR)       | Converts documents to text using Optical Character Recognition.                                                       |
| [ConvertDocumentWithComments](./ConvertDocumentWithComments)         | Converts documents to a multi-page PDF, with comments shown in either the margin or on new pages.                     |
| [CreateBarcode](./CreateBarcode)                                     | Demonstrates create 1D and 2D bar codes                                                                               |
| [ExtractAnnotations](./ExtractAnnotations)                           | Extracts annotations from documents.                                                                                  |
| [ExtractForms](./ExtractForms)                                       | Extracts forms from documents.                                                                                        |
| [ExtractSubfiles](./ExtractSubfiles)                                 | Extracts subfiles from documents or archive, with support for password protected files.                               |
| [GetDocumentWords](./GetDocumentWords)                               | Retrieves words from documents.                                                                                       |
| [LocalizeMetadata](./LocalizeMetadata)                               | Localizes metadata within documents, mapping rendered metadata to either English, French, German or Korean.           |
| [MarkupAnnotationsDemo](./MarkupAnnotationsDemo)                     | Demonstrates marking up annotations in documents.                                                                     |
| [RedactionDemo](./RedactionDemo)                                     | Demonstrates redacting content in a document based on various different rule type.                                    |
| [UpdateForm](./UpdateForm)                                           | Updates form data within a PDF document.                                                                              |
| [DfSamplesCommon](./DfSamplesCommon)                                 | Common functionality for Document Filters samples.                                                                    |
| [DocumentFiltersLicense.cs](./DocumentFiltersLicense.cs)             | Update with your Document Filters License Key                                                                         |

## Getting Started

You can run the sample applications without a license key, with some
limitations.  See [Document Filters Evaluation](../../EVAL.md) for details.

To run the sample applications without feature limitations, ensure you have a
valid Document Filters license key. You can provide this key by either modifying
the DocumentFiltersLicense.cs file or setting it in an environment variable
named `DF_LICENSE_KEY`.

The .NET projects are compatible with win-x64, win-x64, linux-x64,
linux-musl-x64, osx-x64, and osx-arm64.

The samples are configured to pull Document Filters from NuGet. This is
configured in the `DfSamplesCommon` projects.

To compile all samples from the command line, execute:

```bash
cd samples/csharp
dotnet build 
```

Each sample is compiled into its own directory, with binaries generated in
`${PROJECT}/bin/${CONFIGURATION}/net8.0`.

To build all the samples and copy them to a single directory, run:

```bash
dotnet publish -o bin
```

Alternatively, you can open the sln in Visual Studio or the folder in VS Code.
