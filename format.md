# The message format
The project uses `protlib`, which uses a custom protocol described in [its README](protlib/protocol.md).
This is merely a description of the format that the `server` and `client` use for communication.

Note that the format of messages with the same head 
may have a different meaning and structure depending on whether it comes from the client or the server.

These are all the message formats:

| Head | From a client | From the server |
|---|---|---|
|`ACC`||__0 arguments__<br>Connection accepted|
|`REF`||__0 arguments__<br>Connection refused|
|`MSG`|__1 argument__<br>The message to be sent to everyone|__2 arguments__<br>The sender of the message,<br>The text of the message|
|`NIC`|__1 argument__<br>Request to use a nick|__1 argument__<br>The new nick assigned/confirmed by the server|

There are obviously ways to optimise this (such as caching nicknames client-side), one example
optimisation that I made is actually the `NIC` message, which caches nicks on the server side.

## Examples 
(`\0` represents the __NULL__ character)

__Client -> Server__
* `NICJacob\0\0` - Change my nick to `Jacob`
* `MSGHello, world!\0\0` - Send the message `Hello, world!` to everyone under my nickname

__Server -> Client__
* `ACC\0` - I accept your connection
* `NICGuest\0\0` - I assign you the nick of `Guest1`
* `MSGJacob\0Hello, world!\0\0` - `Jacob` has sent the message `Hello, world!`