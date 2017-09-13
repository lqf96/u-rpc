# Protocol Design

## u-RPC Data Types
u-RPC currently supports following data types:  
* `0x00`: Signed 8-bit data
* `0x01`: Unsigned 8-bit data
* `0x02`: Signed 16-bit data
* `0x03`: Unsigned 16-bit data
* `0x04`: Signed 32-bit data
* `0x05`: Unsigned 32-bit data
* `0x06`: Signed 64-bit data
* `0x07`: Unsigned 64-bit data
* `0x08`: Variable length data
  - The variable length data is represented by prepending a 2-byte length of the data before the data itself when serialized.

## u-RPC Signature
u-RPC signature represents the amount and the types of RPC arguments and return values. It consists of a 1-byte number of arguments and the types of arguments.  
For example, for function `f` with type `(uint8_t, uint16_t) -> int16_t`, the signature of the arguements will be `\x02\x01\x03` and the signature of the results will be `\x01\x02`.

## u-RPC Message Types
* `0x00`: Error Response Message  
The error response message is used for indicating that a corresponding request has failed.
* `0x01`: Function Query Message  
The function query message is used for querying the handle of a remote function.
* `0x02`: Function Query Response Message  
The function query response message is used for replying a function query and contains the 16-bit handle for the corresponding function name.
* `0x03`: Function Call Message  
The function call message is used for issuing a remote procedure call.
* `0x04`: Function Call Result Message  
The function call result message is used for replying the function call message and contains call results.

## u-RPC Message Formats
* Message Header (Common for all types of messages)
  - 4-bit magic: `0b1010` (10)
  - 4-bit protocol version (Currently 1)
  - 2-byte message ID
* `0x00`: Error Response Message
  - 2-byte request message ID
  - 1-byte error code
* `0x01`: Function Query Message
  - 1-byte function name length
  - Function name
* `0x02`: Function Query Response Message
  - 2-byte request message ID
  - 2-byte function handle
* `0x03`: Function Call Message
  - 2-byte function handle
  - Signature of arguments
  - Arguments
* `0x04`: Function Call Result Message
  - 2-byte request message ID
  - Signature of results
  - Results
