# The protlib
The protlib (protocol library) is a key helper library used both
by the `server` and the `client`. It is used for consistent
communication using the custom protocol described in the 
[protocol README](protocol.md).

Protlib essentially just takes the socket data stream and packages it
into a struct. It technically doesn't have anything to do with chat
apps and is completely standalone, the meaning of the messages is
decided by the code that uses this library.

It uses `SDL_net` for TCP communication.

## Compiling
The static library can be easily compiled with the `Makefile`.
You just have to set the `SDL_CONFIG` environment variable to the 
path of the `sdl2-config` file (defaulted to `/usr/local/bin/sdl2-config`)
and `make`.