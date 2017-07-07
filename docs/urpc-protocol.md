# u-RPC Protocol Design

* u-RPC types
  - 0x00: 1-byte data
  - 0x01: 2-byte data
  - 0x02: 4-byte data
  - 0x03: 8-byte data
  - 0x04: Variable length data (2-byte length + data)
  - 0x05: Remote object (2-byte handle)
  - 0x06: Remote function (2-byte handle)
* u-RPC signature
  - 1-byte arguments length
  - Arguments type
  - 1-byte return values length
  - Return values type
* u-RPC message format
  - 2-byte magic string: `ur`
  - 1-byte version: 0x00
  - 2-byte message ID
  - 1-byte type
    + 0x00: u-RPC error response
      * 2-byte message ID
      * 1-byte error code
    + 0x01: u-RPC function query
      * 1-byte function name length
      * Function name
    + 0x02: u-RPC function query response
      * 2-byte message ID
      * 2-byte function handle
    + 0x03: u-RPC function call
      * 2-byte function handle
      * Arguments signature
      * Arguments
    + 0x04: u-RPC function call result
      * 2-byte request message ID
      * Result signature
      * Result
