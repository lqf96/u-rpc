# Getting Started
This tutorial will help you get started with the u-RPC framework step by step.

## Create Endpoints
The first step to use the u-RPC framework is to create caller and callee endpoints.

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

Or if you want to add u-RPC signature information without changing the function:

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

## Query Remote Function Handle
For the caller endpoint, we need to know the handle of the remote function before we can call them. While we can be certain of the function handles if we add functions in order, a better approach would be querying the function handle by function name.

For the Python API:

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

For the C API:

```c
//Handle of test function
urpc_func_t test_handle;

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

Somewhere else:

```c
//Query function handle
urpc_get_func(
    &caller,
    "test",
    NULL,
    cb
);
```

## Call Remote Function
Now that all the preparation work is done, we can start calling our test function from the caller endpoint. Below is the demo code for the Python API:

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

For the C API:

```c
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

Somewhere else:

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

For both Python API and C API, we do u-RPC call on the endpoint with function handle, types of arguments, arguments and a callback. When the callback is invoked we can check if the call succeeds or not, and if it succeeds we can then read the call result and move on.
