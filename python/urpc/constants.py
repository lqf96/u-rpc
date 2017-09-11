from __future__ import absolute_import, unicode_literals

# u-RPC protocol magic
URPC_MAGIC = 10
# u-RPC protocol version
URPC_VERSION = 1

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
I8 = URPC_TYPE_I8 = 0x00
# Unsigned 8-bit data
U8 = URPC_TYPE_U8 = 0x01
# Signed 16-bit data
I16 = URPC_TYPE_I16 = 0x02
# Signed 8-bit data
U16 = URPC_TYPE_U16 = 0x03
# Signed 32-bit data
I32 = URPC_TYPE_I32 = 0x04
# Unsigned 32-bit data
U32 = URPC_TYPE_U32 = 0x05
# Signed 64-bit data
I64 = URPC_TYPE_I64 = 0x06
# Unsigned 64-bit data
U64 = URPC_TYPE_U64 = 0x07
# Variable length data
VARY = URPC_TYPE_VARY = 0x08

# Operation successfully completed
URPC_OK = 0x00
# Incorrect function signature
URPC_ERR_SIG_INCORRECT = 0x20
# Nonexist handle
URPC_ERR_NONEXIST = 0x21
# Operation not supported
URPC_ERR_NO_SUPPORT = 0x22
# Store is full
URPC_ERR_NO_MEMORY = 0x23
# Broken u-RPC message
URPC_ERR_BROKEN_MSG = 0x24
# Function call throws exception
URPC_ERR_EXCEPTION = 0x25
# Data too long
URPC_ERR_TOO_LONG = 0x26

# u-RPC type representation for struct module
urpc_type_repr = [
    "b", # URPC_TYPE_I8
    "B", # URPC_TYPE_U8
    "h", # URPC_TYPE_I16
    "H", # URPC_TYPE_U16
    "i", # URPC_TYPE_I32
    "I", # URPC_TYPE_U32
    "l", # URPC_TYPE_I64
    "L", # URPC_TYPE_U64
    None, # URPC_TYPE_VARY
    "H", # URPC_TYPE_FUNC
]
# u-RPC type to size mapping
urpc_type_size = [
    1, # URPC_TYPE_I8
    1, # URPC_TYPE_U8
    2, # URPC_TYPE_I16
    2, # URPC_TYPE_U16
    4, # URPC_TYPE_I32
    4, # URPC_TYPE_U32
    8, # URPC_TYPE_I64
    8, # URPC_TYPE_U64
    None, # URPC_TYPE_VARY
    2, # URPC_TYPE_FUNC
]
