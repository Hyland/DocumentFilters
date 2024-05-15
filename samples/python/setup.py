#!/usr/bin/env python3
# vim: set list et ts=8 sts=4 sw=4 ft=python:

from setuptools import setup, find_packages
import os, platform, urllib.request, zipfile, glob

try:
  DF_VERSION=os.environ['ISYS_BUILD_PRODUCT_VERSION']
except KeyError:
  DF_VERSION = '24.2'

# Download the binaries from 
setup(name='DocumentFilters',
      version=DF_VERSION,
      url='https://github.com/HylandSoftware/DocumentFilters',
      license='MIT',
      author='Hyland Software, Inc.',
      description='Document Filters python API',
      packages=['DocumentFilters'],
      zip_safe=False)
