# The server
This is the rudimentary server app, it handles connections and message exchange
between a variable number of clients. 

It uses `SDL2_net` with `protlib`.

The server actually doesn't use multithreading, it handles all actions on 
the main thread using non-blocking socket sets.

The app isn't interactive, it only logs useful info to the console until you
close it.

## Compiling
The server can be easily compiled with the `Makefile`.
Make sure that you have compiled the protlib in the `protlib` directory
and that you have `SDL2` with `SDL_net 2.0` installed. 
Then you just have to set the `SDL_CONFIG` environment variable to the 
path of the `sdl2-config` file (defaulted to `/usr/local/bin/sdl2-config`)
and `make`.