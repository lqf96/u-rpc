from __future__ import absolute_import, unicode_literals
from unittest import TestCase
from six import text_type

from urpc import URPC, StringType, URPC_ERR_NONEXIST, URPC_ERR_SIG_INCORRECT, U8, U16, U32, VARY
from urpc_test.callee import set_up_test_functions

class Py2PyTest(TestCase):
    def setUp(self):
        # Caller endpoint
        caller = self._caller = URPC(
            send_callback=None
        )
        # Callee endpoint
        callee = self._callee = URPC(
            send_callback=caller.recv_callback,
            n_funcs=16
        )
        # Caller send callback
        caller._send_callback = callee.recv_callback
        # Set up test functions
        set_up_test_functions(self, callee)
    def test_func_query(self):
        """
        Test u-RPC function name to handle query.
        """
        # Existing function
        @self._caller.query("func_1")
        def cb(error, result):
            # No error happened
            self.assertIsNone(error)
            # Function handle
            self.assertIsInstance(result, int)
        # Non-exist function
        @self._caller.query("func_nonexist")
        def cb(error, _):
            # Error happened
            self.assertEqual(error.reason, URPC_ERR_NONEXIST)
    def test_func_call(self):
        """
        Test u-RPC function call.
        """
        # Non-exist function
        @self._caller.call(100, [], [])
        def cb(error, _):
            # Error happened
            self.assertEqual(error.reason, URPC_ERR_NONEXIST)
        # Function 1 (Using urpc_wrap)
        @self._caller.query("func_1_wrap")
        def cb(_, handle):
            # Correct signature
            @self._caller.call(handle, [U8, U8], [2, 3])
            def cb(error, result):
                # No error happened
                self.assertIsNone(error)
                # Call result
                self.assertEqual(result, [5])
            # Incorrect signature
            @self._caller.call(handle, [U8], [2])
            def cb(error, _):
                # Error happened
                self.assertEqual(error.reason, URPC_ERR_SIG_INCORRECT)
        # Function 1 (Using urpc_sig)
        @self._caller.query("func_1_sig")
        def cb(_, handle):
            @self._caller.call(handle, [U8, U8], [2, 3])
            def cb(error, result):
                # No error happened
                self.assertIsNone(error)
                # Call result
                self.assertEqual(result, [5])
        # Function 1 (Implicitly wrapped)
        @self._caller.query("func_1")
        def cb(_, handle):
            @self._caller.call(handle, [U8, U8], [2, 3])
            def cb(error, result):
                # No error happened
                self.assertIsNone(error)
                # Call result
                self.assertEqual(result, [5])
    def test_variable_data(self):
        """
        Test u-RPC variable length data.
        """
        @self._caller.query("func_2")
        def cb(_, handle):
            # Test byte string
            test_bytes = b"1234"
            # Do u-RPC call
            @self._caller.call(handle, [VARY], [test_bytes])
            def cb(error, result):
                # No error happened
                self.assertIsNone(error)
                # Call result
                self.assertEqual(result, [test_bytes*3])
    def test_urpc_string(self):
        """
        Test u-RPC high level string type.
        """
        @self._caller.query("func_3")
        def cb(_, handle):
            # Test string
            test_string = "1234"
            # Do u-RPC call
            @self._caller.call(handle, [StringType], [test_string])
            def cb(error, result):
                # No error happened
                self.assertIsNone(error)
                # Call result
                self.assertEqual(result, [len(test_string)])
    def test_multi_result(self):
        """
        Test zero arguments and multiple results.
        """
        @self._caller.query("func_4")
        def cb(_, handle):
            @self._caller.call(handle, [], [])
            def cb(error, result):
                # No error happened
                self.assertIsNone(error)
                # Call result
                self.assertEqual(result, [4, b"test"])
    def test_variable_sig(self):
        """
        Test u-RPC variable signature function.
        """
        @self._caller.query("func_5")
        def cb(_, handle):
            # 0 arguments
            @self._caller.call(handle, [], [])
            def cb(error, result):
                # No error happened
                self.assertIsNone(error)
                # Number of results
                self.assertEqual(len(result), 0)
            # Call arguments
            test_args = [1, 500, 100000, b"abcd"]
            # 5 arguments
            @self._caller.call(handle, [U8, U16, U32, VARY], test_args)
            def cb(error, result):
                # No error happened
                self.assertIsNone(error)
                # Result
                self.assertEqual(result, test_args)
