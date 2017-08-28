from __future__ import unicode_literals
import functools
from six import text_type

from urpc import urpc_sig, urpc_wrap, U8, VARY, StringType

def _urpc_test_func_1(x, y):
    """
    u-RPC integer arguments test function.

    :param x, y: 8-bit unsigned integers
    :returns: Sum of the two numbers
    """
    return x+y

def _urpc_test_func_2(buf):
    """
    u-RPC variable length data test function.

    :param buf: A byte string buffer
    :returns: The same byte string repeated three times
    """
    return buf*3

def _urpc_test_func_3(test_case, string):
    """
    u-RPC string type data test function.

    :param string: A string
    :returns: Length of the string
    """
    # Ensure we get a string
    test_case.assertEqual(type(string), text_type)
    # Return string length
    return len(string)

def _urpc_test_func_4():
    """
    u-RPC zero arguments and multiple return value test function.

    :returns: A number and a string in a tuple
    """
    return 4, "test"

def _urpc_test_func_5(arg_types, args):
    """
    u-RPC variable signature test function.

    :returns: Argument types and arguments without any change
    """
    return arg_types, args

def set_up_test_functions(test_case, callee):
    """
    Add test functions to callee endpoint.

    :param test_case: Test case
    :param callee: Callee u-RPC endpoint
    """
    # Function 1
    callee.add_func(
        func=urpc_wrap([U8, U8], [U8], _urpc_test_func_1),
        name="func_1_wrap"
    )
    callee.add_func(
        func=urpc_sig([U8, U8], [U8], _urpc_test_func_1),
        name="func_1_sig"
    )
    callee.add_func(
        func=_urpc_test_func_1,
        arg_types=[U8, U8],
        ret_types=[U8],
        name="func_1"
    )
    # Function 2
    callee.add_func(
        func=_urpc_test_func_2,
        arg_types=[VARY],
        ret_types=[VARY],
        name="func_2"
    )
    # Function 3
    callee.add_func(
        func=functools.partial(_urpc_test_func_3, test_case),
        arg_types=[StringType],
        ret_types=[U8],
        name="func_3"
    )
    # Function 4
    callee.add_func(
        func=_urpc_test_func_4,
        arg_types=[],
        ret_types=[U8, StringType],
        name="func_4"
    )
    # Function 5
    callee.add_func(
        func=_urpc_test_func_5,
        name="func_5"
    )
