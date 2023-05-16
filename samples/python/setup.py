#!/usr/bin/env python3
# vim: set list et ts=8 sts=4 sw=4 ft=python:

from setuptools import setup, find_packages
from setuptools.extension import Extension
import os

try:
  DF_VERSION=os.environ['ISYS_BUILD_PRODUCT_VERSION']
except KeyError:
  DF_VERSION = '23.2'

setup(name='DocumentFilters',
      version=DF_VERSION,
      url='https://github.com/HylandSoftware/DocumentFilters',
      license='MIT',
      author='Hyland Software, Inc.',
      description='Document Filters python API',
      packages=find_packages(exclude=['tests']),
    #   long_description=open('README.md').read(),
      zip_safe=False)