from __future__ import absolute_import, unicode_literals
from unittest import TestSuite, makeSuite

from urpc_test.py_test import Py2PyTest

# Test suite
test_suite = TestSuite()
# Collect test cases
test_suite.addTest(makeSuite(Py2PyTest))
