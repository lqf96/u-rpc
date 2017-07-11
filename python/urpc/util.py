from __future__ import unicode_literals
from collections import Iterator, Sequence, namedtuple
from six.moves import range

# Spare table item error prompt
PROMPT_ERR_SPARE_TABLE_ITEM = "Index does correspond to any value."
# Allocation table is full
PROMPT_ERR_TABLE_FULL = "The table is full."

# Allocation table item
_AllocTableItem = namedtuple("_AllocTableItem", ["spare", "next", "data"])

class AllocTable(Sequence):
    """ The allocation table data structure. """
    def __init__(self, capacity):
        """
        Initialize the allocation table.

        :param capacity: Maximum size of the allocation table
        """
        # Capacity and number of elements
        self._capacity = capacity
        self._size = 0
        # Begin of the spare table items linked list
        self._spare_begin = 0
        # Internal data store
        self._store = [_AllocTableItem(True, i, None) for i in range(1, i+1)]
    def __len__(self):
        """
        Get number of elements in the table.
        """
        return self._size
    def __getitem__(self, index):
        """
        Get value by index.

        :param index: Index of the value
        :returns: The value
        """
        item = self._store[index]
        # Spare table item
        if item.spare:
            raise IndexError(PROMPT_ERR_SPARE_TABLE_ITEM)
        # Return data
        return item.data
    def __setitem__(self, index, value):
        """
        Set value by index.

        :param index: Index of the value
        :param value: New value to set
        """
        item = self._store[index]
        # Spare table item
        if item.spare:
            raise IndexError(PROMPT_ERR_SPARE_TABLE_ITEM)
        # Set data
        item._replace(data=value)
    def __delitem__(self, index):
        """
        Remove a value from the table by its index.

        :param index: Index of the value
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
        """
        Add a value to the table and return its corresponding index.

        :param value: Value to be added
        :returns: Index of the value
        """
        # Store is full
        if self._size>=self._capacity:
            raise MemoryError(PROMPT_ERR_TABLE_FULL)
        # Next spare item
        index = self._spare_begin
        # Update next spare table item
        self._spare_begin = self._store[handle].next
        self._store[index] = _AllocTableItem(False, None, value)
        # Update size
        self._size += 1
        # Return corresponding index
        return index

def seq_get(seq, index, default=None):
    """
    Get element from sequence by index.
    Fallback to default if index is out of range.

    :param seq: Sequence
    :param index: Index of the element
    :param default: Fallback default value
    :returns Element if index is valid, otherwise default value
    """
    # Valid index
    if isinstance(index, int) and index>=0 and index<len(seq):
        return seq[index]
    # Fallback to default value
    else:
        return default
