# Python Bindings for Hyland Document Filters SDK

This directory contains the Python bindings for the Hyland Document Filters SDK.
These bindings provide a Pythonic interface to utilize the SDK's functionalities
in Python applications.

## Prerequisites

To use the Python bindings, ensure you have the following installed:

- [Python](https://www.python.org/downloads/) (version 3.6 or later)
- [pip](https://pip.pypa.io/en/stable/) (Python package installer)

## Installation

You can install the Python bindings in two ways:

### 1. Install Directly from the Directory

Navigate to the `bindings/python` directory and run the following command:

```bash
pip install .
```

### 2. Add to `requirements.txt`

If you're using a requirements.txt file for your project, you can add the following line to include the Python bindings:

```text
DocumentFilters @ git+https://github.com/Hyland/DocumentFilters.git@<desired-tag-or-branch>#subdirectory=bindings/python
```

Replace `<desired-tag-or-branch>` with the specific tag or branch you want to
use.

Then, run the following command to install the dependencies:

```bash
pip install -r requirements.txt
```

## Usage

Once the package is installed, you can use the bindings in your Python application. Here’s a simple example:

```python
from DocumentFilters import *

# Your code to use the Document Filters SDK
```

Refer to the [`Hyland Document Filters SDK
documentation`](https://hyland.github.io/DocumentFilters-Docs/latest/index.html)
for more details on how to utilize the API effectively.

## License

The Python bindings are licensed under the same terms as the Hyland Document Filters SDK. See the [LICENSE](../../LICENSE.md) file for more details.
