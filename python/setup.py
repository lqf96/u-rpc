#! /usr/bin/env python
from __future__ import unicode_literals
from setuptools import setup

setup(
    name="u-rpc",
    version="0.2.2",
    author="lqf96",
    author_email="lqf.1996121@gmail.com",
    description="The u-RPC Remote Procedure Call framework",
    license="MIT",
    packages=["urpc"],
    install_requires=["six", "bidict"],
    test_suite="urpc_test.test_suite",
    tests_require=["cffi"],
)
