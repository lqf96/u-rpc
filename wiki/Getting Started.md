# Getting Started
This tutorial will help you get started with the u-RPC framework step by step.

## Create Endpoints
The first step to use the u-RPC framework is to create caller and callee endpoints, and that can be done with the [`URPC`](https://lqf96.github.io/u-rpc/python/html/classurpc_1_1endpoint_1_1_u_r_p_c.html#ac67b6ff4acdaef0a911eacc435e87ab8) class for the Python API and [`urpc_init()`](https://lqf96.github.io/u-rpc/c/html/urpc_8h.html#a8ff65c1c85fadb1a6dff3ebaf037c65a) for the C API.

For the callee endpoint, suppose we have a function `send_func` that takes bytes data and sends it to the caller endpoint. We create the callee endpoint by calling the constructor:

```python
# Callee endpoint
callee = URPC(send_callback=send_func)
```

When we receive any u-RPC message data from the callee endpoint, we call [`URPC.recv_callback()`](https://lqf96.github.io/u-rpc/python/html/classurpc_1_1endpoint_1_1_u_r_p_c.html#ac563e328e5a500a089db5e90c597eef5) with the data we received.

For the caller endpoint the process is similar for the Python API, except `send_func` sends data to the callee endpoint and [`URPC.recv_callback()`](https://lqf96.github.io/u-rpc/python/html/classurpc_1_1endpoint_1_1_u_r_p_c.html#ac563e328e5a500a089db5e90c597eef5) should be called with data coming from the callee endpoint.

For the C API, the caller endpoint is invoked with [`urpc_init()`](https://lqf96.github.io/u-rpc/c/html/urpc_8h.html#a8ff65c1c85fadb1a6dff3ebaf037c65a):

```c
//Caller endpoint
urpc_t caller;

//Initialize caller endpoint
urpc_init(
    &caller,
    //Function table size
    //(Currently not implemented)
    16,
    //Send buffer size
    96,
    //Temporary buffer size
    32,
    //Send function and closure data
    NULL,
    send_func,
    //Capacity of callback table
    8
);
```

Similar to the Python API, here `send_func` sends data to the callee endpoint.

## Add Functions to Endpoint
For the callee endpoint, we then add functions to the endpoint to make it available for others to call. Let's add a test function that increases the only argument by 1:

```python
def test(arg_types, args):
    # Make sure the only argument is uint16_t
    assert len(arg_types)==1 and arg_types[0]==urpc.U16
    # Increase argument by 1
    args[0] += 1
    # Return results with corresponding type information
    return [urpc.U16], args

# Add test function to endpoint
callee.add_func(
    func=test,
    name="test"
)
```

For each u-RPC function, it receives the types and the values of the arguments, and it returns the types and the values of the results.

For functions with fixed signature, we can pass the signature of the function to the framework and let it check the types of the arguments for us:

```python
def test(x):
    return x+1

# Use "urpc_wrap()"
callee.add_func(
    func=urpc.urpc_wrap([urpc.U16], [urpc.U16], test),
    name="test_1"
)

# Pass arguments and results types when adding function
callee.add_func(
    func=test,
    name="test_2",
    arg_types=[urpc.U16],
    ret_types=[urpc.U16]
)
```

Or if you only want to attach u-RPC signature information to the function without modifying it:

```python
@urpc.urpc_sig([urpc.U16], [urpc.U16])
def test(x):
    return x+1

# The function isn't changed
assert test(1)==2

callee.add_func(
    func=test,
    name="test_3"
)
```

## High-level u-RPC types
Apart from the [basic types](Protocol-Design#u-rpc-data-types), the u-RPC framework also supports high-level data types. Each high-level data type has an underlying basic data type and describes how to serialize and deserialize data from its underlying type.

To create a high-level u-RPC type, we inherit from [`URPCType`](https://lqf96.github.io/u-rpc/python/html/classurpc_1_1misc_1_1_u_r_p_c_type.html), implements [`loads()`](https://lqf96.github.io/u-rpc/python/html/classurpc_1_1misc_1_1_u_r_p_c_type.html#a2dd2b9efe960027fd1dac58e4cb8f488) and [`dumps()`](https://lqf96.github.io/u-rpc/python/html/classurpc_1_1misc_1_1_u_r_p_c_type.html#adb333b6a56081ab076279d131d4cf544) for serialization and deserialization, and declare its underlying type:

```python
# Simplified u-RPC string type
class StringType(urpc.URPCType):
    # Constructor
    def __init__(self, encoding="utf-8"):
        # Encoding
        self.encoding = encoding
    # Deserializer
    def loads(self, data):
        return text_type(data, encoding=self.encoding)
    # Serializer
    def dumps(self, value):
        return bytearray(value.encode(self.encoding))
    # Underlying type
    underlying_type = URPC_TYPE_VARY
```

The above code shows how the [`StringType`](https://lqf96.github.io/u-rpc/python/html/classurpc_1_1misc_1_1_string_type.html) works. It can be used to represent a string on the callee endpoint.

To use high-level types, just replace basic types with high-level types or their instances in the function signature:

```python
@urpc.urpc_sig([urpc.StringType], [urpc.U16])
def str_len(string):
    return len(string)
```

## Query Remote Function Handle
For the caller endpoint, we need to know the handle of the remote function before we can call them. While we can be certain of the function handles if we add functions in order, a better approach would be querying the function handle by function name.

For the Python API we call [`URPC.query()`](https://lqf96.github.io/u-rpc/python/html/classurpc_1_1endpoint_1_1_u_r_p_c.html#a38dacd5128e9080ee7cf102e78d14081) to query corresponding function handle:

```python
# Error happened
def cb(e, handle):
    # Error happened
    if e:
        print("Error happened: %s" % e)
    # RPC succeeded
    else:
        test_handle = handle
        print("Handle for function test: %d" % handle)

test_handle = None
# Query function handle
caller.query("test", cb)
```

For the C API we call [`urpc_get_func()`](https://lqf96.github.io/u-rpc/c/html/urpc_8h.html#a86b105535e233b00e5482a7e3781dbf0):

```c
//Handle of test function
urpc_func_t test_handle;

//Query function handle callback
WIO_CALLBACK(cb) {
    //Error happened
    if (status)
        printf("Error happened: %d\n", status);
    //Query succeeded
    else {
        //Function handle
        test_handle = *((urpc_func_t*)result);

        printf("Handle for function test: %d", test_handle);
    }

    return WIO_OK;
}
```

```c
//Query function handle
urpc_get_func(
    &caller,
    "test",
    NULL,
    cb
);
```

For both Python and C API, we query the function handle on the endpoint with the function name. When the query succeeds the callback will be invoked with the function handle, or else it will receive an error code describing the error happened.

## Call Remote Function
Now that all the preparation work is done, we can start calling our test function from the caller endpoint. Below is the demo code for the Python API [`URPC.call()`](https://lqf96.github.io/u-rpc/python/html/classurpc_1_1endpoint_1_1_u_r_p_c.html#aa33f3e52a2bcd42da2cba8f524c9a4ae):

```python
def cb(e, result):
    # Error happened
    if e:
        print("Error happened: %s" % e)
    # RPC succeeded
    else:
        assert result[0]==2
        print("RPC call succeeded!")

# Do RPC
caller.call(test_handle, [urpc.U16], [1], cb)
```

For the C API, we do remote procedure call with [`urpc_call()`](https://lqf96.github.io/u-rpc/c/html/urpc_8h.html#a59a64304e183a9420a416c2a79196691):

```c
//RPC callback function
WIO_CALLBACK(cb) {
    //Error happened
    if (status)
        printf("Error happened: %d\n", status);
    //RPC succeeded
    else {
        //RPC results
        void** rpc_results = (void**)result;
        //The first return value
        //(rpc_results[0] points to the signature of return values)
        uint16_t* ret = (uint16_t*)rpc_results[1];

        if (*ret==2)
            printf("RPC call succeeded!\n");
    }

    return WIO_OK;
}
```

```c
//Argument
uint16_t arg = 1;
//Do RPC
urpc_call(
    &caller,
    test_handle,
    URPC_SIG(1, URPC_TYPE_U16),
    URPC_ARG(&arg),
    NULL,
    cb
);
```

For both Python API and C API, we do u-RPC call on the endpoint with function handle, types of arguments, arguments and a callback. When the callback is invoked we can check if the call succeeds or not, and if it succeeds we can then get the call result and move on.
