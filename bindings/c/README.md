# C Bindings for Hyland Document Filters SDK

This directory contains the C bindings for the Hyland Document Filters SDK. The
C bindings provide low-level access to the SDK's functionalities, making it
suitable for performance-critical applications and integration with other
C-based projects.

## Prerequisites

To build and use the C bindings, ensure you have the following installed:

- CMake (version 3.10 or later)
- A C compiler (GCC, Clang, or MSVC)

## Integration with FetchContent

You can integrate the C bindings into your project using CMake's `FetchContent`
module. Follow the steps below to include the bindings in your CMake project:

### 1. Add FetchContent to Your CMake Project

In your `CMakeLists.txt`, include the following lines to fetch the C bindings:

```cmake
include(FetchContent)

FetchContent_Declare(
    HylandDocumentFiltersC
    GIT_REPOSITORY https://github.com/Hyland/DocumentFilters.git
    GIT_TAG <desired-tag-or-branch>
    GIT_SUBDIRECTORY bindings/c
)

FetchContent_MakeAvailable(HylandDocumentFiltersC)
```

Replace <desired-tag-or-branch> with the specific tag or branch you want to use.

### 2. Link the C Bindings

After fetching the content, you can link the C bindings to your target:

```cmake
target_link_libraries(your_target PRIVATE HylandDocumentFiltersC)
```

Replace `your_target with` the name of your executable or library.

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
for details on how to use the C bindings in your application. The API functions
and examples can be found within the SDK documentation.

## License

The C bindings are licensed under the same terms as the Hyland Document Filters SDK. See the [LICENSE](../../LICENSE.md) file for more details.
