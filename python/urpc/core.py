from __future__ import unicode_literals
from io import BytesIO
import struct, weakref

# u-RPC protocol version
URPC_VERSION = 0
# u-RPC magic ("ur")
URPC_MAGIC = 29301

# Error message
URPC_MSG_ERROR = 0
# Function query message
URPC_MSG_FUNC_QUERY = 1
# Function query response message
URPC_MSG_FUNC_RESP = 2
# Function call message
URPC_MSG_CALL = 3
# Function call result message
URPC_MSG_CALL_RESULT = 4

# Signed 8-bit data
URPC_TYPE_I8 = 0x00
# Unsigned 8-bit data
URPC_TYPE_U8 = 0x01
# Signed 16-bit data
URPC_TYPE_I16 = 0x02
# Signed 8-bit data
URPC_TYPE_U16 = 0x03
# Signed 32-bit data
URPC_TYPE_I32 = 0x04
# Unsigned 32-bit data
URPC_TYPE_U32 = 0x05
# Signed 64-bit data
URPC_TYPE_I64 = 0x06
# Unsigned 64-bit data
URPC_TYPE_U64 = 0x07
# Variable length data
URPC_TYPE_VARY = 0x08
# Remote object handle
URPC_TYPE_OBJ = 0x09
# Callback handle
URPC_TYPE_CALLBACK = 0x0a

# Operation successfully completed
URPC_OK = 0x00
# Incorrect function signature
URPC_ERR_SIG_INCORRECT = 0x01
# Nonexist handle
URPC_ERR_NONEXIST = 0x02
# Operation not supported
URPC_ERR_NO_SUPPORT = 0x03
# Store is full
URPC_ERR_STORE_FULL = 0x04
# Broken u-RPC message
URPC_ERR_BROKEN_MSG = 0x05
# Function call throws exception
URPC_ERR_EXCEPTION = 0x06
# Data too long
URPC_ERR_TOO_LONG = 0x07

def read_data(stream, urpc_type):
    return struct.unpack(
        _urpc_type_repr[urpc_type],
        stream.read(_urpc_type_size[urpc_type])
    )[0]

def read_vary(stream):
    """
    Read variable length data from stream.
    The data should have a 1-byte size, followed by bytes of that size.

    :param stream: Data stream
    :returns: Data in byte array
    """
    data_size = ord(stream.read(1))
    return bytearray(stream.read(data_size))

def write_data(stream, data, urpc_type):
    stream.write(struct.pack(
        _urpc_type_repr[urpc_type],
        data
    ))

def write_vary(stream, data):
    data_size = len(data)
    # Data length check
    if data_size>=2**8:
        raise URPCError(URPC_ERR_TOO_LONG)
    # Write to stream
    stream.write(bytearray([data_size]))
    stream.write(bytearray(data))

def list_get(l, index, default=None):
    if isinstance(index, int) and index>=0 and index<len(l):
        return l[index]
    else:
        return default

class _URPCSlot(object):
    def __init__(self, next_slot):
        self.next_slot = next_slot

class URPCError(BaseException):
    def __init__(self, reason):
        self.reason = reason

class URPC(object):
    def __init__(self, send_callback, n_funcs=256, n_objs=256):
        # Functions store (Handle to function lookup)
        self._funcs_begin = 0
        self._funcs_size = n_funcs
        self._funcs_store = [_URPCSlot(i) for i in range(1, n_funcs+1)]
        # Function name and handle lookups (Bidirectional)
        self._name_lookup = {}
        self._rev_name_lookup = {}
        # Objects store (Handle to object reference lookup)
        self._objects_begin = 0
        self._objects_size = n_objs
        self._objects_store = [_URPCSlot(i) for i in range(1, n_objs+1)]
        # Object reference to handle lookup
        self._objects_lookup = {}
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
    def _build_error_msg(self, msg_id, error_code):
        """
        Build an error message with given error code.

        :param error_code: Error code
        :returns: Error message stream
        """
        res = self._build_header(URPC_MSG_ERROR, "recv")
        # Write request message ID and error code
        write_data(res, msg_id, URPC_TYPE_U16)
        write_data(res, error_code, URPC_TYPE_U16)
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
            # Remote object
            elif obj_type==URPC_TYPE_OBJ:
                handle = self._objects_lookup.get(weakref.ref(obj))
                if not handle:
                    handle = self.add_object(obj)
                write_data(stream, handle, URPC_TYPE_U16)
            # TODO: Callback (currently not supported)
            elif obj_type==URPC_TYPE_CALLBACK:
                raise URPCError(URPC_ERR_NO_SUPPORT)
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
            # Remote object
            elif obj_type==URPC_TYPE_OBJ:
                obj = read_data(stream, URPC_TYPE_OBJ)
                obj_ref = list_get(self._objects_store, obj)
                if isinstance(obj_ref, _URPCSlot):
                    raise URPCError(URPC_ERR_NONEXIST)
                obj = obj_ref()
            # TODO: Callback (currently not supported)
            elif obj_type==URPC_TYPE_CALLBACK:
                raise URPCError(URPC_ERR_NO_SUPPORT)
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
            return self._build_error_msg(msg_id, e.reason)
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
        name = read_vary(req)
        # Search for function in function lookup
        handle = self._funcs_lookup.get(name)
        if not handle:
            raise URPCError(URPC_ERR_NONEXIST)
        # Response message
        res = self._build_header(URPC_MSG_FUNC_RESP, "recv")
        write_data(res, msg_id, URPC_TYPE_U16)
        write_data(res, handle, URPC_TYPE_U16)
        return res
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
        func = self._funcs_store[handle]
        if not func or isinstance(func, _URPCSlot):
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
    def add_func(self, func, name=None):
        """
        Add a function to u-RPC instance.

        :param func: Function to be added
        :param name: Name of the function (Optional)
        :returns: Handle for the object
        :raises URPCError: If there is no more space for the function
        """
        handle = self._funcs_begin
        # Functions store is full
        if handle==self._funcs_size:
            raise URPCError(URPC_ERR_STORE_FULL)
        # Add function to store and update next available item
        self._funcs_begin = self._funcs_store[handle].next_slot
        self._funcs_store[handle] = func
        # Add function to name lookup tables
        if name:
            self._name_lookup[name] = handle
            self._rev_name_lookup[handle] = name
        # Return handle
        return handle
    def remove_func(self, handle):
        """
        Remove function from u-RPC instance and function lookup table.

        :param handle: Handle for the function
        :raises URPCError: If the handle does not correspond to a function
        """
        func = self._funcs_store[handle]
        # Attempt to remove nonexistant handle
        if not func or isinstance(obj_ref, _URPCSlot):
            raise URPCError(URPC_ERR_NONEXIST)
        # Remove function from name lookup tables
        name = self._rev_name_lookup.get(handle)
        if name:
            del self._name_lookup[name]
            del self._rev_name_lookup[handle]
        # Update next available item
        self._funcs_store[handle] = _URPCSlot(self._funcs_begin)
        self._funcs_begin = handle
    def add_object(self, obj):
        """
        Add an object to u-RPC instance.
        The URPC instance keeps a weak reference of the object,
        so it does not affect the garbage collection of the object.
        When the object no longer exists,
        the corresponding slot is automatically released.

        :param obj: Object to be added
        :returns: Handle for the object
        :raises URPCError: If there is no more space for the object
        """
        handle = self._objects_begin
        # Objects store is full
        if handle==self._objects_size:
            raise URPCError(URPC_ERR_STORE_FULL)
        # Update next available item
        self._objects_begin = self._objects_store[handle].next_slot
        # Get weak reference of the object
        def finalize_callback(obj_ref):
            if self._objects_store[handle]==obj_ref:
                self.remove_object(handle)
        obj_ref = weakref.ref(obj, finalize_callback)
        # Add object to object store and lookup
        self._objects_store[handle] = obj_ref
        self._objects_lookup[obj_ref] = handle
        # Return handle
        return handle
    def remove_object(self, handle):
        """
        Remove an object from u-RPC instance by handle.

        :param handle: Handle for the object
        :raises URPCError: If the handle does not correspond to an object
        """
        obj_ref = self._objects_store[handle]
        # Attempt to remove nonexistant handle
        if isinstance(obj_ref, _URPCSlot):
            raise URPCError(URPC_ERR_NONEXIST)
        # Remove reference from object lookup
        del self._objects_lookup[obj_ref]
        # Update next available item of object store
        self._objects_store[handle] = _URPCSlot(self._objects_begin)
        self._objects_begin = handle
    def call(self, handle, sig_args, args, callback):
        """
        Do u-RPC call.

        :param handle: Remote function handle
        :param sig_args: Signature of arguments
        :param args: Arguments
        :param sig_rets: Signature of return values
        :param callback: Called when u-RPC call completed
        """
        msg_id = self._counters["send"]
        req = self._build_header(URPC_MSG_CALL, "send")
        # Function handle
        write_data(req, handle, URPC_TYPE_U16)
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
    None, # URPC_MSG_FUNC_RESP
    URPC._handle_call, # URPC_MSG_CALL
    URPC._handle_call_result, # URPC_MSG_CALL_RESULT
]
# u-RPC type representation for struct module
_urpc_type_repr = [
    "b", # URPC_TYPE_I8
    "B", # URPC_TYPE_U8
    "h", # URPC_TYPE_I16
    "H", # URPC_TYPE_U16
    "i", # URPC_TYPE_I32
    "I", # URPC_TYPE_U32
    "l", # URPC_TYPE_I64
    "L", # URPC_TYPE_U64
    None, # URPC_TYPE_VARY
    "H", # URPC_TYPE_OBJ
    "H", # URPC_TYPE_FUNC
]
# u-RPC type to size mapping
_urpc_type_size = [
    1, # URPC_TYPE_I8
    1, # URPC_TYPE_U8
    2, # URPC_TYPE_I16
    2, # URPC_TYPE_U16
    4, # URPC_TYPE_I32
    4, # URPC_TYPE_U32
    8, # URPC_TYPE_I64
    8, # URPC_TYPE_U64
    None, # URPC_TYPE_VARY
    2, # URPC_TYPE_OBJ
    2, # URPC_TYPE_FUNC
]
