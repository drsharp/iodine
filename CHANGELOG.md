# Iodine
[![Gem Version](https://badge.fury.io/rb/iodine.svg)](https://badge.fury.io/rb/iodine)
[![Inline docs](http://inch-ci.org/github/boazsegev/iodine.svg?branch=master)](http://www.rubydoc.info/github/boazsegev/iodine/master/frames)

Please notice that this change log contains changes for upcoming releases as well. Please refer to the current gem version to review the current release.

## Changes:

***

Change log v.0.2.13 (next release)

**Credit** credit to Elia Schito (@elia) and Augusts Bautra (@Epigene) for fixing parts of the documentation (PR #11 , #12).

***

Change log v.0.2.12

**Fix** removed `mempool` after it failed some stress and concurrency tests.

***

Change log v.0.2.11

**Fix**: C layer memory pool had a race-condition that could have caused, in some unlikely events, memory allocation failure for Websocket protocol handlers. This had now been addressed and fixed.

**Experimental feature**: added an `each_write` feature to allow direct `write` operations that write data to all open Websocket connections sharing the same process (worker). When this method is called without the optional block, the data will be sent without the need to acquire the Ruby GIL.

**Update**: lessons learned from `facil.io` have been implemented for better compatibility of Iodine's core C layer.

***

Change log v.0.2.10

**Update**: added documentation and an extra helper method to set a connection's timeout when using custom protocols (Iodine as an EventMachine alternative).

**C Layer Update** updated the [`facil.io`](http://facil.io) library used, to incorporate the following fixes / update:

* Better cross platform compilation by avoiding some name-space clashes. i.e, fixes a name clash with the `__used` directive / macro, where some OSs (i.e. CentOS) used a similar directive with different semantics.

* Reviewed and fixed some signed vs. unsigned integer comparisons.

* Smoother event scheduling by increasing the event-node's pool size.

* Smoother thread concurrency growth by managing thread `nanosleep` times as thread count dependent.

* Cleared out "unused variable" warnings.

* Streamlined the `accept` process to remove a double socket's data clean-up.

* `SERVER_DELAY_IO` is now implemented as an event instead of a stack frame.

* Fixed a possible Linux `sendfile` implementation issue where sometimes errors wouldn't be caught or `sendfile` would be called past a file's limit (edge case handling).

* `bscrypt` random generator (where `dev/random` is unavailable) should now provide more entropy.


***

Change log v.0.2.9

**Fix**: fixed a gcc-4.8 compatibility issue that prevented iodine 0.2.8 from compiling on Heroku's cedar-14 stack. This was related to missing system include files in gcc-4.8. It should be noted that Heroku's stack and compiler (which utilizes Ubuntu 14) has known issues and / or limited support for some of it's published features... but I should have remembered that before releasing version 0.2.8... sorry about that.

***

Change log v.0.2.8

**Memory Performance**: The Websocket connection Protocol now utilizes both a C level memory pool and a local thread storage for temporary data. This helps mitigate possible memory fragmentation issues related to long running processes and long-lived objects. In addition, the socket `read` buffer was moved from the protocol object to a local thread storage (assumes pthreads and not green threads). This minimizes the memory footprint for each connection (at the expense of memory locality) and should allow Iodine to support more concurrent connections using less system resources. Last, but not least, the default message buffer per connection starts at 4Kb instead of 16Kb (grows as needed, up to `Iodine::Rack.max_msg_size`), assuming smaller messages are the norm.

**Housekeeping**: Cleaned up some code, removed old files, copied over the latest [`facil.io`](http://facil.io) library. There's probably some more housekeeping left to perform, especially anywhere where documentation is concerned. I welcome help with documentation.

***

Change log v.0.2.7

**Minor Fix**: fixed an issue where a negative number of processes or threads would initiate a very large number of forks, promoting a system resource choke. Limited the number of threads (1023) and processes (127).

**Update**: Automated the number of processes (forks) and threads used when these are not explicitly specified. These follow the number of cores / 2.

***

Change log v.0.2.6

**Update**: The IO reactor review will now be delayed until all events scheduled are done. This means that is events schedule future events, no IO data will be reviewed until all scheduled data is done. Foolish use might cause infinite loops that skip the IO reactor, but otherwise performance is improved (since the IO reactor might cause a thread to "sleep", delaying event execution).

***

Change log v.0.2.5

**Fix:**: fix for issue #9 (credit to Jack Christensen for exposing the issue) caused by an unlocked critical section's "window of opportunity" that allowed asynchronous Websocket `each` blocks to run during the tail of the Websocket handshake (while the `on_open` callback was running in parallel).

**Minor Fix**: Fix Iodine::Rack's startup message's `fprintf` call to fit correct argument sizes (Linux warnings).

***

Change log v.0.2.4

**Minor Fix**: Patched Iodine against Apple's broken `getrlimit` on macOS. This allows correct auto-setting of open file limits for the socket layer.

**Minor Fix**: Fixed the processor under-utilization warning, where "0" might be shown for the number processes instead of "1".

**Update**: Added support for the `env` keys `HTTP_VERSION` and `SERVER_PROTOCOL` to indicate the HTTP protocol version. Iodine implements an HTTP/1.1 server, so versions aren't expected to be higher than 1.x.

**Update**: Iodine::Rack startup messages now include details regarding open file limits imposed by the OS (open file limits control the maximum allowed concurrent connections and other resource limits).

***

Change log v.0.2.3

**Update**: The `write` system call is now deferred when resources allow, meaning that (as long as the `write` buffer isn't full) `write` is not only non-blocking, but it's performed as a separate event, outside of the Ruby GIL.

**Update**: The global socket `write` buffer was increased to ~16Mb (from ~4Mb), allowing for more concurrent `write` operations. However, the `write` buffer is still limited and `write` might block while the buffer is full. Blocking and "hanging" the server until there's enough room in the buffer for the requested `write` will slow the server down while keeping it healthy and more secure. IMHO, it is the lesser of two evils.

***

Change log v.0.2.2

**Update** The static file service now supports `ETag` caching, sending a 304 (not changed) response for valid ETags.

**Update**: A performance warning now shows if the CPUs are significantly under-utilized (less than half are used) of if too many are utilized (more than double the amount of CPUs), warning against under-utilization or excessive context switching (respectively).

***

Change log v.0.2.1

**Notice**: The [Rack Websocket Draft](https://github.com/rack/rack/pull/1107) does not support the `each` and `defer` methods. Although I tried to maintain these as part of the draft, the community preferred to leave the implementation of these to the client (rather then the server). If collisions occur, these methods might be removed in the future.

**Update**: Websockets now support the `has_pending?` method and `on_ready` callback, as suggested by the [Rack Websocket Draft](https://github.com/rack/rack/pull/1107).

**Update**: deprecated the websocket method `uuid` in favor of `conn_id`, as suggested by the [Rack Websocket Draft](https://github.com/rack/rack/pull/1107).

**Fix**: fixed an issue were the server would crash when attempting to send a long enough websocket message.

***

Change log v.0.2.0

This version is a total rewrite. The API is totally changed, nothing stayed.

Iodine is now written in C, as a C extension for Ruby. The little, if any, ruby code written is just the fluff and feathers.

***

### deprecation of the 0.1.x version line

Change log v.0.1.21

**Optimization**: Minor optimizations. i.e. - creates 1 less Time object per request (The logging still creates a Time object unless disabled using `Iodine.logger = nil`).

**Security**: Http/1 now reviews the Body's size as it grows (similar to Http/2), mitigating any potential attacks related to the size of the data sent.

**Logs**: Log the number of threads utilized when starting up the server.

***

Change log v.0.1.20

**Update/Fix**: Updated the `x-forwarded-for` header recognition, to accommodate an Array formatting sometimes used (`["ip1", "ip2", ...]`).

**Update**: native support for the `Forwarded` header Http.

**API Changes**: `Iodine::Http.max_http_buffer` was replaced with `Iodine::Http.max_body_size`, for a better understanding of the method's result.

***

Change log v.0.1.19

**Update**: added the `go_away` method to the Http/1 peorotocol, for seamless connection closeing across Http/2, Http/1 and Websockets.

***

Change log v.0.1.18

**Update**: The request now has the shortcut method `Request#host_name` for accessing the host's name (without the port part of the string).

***

Change log v.0.1.17

**Credit**: thanks you @frozenfoxx for going through the readme and fixing my broken grammer.

**Fix**: fixed an issue where multiple Pings might get sent when pinging takes time. Now pings are exclusive (run within their own Mutex).

**Fix**: Http/2 is back... sorry about breaking it in the 0.1.16 version. When I updated the write buffer I forgot to write the status of the response, causing a protocol error related with the headers. It's now working again.

**Update**: by default and for security reasons, session id's created through a secure connection (SSL) will NOT be available on a non secure connection (SSL/TLS). However, while upgrading to the encrypted connection, the non_encrypted session storage is now available for review using the `Response#session_old` method.

* Remember that sessions are never really safe, no matter how much we guard them. Session hijacking is far too easy. This is why Iodine stores the session data locally and not within the session cookie. This is also why you should review any authentication before performing sensitive tasks based on session stored authentication data.

***

Change log v.0.1.16

**Performance**: Http/1 and Http/2 connections now share and recycle their write buffer when while reading the response body and writing it to the IO. This (hopefuly) prevents excess `malloc` calls by the interperter.

***

Change log v.0.1.15

**Update**: IO reactor will now update IO status even when tasks are pending. IO will still be read only when there are no more tasks to handle, but this allows chained tasks to relate to the updated IO status. i.e. this should improve websocket availability for broadcasting (delay from connection to availability might occure until IO is registered).

**Update**: Websockets now support the `on_ping` callback, which will be called whenever a ping was sent without error.

***

Change log v.0.1.14

**Update**: the Response now supports `redirect_to` for both permanent and temporary redirection, with an optional `flash` cookie setup.

**Performance**: the Protocol class now recycles the data string as a thread global socket buffer (different threads have different buffer strings), preventing excessive `malloc` calls by the Ruby interpreter. To keep the `data` (in `on_message(data)`) past the `on_message` method's scope, be sure to duplicate it using `data.dup`, or the string's buffer will be recycled.

***

Change log v.0.1.13

**Change**: Session cookie lifetime is now limited to the browser's session. The local data will still persist until the tmp-folder is cleared (when using session file storage).

**Fix**: renamed the SSL session token so that the SSL session id isn't lost when a non-secure session is used.

**Fix**: The `flash` cookie-jar will now actively prevent Symbol and String keys from overlapping.

**Compatibility**: minor fixes and changes in preperation for Ruby 2.3.0. These may affect performance due to slower String initialization times.

***

Change log v.0.1.12

**Update**: Passing a hash as the cookie value will allow to set cookie parameters using the {Response#set_cookie} options. i.e.: `cookies['key']= {value: "lock", max_age: 20}`.

**Security**: set the HttpOnly flag for session id cookies.

***

Change log v.0.1.11

**Fix**: fixed the Rack server Handler, which was broken in version 0.1.10.

***

Change log v.0.1.10

**Fix**: make sure the WebsocketClient doesn't automatically renew the connection when the connection was manually closed by the client.

**Performance**: faster TimedEvent clearing when manually stopped. Minor improvements to direct big-file sending (recycle buffer to avoid malloc).

***

Change log v.0.1.9

**Fix**: WebsocketClient connection renewal will now keep the same WebsocketClient instance object.

**Update** Creating a TimedEvent before Iodine starts running will automatically 'nudge' Iodine into "Task polling" mode, cycling until the user signals a stop.

**Update**: repeatedly calling `Iodine.force_start!` will now be ignored, as might have been expected. Once Iodine had started, `force_start!` cannot be called until Iodine had finished (and even than, Iodine might never be as fresh nor as young as it was).

***

Change log v.0.1.8

**Fix**: Websocket broadcasts are now correctly executed within the IO's mutex locker. This maintains the idea that only one thread at a time should be executing code on behald of any given Protocol object ("yes" to concurrency between objects but "no" to concurrency within objects).

**Fix** fixed an issue where manually setting the number of threads for Rack applications (when using Iodine as a Rack server), the setting was mistakenly ignored.

**Fix** fixed an issue where sometimes extractin the Http response's body would fail (if body is `nil`).

**Feature**: session objects are now aware of the session id. The seesion id is available by calling `response.session.id`

**Fix** fixed an issue where Http streaming wasn't chunk encoding after connection error handling update.

**Fix** fixed an issue where Http streaming would disconnect while still processing. Streaming timeout now extended to 15 seconds between response writes.

***

Change log v.0.1.7

Removed a deprecation notice for blocking API. Client API will remain blocking due to use-case requirements.

***

Change log v.0.1.6

**Fix**: fixed an issue where a session key-value pair might not get deleted when using `session.delete key` and the `key` is not a String object. Also, now setting a key's value to `nil` should delete the key-value pair.

**Fix**: fixed an issue where WebsocketClient wouldn't mask outgoing data, causing some servers to respond badly.

**Performance**: minor performance improvements to the websocket parser, for unmasking messages.

**Deprecation notice**:

(removed after reviewing use-cases).

***

Change log v.0.1.5

**Feature**: The Response#body can now be set to a File object, allowing Iodine to preserve memory when serving large static files from disc. Limited Range requests are also supported - together, these changes allow Iodine to serve media files (such as movies) while suffering a smaller memory penalty and supporting a wider variaty of players (Safari requires Range request support for it's media player).

**Fix**: Fixed an issue where Iodine might take a long time to shut down after a Fatal Error during the server initialization.

***

Change log v.0.1.4

**Fix**: fixed an issue with where the WebsocketClient#on_close wouldn't be called for a renewable Websocket connection during shutdown.

**Fix**: fixed an issue where a protocol's #on_close callback wouldn't be called if the Iodine server receives a shutdown signal.

**Fix**: fixed an issue where Http2 header size limit condition was not recognized by the Ruby parser (a double space issue, might be an issue with the 2.2.3 Ruby parser).

***

Change log v.0.1.3

**Fix**: fixed an issue with the new form/multipart parser, where the '+' sign would be converted to spaces on form fields (not uploaded files), causing inadvert potential change to the original POSTed data.

***

Change log v.0.1.2

**Fix**: fixed an issue where the default implementation of `ping` didn not reset the timeout if the connection wasn't being closed (the default implementation checks if the Protocol is working on existing data and either resets the timer allowing the work to complete or closes the connection if no work is being done).

***

Change log v.0.1.1

**Fix**: Fixed an issue where slow processing of Http/1 requests could cause timeout disconnections to occur while the request is being processed.

**Change/Security**: Uploads now use temporary files. Aceessing the data for file uploads should be done throught the `:file` property of the params hash (i.e. `params[:upload_field_name][:file]`). Using the `:data` property (old API) would cause the whole file to be dumped to the memory and the file's content will be returned as a String.

**Change/Security**: Http upload limits are now enforced. The current default limit is about ~0.5GB.

**Feature**: WebsocketClient now supports both an auto-connection-renewal and a polling machanism built in to the `WebsocketClient.connect` API. The polling feature is mostly a handy helper for testing, as it is assumed that connection renewal and pub/sub offer a better design than polling.

**Logging**: Better Http error logging and recognition.

***

Change log v.0.1.0

**First actual release**:

We learn, we evolve, we change... but we remember our past and do our best to help with the transition and make it worth the toll it takes on our resources.

I took much of the code used for GRHttp and GReactor, changed it, morphed it and united it into the singular Iodine gem. This includes Major API changes, refactoring of code, bug fixes and changes to the core approach of how a task/io based application should behave or be constructed.

For example, Iodine kicks in automatically when the setup script is done, so that all code is run from within tasks and IO connections and no code is run in parallel to the Iodine engine.

Another example, Iodine now favors Object Oriented code, so that some actions - such as writing a network service - require classes of objects to be declared or inherited (i.e. the Protocol class).

This allows objects to manage their data as if they were in a single thread environment, unless the objects themselves are calling asynchronous code. For example, the Protocol class makes sure that the `on_open` and `on_message(data)` callbacks are excecuted within a Mutex (`on_close` is an exception to the rule since it is assumed that objects should be prepared to loose network connection at any moment).

Another example is that real-life deployemnt preferences were favored over adjustability or features. This means that some command-line arguments are automatically recognized (such as the `-p <port>` argument) and thet Iodine assumes a single web service per script/process (whereas GReactor and GRHttp allowed multiple listening sockets).

I tested this new gem during the 0.0.x version releases, and I feel that version 0.1.0 is stable enough to work with. For instance, I left the Iodine server running all night under stress (repeatedly benchmarking it)... millions of requests later, under heavey load, a restart wasn't required and memory consumption didn't show any increase after the warmup period.



## License

The gem is available as open source under the terms of the [MIT License](http://opensource.org/licenses/MIT).
