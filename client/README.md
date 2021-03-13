# The client
This is the client API, it exports a few simple C functions that
allow immediate communication with the server.

It uses `SDL2_net` with `protlib`.

The API is documented in the [header file itself](include/client.h "The client.h file").

## Compiling
The shared library can be easily compiled with the `Makefile`.
Make sure that you have compiled the protlib in the `protlib` directory
and that you have SDL2 with SDL_net 2.0 installed. 
Then you just have to set the `SDL_CONFIG` environment variable to the 
path of the `sdl2-config` file (defaulted to `/usr/local/bin/sdl2-config`)
and `make client.dll` or `make client.so`.