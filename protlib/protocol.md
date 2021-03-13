# The protocol
The protlib uses a very simple custom protocol.  

All data transfer starts with the head, which is always 3 ASCII letters, 
and is optionally followed by a sequence of arguments, every argument must be 
null-terminated. Every message must be terminated by a null character, you
can also imagine this as an empty argument at the end of every message.
Because of this, the protocol doesn't support empty arguments.

Here are some example messages (`\0` represents the __NULL__ character):  
```
HEDFirst argument\0Second Argument\0The message has to be terminated by one extra NULL character\0\0
```
or
```
DCNThis message has only one argument\0\0
```
or
```
CON\0
```