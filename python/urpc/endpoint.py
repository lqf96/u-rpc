from __future__ import absolute_import, unicode_literals
import struct
from io import BytesIO
from bidict import bidict

from urpc.constants import *
from urpc.util import AllocTable, seq_get, read_data, read_vary, write_data, write_vary
from urpc.misc import URPCError, URPCType, urpc_wrap

# u-RPC protocol version
URPC_VERSION = 0
# u-RPC magic ("ur")
URPC_MAGIC = 29301

class URPC(object):
    """ u-RPC endpoint class. """
    def __init__(self, send_callback, n_funcs=256):
        """
        u-RPC endpoint class constructor.

        :param send_callback: Function for sending data
        :param n_funcs: Maximum number of functions in store
        """
        # Functions store (Handle to function mapping)
        self._funcs_store = AllocTable(n_funcs)
        # Function name to handle mapping
        self._func_name_lookup = bidict()
        # Message ID counter
        self._counters = {"send": 0, "recv": 0}
        # Send data callback
        self._send_callback = send_callback
        # Operation callbacks
        self._oper_callbacks = {}
    def _build_header(self, msg_type, counter):
        """
        Build u-RPC message header.
        When calling this function, the message counter will be updated.

        :param msg_type: Message type
        :returns: Response stream with message header written
        """
        res = BytesIO()
        # Magic and protocol version
        write_data(res, URPC_MAGIC, URPC_TYPE_U16)
        write_data(res, URPC_VERSION, URPC_TYPE_U8)
        # Message ID and type
        write_data(res, self._counters[counter], URPC_TYPE_U16)
        write_data(res, msg_type, URPC_TYPE_U8)
        # Update counter
        self._counters[counter] += 1
        if self._counters[counter]>=2**16:
            self._counters[counter] = 0
        return res
    def _marshall(self, stream, sig, objects):
        """
        Marshall objects into data stream

        :param stream: Data stream
        :param sig: Signature of objects
        :param objects: Objects to be marshalled
        """
        # Check signature
        if len(sig)!=len(objects):
            raise URPCError(URPC_ERR_SIG_INCORRECT)
        # Marshall arguments
        for obj, obj_type in zip(objects, sig):
            # Variable length data
            if obj_type==URPC_TYPE_VARY:
                write_vary(stream, obj)
            # Value types
            else:
                write_data(stream, obj, obj_type)
    def _unmarshall(self, stream, sig):
        """
        Unmarshall objects from data stream

        :param stream: Data stream
        :param sig: Signature of objects
        :returns: Objects in an array
        """
        objects = []
        # Unmarshall arguments
        for obj_type in sig:
            obj = None
            # Read argument
            if obj_type==URPC_TYPE_VARY:
                obj = read_vary(stream)
            else:
                obj = read_data(stream, obj_type)
            objects.append(obj)
        return objects
    def _invoke_callback(self, msg_id, result):
        """
        Invoke and remove callback for given message ID.

        :param msg_id: Request message ID
        :param result: Result
        """
        # Invoke callback
        self._oper_callbacks[msg_id](None, result)
        # Remove callback
        del self._oper_callbacks[msg_id]
    def _handle_msg(self, req):
        """
        Handle received messages.

        :param req: Request message stream
        :returns: Response message stream
        """
        # Message ID defaults to 0 (Unknown)
        msg_id = 0
        try:
            # Response stream
            res = None
            # Parse magic string and version
            if read_data(req, URPC_TYPE_U16)!=URPC_MAGIC:
                raise URPCError(URPC_ERR_BROKEN_MSG)
            if read_data(req, URPC_TYPE_U8)!=URPC_VERSION:
                raise URPCError(URPC_ERR_NO_SUPPORT)
            # Parse message ID and type
            msg_id = read_data(req, URPC_TYPE_U16)
            msg_type = read_data(req, URPC_TYPE_U8)
            # Call message handler
            msg_handler = _urpc_msg_handlers[msg_type]
            if not msg_handler:
                raise URPCError(URPC_ERR_NO_SUPPORT)
            return msg_handler(self, req, msg_id)
        # URPC error occured
        except URPCError as e:
            res = self._build_header(URPC_MSG_ERROR, "recv")
            # Write request message ID and error code
            write_data(res, msg_id, URPC_TYPE_U16)
            write_data(res, e.reason, URPC_TYPE_U16)
            # Return stream
            return res
    def _handle_error(self, res, msg_id):
        """
        u-RPC error result handler.

        :param res: Response message stream
        :param msg_id: Request message ID
        :returns: A response message
        """
        # Request message ID and error number
        req_msg_id = read_data(res, URPC_TYPE_U16)
        error_num = read_data(res, URPC_TYPE_U8)
        # Invoke callback with error object
        callback = self._oper_callbacks[req_msg_id]
        callback(URPCError(error_num), None)
        # Remove callback function
        del self._oper_callbacks[req_msg_id]
    def _handle_func_query(self, req, msg_id):
        """
        u-RPC function query handler.

        :param req: Request message stream
        :param msg_id: Request message ID
        :returns: A response message
        """
        # Function name
        name = read_vary(req).decode("utf-8")
        # Search for function in function lookup
        handle = self._func_name_lookup.get(name)
        if handle==None:
            raise URPCError(URPC_ERR_NONEXIST)
        # Response message
        res = self._build_header(URPC_MSG_FUNC_RESP, "recv")
        write_data(res, msg_id, URPC_TYPE_U16)
        write_data(res, handle, URPC_TYPE_U16)
        return res
    def _handle_func_resp(self, res, msg_id):
        """
        u-RPC function query response handler.

        :param req: Request message stream
        :param msg_id: Request message ID
        """
        # Request message ID
        req_msg_id = read_data(res, URPC_TYPE_U16)
        # Function handle
        handle = read_data(res, URPC_TYPE_U16)
        # Invoke callback
        self._invoke_callback(req_msg_id, handle)
    def _handle_call(self, req, msg_id):
        """
        u-RPC function call handler.

        :param req: Request message stream
        :param msg_id: Request message ID
        :returns: A response message
        """
        # Function handle
        handle = read_data(req, URPC_TYPE_U16)
        # Arguments and signature
        sig_args = read_vary(req)
        args = self._unmarshall(req, sig_args)
        # Lookup for function in store
        func = seq_get(self._funcs_store, handle)
        if not func:
            raise URPCError(URPC_ERR_NONEXIST)
        # Call function
        sig_rets, result = func(sig_args, args)
        if len(result)!=len(sig_rets):
            raise URPCError(URPC_ERR_SIG_INCORRECT)
        # Response message
        res = self._build_header(URPC_MSG_CALL_RESULT, "recv")
        write_data(res, msg_id, URPC_TYPE_U16)
        # Return values and signature
        write_vary(res, sig_rets)
        self._marshall(res, sig_rets, result)
        return res
    def _handle_call_result(self, res, msg_id):
        """
        u-RPC error result handler.

        :param res: Response message stream
        :param msg_id: Request message ID
        :returns: A response message
        """
        # Request message ID
        req_msg_id = read_data(res, URPC_TYPE_U16)
        # Result and signature
        sig_rets = read_vary(res)
        result = self._unmarshall(res, sig_rets)
        # Invoke callback
        self._invoke_callback(req_msg_id, result)
    def add_func(self, func, arg_types=None, ret_types=None, name=None):
        """
        Add a function to u-RPC instance.

        :param func: Function to be added
        :param name: Name of the function (Optional)
        :returns: Handle for the object
        :raises URPCError: If there is no more space for the function
        """
        # Arguments and results types
        if arg_types==None:
            arg_types = getattr(func, "__urpc_arg_types", None)
        if ret_types==None:
            ret_types = getattr(func, "__urpc_ret_types", None)
        # Wrap Python function as u-RPC function
        if arg_types!=None and ret_types!=None:
            func = urpc_wrap(arg_types, ret_types, func)
        # Add function to functions store
        handle = self._funcs_store.add(func)
        # Add function to name lookup
        if name:
            self._func_name_lookup[name] = handle
        # Return handle
        return handle
    def remove_func(self, handle):
        """
        Remove function from u-RPC instance and function lookup table.

        :param handle: Handle for the function
        :raises URPCError: If the handle does not correspond to a function
        """
        # Remove function from functions store
        del self._funcs_store[handle]
        # Remove function from name lookup
        if handle in self._func_name_lookup.inv:
            del self._func_name_lookup.inv[handle]
    def query(self, func_name, callback=None):
        """
        Query u-RPC function handle.

        :param func_name: Function name
        :param callback: Called when query completed
        """
        # Decorator style
        if not callback:
            return lambda _callback: self.query(func_name, _callback)
        # Build u-RPC message
        msg_id = self._counters["send"]
        req = self._build_header(URPC_MSG_FUNC_QUERY, "send")
        # Function name length and function name
        write_vary(req, func_name.encode("utf-8"))
        # Operation callback
        self._oper_callbacks[msg_id] = callback
        # Send request message
        self._send_callback(req.getvalue())
    def call(self, handle, sig_args, args, callback=None):
        """
        Do u-RPC call.

        :param handle: Remote function handle
        :param sig_args: Signature of arguments
        :param args: Arguments
        :param sig_rets: Signature of return values
        :param callback: Called when u-RPC call completed
        """
        # Decorator style
        if not callback:
            return lambda _callback: self.call(handle, sig_args, args, _callback)
        # Build u-RPC message
        msg_id = self._counters["send"]
        req = self._build_header(URPC_MSG_CALL, "send")
        # Function handle
        write_data(req, handle, URPC_TYPE_U16)
        # Arguments and types transform
        for i in range(len(sig_args)):
            t = sig_args[i]
            # URPCType subclass argument
            if isinstance(t, type) and issubclass(t, URPCType):
                t = t()
            # URPCType instance
            if isinstance(t, URPCType):
                args[i] = t.dumps(args[i])
                sig_args[i] = t.underlying_type
        # Arguments signature and arguments
        write_vary(req, sig_args)
        self._marshall(req, sig_args, args)
        # Operation callback
        self._oper_callbacks[msg_id] = callback
        # Send request message
        self._send_callback(req.getvalue())
    def recv_callback(self, data):
        """
        Callback function for incoming u-RPC messages.

        :param data: u-RPC message data (in bytes)
        """
        # Request message stream
        req = BytesIO(data)
        # Handle message
        res = self._handle_msg(req)
        # Send response message
        if res:
            self._send_callback(res.getvalue())

# u-RPC message handlers
_urpc_msg_handlers = [
    URPC._handle_error, # URPC_MSG_ERROR
    URPC._handle_func_query, # URPC_MSG_FUNC_QUERY
    URPC._handle_func_resp, # URPC_MSG_FUNC_RESP
    URPC._handle_call, # URPC_MSG_CALL
    URPC._handle_call_result, # URPC_MSG_CALL_RESULT
]
