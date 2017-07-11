from __future__ import unicode_literals
from collections import Container
from abc import ABCMeta, abstractmethod
from six import text_type, with_metaclass

from urpc.core import URPCError, URPC_TYPE_VARY

class URPCType(with_metaclass(ABCMeta, object)):
    """
    u-RPC Python-side high-level type and (de)serializer.
    Must provide property "underlying_type" on subclass instances.
    """
    @abstractmethod
    def loads(self, data):
        """
        Convert u-RPC data to Python value.

        :param data: Raw data to convert
        """
        pass
    @abstractmethod
    def dumps(self, value):
        """
        Convert Python value to u-RPC data.

        :param value: Python value to convert
        """
        pass

class StringType(URPCType):
    """ u-RPC server-side string type and (de)serializer. """
    def __init__(self, encoding="utf-8"):
        self.encoding = encoding
    def loads(self, data):
        """
        Convert u-RPC variable length data to Python string.

        :param data: Raw data to convert
        :raises UnicodeDecodeError: If string decoding fails
        """
        return text_type(data, encoding=self.encoding)
    def dumps(self, value):
        """
        Convert Python string to u-RPC variable length data.

        :param value: Python string to convert
        :raises UnicodeEncodeError: If string encoding fails
        """
        return bytearray(value.encode(self.encoding))
    # u-RPC underlying type
    underlying_type = URPC_TYPE_VARY

def wraps(arg_types, ret_types, func=None):
    """
    Wrap a Python function as a u-RPC function.

    :param arg_types: Arguments signature
    :param ret_types: Result signature
    :param func: Function to wrap
    :returns: u-RPC wrapper function
    """
    # Decorator form
    if not func:
        return lambda _func: urpc_func(arg_types, ret_types, _func)
    # TODO: Convert signature to byte array
    pass
    # TODO: u-RPC wrapper function
    def wrapper(_arg_types, _args):
        pass
    return wrapper
