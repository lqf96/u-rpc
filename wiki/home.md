# Home
Welcome to the u-RPC wiki home!

## What is u-RPC?
u-RPC is a remote procedure call (RPC) framework that enables communication between embedded devices and computers.

## How do I use it?
* Server: The server side of the framework is implemented as a Python package. To install it, go to [`python`](https://github.com/lqf96/u-rpc/tree/master/python) directory and run `./setup.py install` there.
* Client: There are two ways to use the client side code.
  - Using the built library  
  You can build the u-RPC static library by running `make static` or the dynamic library by running `make dynamic` inside the [`c`](https://github.com/lqf96/u-rpc/tree/master/c) directory. To build your own program, remember to add the [`c`](https://github.com/lqf96/u-rpc/tree/master/c) directory to the include and library directories. Here is a simple command line example:

  ```sh
  cc -I/path/to/urpc/c -o example example.c -L/path/to/urpc/c -lurpc
  ```

  - Using the source code directly  
  Alternatively you can directly use the u-RPC source code from the [`c`](https://github.com/lqf96/u-rpc/tree/master/c) directory. You can omit [`c/wio-shim.c`](https://github.com/lqf96/u-rpc/tree/master/c/wio-shim.c) by providing your own WIO API implementation.

## How do I test it?
To run the test go to [`python`](https://github.com/lqf96/u-rpc/tree/master/python) directory and run `./setup.py test` there. Currently the test only covers server side code, and we plan to add client side test code in the future.

## API Documentation
The API descriptions are written in Javadoc-like format and the documentation is generated with Doxygen.
* [C API](https://lqf96.github.io/u-rpc/c/html/index.html)
* [Python API](https://lqf96.github.io/u-rpc/python/html/index.html)

## License
The project is licensed under [MIT License](https://github.com/lqf96/u-rpc/tree/master/LICENSE).
