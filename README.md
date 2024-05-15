
# BCNet
![BCNet Logo](img/logo.png?raw=true)

BCNet is a high-level game networking library written in C++. 

This library utilises Valve’s “GameNetworkingSockets” and includes simple modular interfaces for both client and server applications along with additional utilities allowing the user to easily handle and transmit packets across a network for their game or application.

The library can be built as either a static or a shared library, as long as the proper defines are configured with the compiler. 

Currently, it can only be built as an x64 library and only using Visual Studio, there is currently no support for GCC or LLVM/Clang, but it can be added in a later build.

Due to the dependencies, it is currently required to have their DLLs next to the application whether it is linking against the shared library or not, in a future build this may be fixed as it requires the dependencies to be built as a static library.

As stated before, this library relies on “GameNetworkingSockets” and the dependencies that come with it. Due to the nature of this library, networking with this API is TCP style.

# API

The main two headers to include are “IBCNetClient.h” and “IBCNetServer.h”, these contain the interfaces for the client and server, it is not necessary to include both as they act independently.

All functions and classes are within the “BCNet” namespace.

The functionality of the API allows for the creation of a server and client, where the server listens in on a configured port so that it can establish a connection with a client. Both the server and the client can transmit packets across this connection using the provided methods and handle the packets accordingly. 

The application uses the interfaces provided to do so and can also configure certain callbacks to handle packets or changes to the connection status, making the API extremely modular.

Additionally, the API provides a user command system, allowing interaction between the applications across the network by just executing a certain command within the applications console, custom commands can be added to the system utilizing callbacks, and utilities have been provided to help dealing with command parameters. 

There are default commands for both the server and client such as: “/kick user” for the server, and “/connect ip port”, “/disconnect”, “/whosonline”, and more for the client.

# Integration

Currently, building the library is as simple as opening the Visual Studio solution and building it. There is no support for other build systems currently. 

To use the library in your application, you will need to add the library’s include directory to your compilers additional include paths, and then link against the appropriate .lib within the linker settings. 

Alternatively, if you are using Visual Studio, you can add either the “BCNet” or “BCNet_static” project into your solution and reference it, making sure to still add it to the include paths. 

If you are linking against the static version of the library, you will need to add “BCNET_API_STATIC” to your compilers defines.

The most minimum version of the C++ Standard the library supports is C++17, though it is recommended to build the library and your application using the latest standard.

## Troubleshooting
- If your solution doesn’t build the first time when referencing the library’s project with your application, then try doing a rebuild of the application only.
