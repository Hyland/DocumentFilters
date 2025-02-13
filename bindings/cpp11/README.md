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
    SOURCE_SUBDIR bindings/cpp11
)

FetchContent_MakeAvailable(HylandDocumentFiltersCpp11)
```

Replace `desired-tag-or-branch` with the specific tag or branch you want to use.

#### 1.1. Configuration options

By default, integrating this CMake project will download the release binaries
from GitHub for use in your application. If no specific options are set, the
latest version will be downloaded, regardless of which tag was cloned.

You can customize this behavior using the following options:

- **DOCFILTERS_DOWNLOAD_BINARIES**: Controls whether the release binaries are
  downloaded. Defaults to `ON`.
- **DOCFILTERS_INSTALL_BINARIES**: Determines if the release binaries should be
  copied to the output `bin` directory during the `cmake --install` step.
  Defaults to `ON`.
- **DOCFILTERS_INSTALL_ASSETS**: Specifies if release assets (such as fonts,
  configuration files, and OCR data) should be copied to the output `bin`
  directory during installation. Defaults to `ON`.
- **DOCFILTERS_RELEASES_URL**: Overrides the URL from which the release binaries
  are downloaded. Defaults to the official GitHub repository.
- **DOCFILTERS_VERSION**: Specifies the version of the release binaries to
  download. Defaults to the latest version.
- **DOCFILTERS_RUNTIME_BINARIES_DIRECTORY**: If `DOCFILTERS_DOWNLOAD_BINARIES`
  is set to OFF, provide a directory where the DocFilters release binaries are
  located.

##### Example 1. Download and Install Specific Version of Binaries

If you want to download and install a specific version of the release binaries,
for example, version `25.1.0`:

```cmake
set(DOCFILTERS_VERSION "25.1.0" CACHE STRING "")
FetchContent_Declare( ... )
```

##### Example 2. Disable Binary Download and Use Local Binaries

If you already have the release binaries downloaded locally and want to avoid
downloading them again:

```cmake
set(DOCFILTERS_DOWNLOAD_BINARIES OFF CACHE BOOL "")
set(DOCFILTERS_RUNTIME_BINARIES_DIRECTORY "/path/to/local/binaries" CACHE STRING "")
FetchContent_Declare( ... )
```

This will use the binaries from the specified local directory.

##### Example 3. Use Custom URL for Downloading Binaries

If you are hosting the release binaries in a different location (e.g., internal
server or alternative GitHub repository), you can provide a custom URL for the
download:

```cmake
set(DOCFILTERS_RELEASES_URL "https://internal-server.com/myproject/releases" CACHE STRING "")
FetchContent_Declare( ... )
```

##### Example 4. Skip Installation of Assets

If you don't need to install additional assets like fonts, configuration files,
or OCR data, you can disable the asset installation:

```cmake
set(DOCFILTERS_INSTALL_ASSETS OFF CACHE BOOL "")
FetchContent_Declare( ... )
```

##### Example 5. Disable Installation of Binaries

If you only need the binaries for the build process but don’t want them
installed into the output directory during `cmake --install`:

```cmake
set(DOCFILTERS_INSTALL_BINARIES OFF CACHE BOOL "")
FetchContent_Declare( ... )
```

### 2. Link the C++11 Bindings

After fetching the content, you can link the C++11 bindings to your target:

```cmake
target_link_libraries(your_target PRIVATE DocumentFilters::Cpp11)
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

## Troubleshooting

### 1. Link Error: mismatch detected for 'RuntimeLibrary'

```link
error LNK2038: mismatch detected for 'RuntimeLibrary': value 'MDd_DynamicDebug' doesn't match value 'MT_StaticRelease' in file.cpp.obj
```

If you encounter a `RuntimeLibrary` mismatch error while compiling in Visual
Studio, ensure that your `CMakeLists.txt` specifies a `cmake_minimum_required`
version of at least `3.16` as shown below:

```cmake
cmake_minimum_required (VERSION 3.16)
```

## License

The C++11 bindings are licensed under the same terms as the Hyland Document Filters SDK. See the [LICENSE](../../LICENSE.md) file for more details.
