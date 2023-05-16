## Setup

You must install the python bindings before you can run the samples.

```
python setup.sh install
```

## Troubleshooting

```libISYS11df.so: cannot open shared object file: No such file or directory```

If you see an error similar to above, it means that Python was unable to locate the Document Filters Shared Libraries. Standard dlopen rules are used to locate the libraries.

This can often be worked around by adding the path containing the libraries to the LD_LIBRARY_PATH environment variable.