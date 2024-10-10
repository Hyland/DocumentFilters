# Document Filters C++ 11 Samples

This repository contains samples and utilities for Document Filters, a set of
tools for converting and processing various document formats. Explore the
following directories and files to understand and use the capabilities of
Document Filters.

> Note: The C++11 bindings are now deprecated. Consider moving to the [C++
> 17](../cpp17) bindings.

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
Started](https://docs.hyland.com/DocumentFilters/en_US/Print/getting_started_with_document_filters/create_a_c_api_class_wrapper_around_native_library_functions_application.html)
section in the documentation.

## Projects and Files

| Name                                                                       | Description                                             |
| -------------------------------------------------------------------------- | ------------------------------------------------------- |
| [ConvertDocumentToClassicHTML](./ConvertDocumentToClassicHTML)             | Converts documents to classic HTML format.              |
| [ConvertDocumentToHDHTML](./ConvertDocumentToHDHTML)                       | Converts documents to high-definition HTML.             |
| [ConvertDocumentToInteractiveHDHTML](./ConvertDocumentToInteractiveHDHTML) | Converts documents to interactive high-definition HTML. |
| [ConvertDocumentToPDF](./ConvertDocumentToPDF)                             | Converts documents to PDF format.                       |
| [ConvertDocumentToPNG](./ConvertDocumentToPNG)                             | Converts documents to PNG format.                       |
| [ConvertDocumentToPostscript](./ConvertDocumentToPostscript)               | Converts documents to Postscript format.                |
| [ConvertDocumentToStructuredXML](./ConvertDocumentToStructuredXML)         | Converts documents to structured XML format.            |
| [ConvertDocumentToSVG](./ConvertDocumentToSVG)                             | Converts documents to SVG format.                       |
| [ConvertDocumentToThumbnail](./ConvertDocumentToThumbnail)                 | Converts documents to thumbnail images.                 |
| [ConvertDocumentToTIFF](./ConvertDocumentToTIFF)                           | Converts documents to TIFF format.                      |
| [ConvertDocumentToTIFFStream](./ConvertDocumentToTIFFStream)               | Converts documents to TIFF format using streams.        |
| [ConvertDocumentToUnicode](./ConvertDocumentToUnicode)                     | Converts documents to Unicode format.                   |
| [ConvertDocumentToUTF8](./ConvertDocumentToUTF8)                           | Converts documents to UTF-8 format.                     |
| [ConvertDocumentToXML](./ConvertDocumentToXML)                             | Converts documents to XML format.                       |
| [DocumentFiltersObjects](./DocumentFiltersObjects)                         | Object library for Document Filters.                    |
| [DocumentFiltersSamples](./DocumentFiltersSamples)                         | Sample projects demonstrating Document Filters usage.   |
| [GetDocumentType](./GetDocumentType)                                       | Retrieves the type of a document.                       |
| [GetDocumentWords](./GetDocumentWords)                                     | Retrieves words from documents.                         |
| [WatermarkDocument](./WatermarkDocument)                                   | Applies a watermark to documents.                       |
| [CMakeLists.txt](./CMakeLists.txt)                                         | CMake Project File.                                     |

## Getting Started

You can run the sample applications without a license key, with some
limitations.  See [Document Filters Evaluation](../../EVAL.md) for details.

To run the sample applications without feature limitations, ensure you have a
valid Document Filters license key. You can provide this code by either
modifying the DocumentFiltersLicense.h file or setting it in an environment
variable named `DF_LICENSE_KEY`.

### CMake

The C++ samples are constructed using cmake across all platforms. If you are on
Linux, Mac, or Windows with CMake in your system's path, you can build using:

```bash
cd ./samples/cpp
cmake -S . -B build
cmake --build build
```

This process will compile all samples into the build/bin directory.

The samples will search for the Document Filters shared libraries or DLLs
following the operating system's loading rules. The CMake project
`DocumentFiltersBinaries` will fetch the binaries from GitHub releases and
transfer them to the output directory.

### Windows

On Windows, you can build using the cmake command line tools, or using Visual
Studio.

To build with the command line, start a command prompt for your version of
Visual Studio, for example `x64 Native Tools Command Prompt for Visual Studio
2022`.

```bat
cd samples\cpp
cmake -S . -B build
cmake --build build
```

### Building with Visual Studio

Alternatively, you can use the Visual Studio IDE for building. Follow these
steps:

1. In the `What do you want to do` dialog, choose `Open a local folder`.
   Alternatively, select `File > Open > Folder...` from the menu.
2. Select the directory containing the C++ samples: `samples/cpp`.
3. From the menu, go to `View > CMake Targets`.
4. Execute `Build > Build All` from the menu.

### Building with Visual Studio Code

Visual Studio Code, with the CMake plugin, is another excellent option for
working with the C++ samples. Ensure you have the [CMake
Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)
extension installed.

1. Choose `File > Open Folder` from the menu and select the directory with the
   C++ samples: `samples/cpp`.
2. In the sidebar, click on the `CMake` icon.
3. Under `Configure`, select the compiler/toolchain of your choice.
4. Press `F7` to initiate the build process.
