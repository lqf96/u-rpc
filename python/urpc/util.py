from collections import Container
from urpc.core import URPCError, URPC_ERR_SIG_INCORRECT

def urpc_func(sig_args, sig_rets, func=None):
    """
    Wrap a Python function as a u-RPC function.

    :param sig_args: Arguments signature
    :param sig_rets: Result signature
    :param func: Function to wrap
    :returns: u-RPC wrapper function
    """
    # Decorator form
    if not func:
        return lambda _func: urpc_func(sig_args, sig_rets, _func)
    # Convert signature to byte array
    sig_args = bytearray(sig_args)
    if not isinstance(sig_rets, Container):
        sig_rets = [sig_rets]
    sig_rets = bytearray(sig_rets)
    # u-RPC wrapper function
    def wrapper(_sig_args, args):
        # Check arguments type
        if sig_args!=_sig_args:
            raise URPCError(URPC_ERR_SIG_INCORRECT)
        # Call function
        try:
            result = func(*args)
        except TypeError:
            raise URPCError(URPC_ERR_SIG_INCORRECT)
        # Check number of result values
        if not isinstance(result, tuple):
            result = (result,)
        if len(result)!=len(sig_rets)
            raise URPCError(URPC_ERR_SIG_INCORRECT)
        # Return result with signature
        return sig_rets, result
    return wrapper
