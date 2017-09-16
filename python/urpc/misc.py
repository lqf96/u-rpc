from __future__ import absolute_import, unicode_literals
import functools
from collections import Container
from abc import ABCMeta, abstractmethod
from six import text_type, with_metaclass
from six.moves import range

from urpc.constants import *

class URPCError(Exception):
    """!
    @brief u-RPC error class.
    """
    def __init__(self, reason):
        """!
        @brief u-RPC error type constructor.

        @param reason Reason of the error.
        """
        ## Reason of the error
        self.reason = reason

class URPCType(with_metaclass(ABCMeta, object)):
    """!
    @brief u-RPC Python-side high-level type and (de)serializer.

    Must provide property "underlying_type" on subclass instances.
    """
    @abstractmethod
    def loads(self, data):
        """!
        @brief Convert u-RPC data to Python value.

        @param data Raw u-RPC data to convert.
        @return Python value.
        """
        pass
    @abstractmethod
    def dumps(self, value):
        """!
        @brief Convert Python value to u-RPC data.

        @param value: Python value to convert.
        @return Raw u-RPC data.
        """
        pass

class StringType(URPCType):
    """!
    @brief u-RPC server-side string type and (de)serializer.
    """
    def __init__(self, encoding="utf-8"):
        """!
        @brief u-RPC string type constructor.

        @param encoding: Python string encoding.
        """
        ## Python string encoding
        self.encoding = encoding
    def loads(self, data):
        """!
        @brief Convert u-RPC variable length data to Python string.

        @param data Raw bytes to convert.
        @return A python string.
        @throws UnicodeDecodeError If string decoding fails.
        """
        return text_type(data, encoding=self.encoding)
    def dumps(self, value):
        """!
        @brief Convert Python string to u-RPC variable length data.

        @param value Python string to convert.
        @return Encoded byte string.
        @throws UnicodeEncodeError If string encoding fails.
        """
        return bytearray(value.encode(self.encoding))
    ## u-RPC underlying type
    underlying_type = URPC_TYPE_VARY

def urpc_sig(arg_types, ret_types, func=None):
    """!
    @brief Decorate Python function with u-RPC signature.

    @param arg_types Arguments signature.
    @param ret_types Result signature.
    @param func: Function to decorate.
    @return Decorated Python function.
    """
    # Decorator style
    if not func:
        return lambda _func: urpc_sig(arg_types, ret_types, _func)
    # Wrapper function
    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        return func(*args, **kwargs)
    # Set arguments and result signature
    wrapper.__urpc_arg_types = arg_types
    wrapper.__urpc_ret_types = ret_types
    # Return function
    return wrapper

def urpc_wrap(arg_types, ret_types, func=None):
    """!
    @brief Wrap a Python function as a u-RPC function.

    @param arg_types Arguments signature.
    @param ret_types Result signature.
    @param func Function to wrap.
    @return u-RPC wrapper function.
    """
    # Decorator form
    if not func:
        return lambda _func: urpc_func(arg_types, ret_types, _func)
    # Arguments types
    for i in range(len(arg_types)):
        t = arg_types[i]
        if isinstance(t, type) and issubclass(t, URPCType):
            arg_types[i] = t()
    # Result types
    for i in range(len(ret_types)):
        t = ret_types[i]
        if isinstance(t, type) and issubclass(t, URPCType):
            ret_types[i] = t()
    # Low-level argument types and return types
    underlying_arg_types = bytearray([
        t if isinstance(t, int) else t.underlying_type
        for t in arg_types
    ])
    underlying_ret_types = bytearray([
        t if isinstance(t, int) else t.underlying_type
        for t in ret_types
    ])
    # u-RPC wrapper function
    def wrapper(_arg_types, _args):
        # Compare actual arguments types with low-level arguments types
        if _arg_types!=underlying_arg_types:
            raise URPCError(URPC_ERR_SIG_INCORRECT)
        # Deserialize arguments
        args = [
            t.loads(arg) if isinstance(t, URPCType) else arg
            for t, arg in zip(arg_types, _args)
        ]
        # Invoke wrapped Python function
        try:
            results = func(*args)
        except BaseException as e:
            raise URPCError(URPC_ERR_EXCEPTION)
        # Wrap result in tuple
        if not isinstance(results, tuple):
            results = (results,)
        # Serialize results
        results = [
            t.dumps(arg) if isinstance(t, URPCType) else arg
            for t, arg in zip(ret_types, results)
        ]
        # Return results and low-level results types
        return underlying_ret_types, results
    return wrapper
