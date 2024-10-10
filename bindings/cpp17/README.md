# C++17 Bindings for Hyland Document Filters SDK

This directory contains the C++17 bindings for the Hyland Document Filters SDK.
These bindings take advantage of modern C++ features to provide an improved
interface for integrating the SDK into C++ projects. 

## Prerequisites

To build and use the C++17 bindings, ensure you have the following installed:

- CMake (version 3.10 or later)
- A C++17 compatible compiler (GCC, Clang, or MSVC)

## Integration with FetchContent

You can integrate the C++11 bindings into your project using CMake's `FetchContent` module. Follow the steps below to include the bindings in your CMake project:

### 1. Add FetchContent to Your CMake Project

In your `CMakeLists.txt`, include the following lines to fetch the C++11 bindings:

```cmake
include(FetchContent)

FetchContent_Declare(
    HylandDocumentFiltersCpp17
    GIT_REPOSITORY https://github.com/Hyland/DocumentFilters.git
    GIT_TAG <desired-tag-or-branch>
    GIT_SUBDIRECTORY bindings/cpp17
)

FetchContent_MakeAvailable(HylandDocumentFiltersCpp17)
```

Replace `desired-tag-or-branch` with the specific tag or branch you want to use.

### 2. Link the C++17 Bindings

After fetching the content, you can link the C++17 bindings to your target:

```cmake
target_link_libraries(your_target PRIVATE HylandDocumentFiltersCpp17)
```

Replace `your_target` with the name of your executable or library.

### 3. Build Your Project

Run the following commands to configure and build your project:

```bash
mkdir build
cd build
cmake ..
make
```

## Usage

Refer to the [`Hyland Document Filters SDK
documentation`](https://docs.hyland.com/DocumentFilters/en_US/Print/index.html)
for details on how to use the C++17 bindings in your application. The API functions
and examples can be found within the SDK documentation.

## License

The C++17 bindings are licensed under the same terms as the Hyland Document Filters SDK. See the [LICENSE](../../LICENSE.md) file for more details.
