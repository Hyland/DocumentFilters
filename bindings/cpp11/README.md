# C++11 Bindings for Hyland Document Filters SDK (Deprecated)

This directory contains the C++11 bindings for the Hyland Document Filters SDK. These bindings provided a modern C++ interface to the SDK, allowing developers to utilize its functionalities in C++ projects. However, these bindings are now deprecated in favor of the C++17 bindings. Users are encouraged to transition to the C++17 bindings for new development.

## Prerequisites

To build and use the C++11 bindings, ensure you have the following installed:

- CMake (version 3.10 or later)
- A C++11 compatible compiler (GCC, Clang, or MSVC)

## Integration with FetchContent

You can integrate the C++11 bindings into your project using CMake's `FetchContent` module. Follow the steps below to include the bindings in your CMake project:

### 1. Add FetchContent to Your CMake Project

In your `CMakeLists.txt`, include the following lines to fetch the C++11 bindings:

```cmake
include(FetchContent)

FetchContent_Declare(
    HylandDocumentFiltersCpp11
    GIT_REPOSITORY https://github.com/Hyland/DocumentFilters.git
    GIT_TAG <desired-tag-or-branch>
    GIT_SUBDIRECTORY bindings/cpp11
)

FetchContent_MakeAvailable(HylandDocumentFiltersCpp11)
```

Replace `desired-tag-or-branch` with the specific tag or branch you want to use.

### 2. Link the C++11 Bindings

After fetching the content, you can link the C++11 bindings to your target:

```cmake
target_link_libraries(your_target PRIVATE HylandDocumentFiltersCpp11)
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
for details on how to use the C++11 bindings in your application. The API functions
and examples can be found within the SDK documentation.

## License

The C++11 bindings are licensed under the same terms as the Hyland Document Filters SDK. See the [LICENSE](../../LICENSE.md) file for more details.
