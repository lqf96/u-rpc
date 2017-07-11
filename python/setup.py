#! /usr/bin/env python
from setuptools import setup

setup(
    name="u-rpc",
    version="0.1.0",
    author="email",
    author_email="lqf.1996121@gmail.com",
    description="The u-RPC Remote Procedure Call framework",
    license="MIT",
    packages=["urpc"],
    install_requires=["six", "bidict"]
)
