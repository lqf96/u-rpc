from __future__ import absolute_import, unicode_literals
import struct
from collections import Iterator, Sequence, namedtuple
from six.moves import range

from urpc.constants import urpc_type_repr, urpc_type_size

## Spare table item error prompt
PROMPT_ERR_SPARE_TABLE_ITEM = "Index does not correspond to any value."
## Full allocation table prompt
PROMPT_ERR_TABLE_FULL = "The table is full."

## Allocation table item class
_AllocTableItem = namedtuple("_AllocTableItem", ["spare", "next", "data"])

class AllocTable(Sequence):
    """!
    @brief The allocation table data structure.
    """
    def __init__(self, capacity):
        """!
        @brief Initialize the allocation table.

        @param capacity Capacity of the allocation table.
        """
        ## Capacity of elements
        self._capacity = capacity
        ## Number of elements
        self._size = 0
        ## Begin of the spare table items linked list
        self._spare_begin = 0
        ## Internal data store
        self._store = [_AllocTableItem(True, i, None) for i in range(1, capacity+1)]
    def __len__(self):
        """!
        @brief Get number of elements in the table.

        @return Number of elements in the table.
        """
        return self._size
    def __getitem__(self, index):
        """!
        @brief Get value by index.

        @param index Index of the value.
        @return The value.
        """
        item = self._store[index]
        # Spare table item
        if item.spare:
            raise IndexError(PROMPT_ERR_SPARE_TABLE_ITEM)
        # Return data
        return item.data
    def __setitem__(self, index, value):
        """!
        @brief Set value by index.

        @param index Index of the value.
        @param value New value to set.
        """
        item = self._store[index]
        # Spare table item
        if item.spare:
            raise IndexError(PROMPT_ERR_SPARE_TABLE_ITEM)
        # Set data
        item._replace(data=value)
    def __delitem__(self, index):
        """!
        @brief Remove a value from the table by its index.

        @param index Index of the value
        """
        item = self._store[index]
        # Spare table item
        if item.spare:
            raise IndexError(PROMPT_ERR_SPARE_TABLE_ITEM)
        # Update next spare item
        self._store[index] = _AllocTableItem(True, self._spare_begin, None)
        self._spare_begin = index
        # Update size
        self._size -= 1
    def add(self, value):
        """!
        @brief Add a value to the table and return its corresponding index.

        @param value Value to be added.
        @return Index of the value.
        """
        # Store is full
        if self._size>=self._capacity:
            raise MemoryError(PROMPT_ERR_TABLE_FULL)
        # Next spare item
        index = self._spare_begin
        # Update next spare table item
        self._spare_begin = self._store[index].next
        self._store[index] = _AllocTableItem(False, None, value)
        # Update size
        self._size += 1
        # Return corresponding index
        return index
    def get(self, index, default=None):
        """!
        @brief Get value by index.

        (Fallback to default if index corresponds with a spare item)

        @param index Index of the value.
        @param default Default value if item is spare.
        @return The value.
        """
        item = self._store[index]
        # Spare table item; fallback to default value
        if item.spare:
            return default
        # Return data
        else:
            return item.data

def seq_get(seq, index, default=None):
    """!
    @brief Get element from sequence by index.

    (Fallback to default if index is out of range)

    @param seq Sequence.
    @param index Index of the element.
    @param default Fallback default value.
    @return Element if index is valid, otherwise default value.
    """
    # Valid index
    if isinstance(index, int) and index>=0 and index<len(seq):
        return seq[index]
    # Fallback to default value
    else:
        return default

def read_data(stream, urpc_type):
    """!
    @brief Read data of given type from stream.

    @param stream Data stream.
    @param urpc_type u-RPC data type.
    @return Read data in given type.
    """
    return struct.unpack(
        urpc_type_repr[urpc_type],
        stream.read(urpc_type_size[urpc_type])
    )[0]

def read_vary(stream, data_size_type="B"):
    """!
    @brief Read variable length data from stream.

    @param stream Data stream.
    @param data_size_type Type of data size in Python's struct module representation.
    @return Data in byte array.
    """
    # Read data size
    data_size_size = struct.calcsize(data_size_type)
    data_size = struct.unpack(data_size_type, stream.read(data_size_size))[0]
    # Read data
    return bytearray(stream.read(data_size))

def write_data(stream, data, urpc_type):
    """!
    @brief Write data of given type to stream.

    @param stream Data stream.
    @param data Data to write to stream.
    @param urpc_type u-RPC data type.
    """
    stream.write(struct.pack(
        urpc_type_repr[urpc_type],
        data
    ))

def write_vary(stream, data, data_size_type="B"):
    """!
    @brief Write variable length data to stream.

    @param stream Data stream.
    @param data Data to write to stream.
    @param data_size_type Type of data size in Python's struct module representation.
    """
    data_size = len(data)
    # Data length check
    data_size_size = struct.calcsize(data_size_type)
    if data_size>=2**(data_size_size*8):
        raise URPCError(URPC_ERR_TOO_LONG)
    # Write to stream
    stream.write(struct.pack(data_size_type, data_size))
    stream.write(bytearray(data))
