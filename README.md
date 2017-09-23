# skynetplus
```
skynetplus is a server frame. It's aim is follow.
(1) Synchronous programming. net or io operator Synchronous. Code as single thread, no lock, less malloc/new
(2) More efficient. no waste machine cpu. no wait.
(3) Interface. bussiness supply as service
(4) Distributed. register request and response serialize func, all interface can use in any other machine.
```
if you use the frame, you shoud know the diff between resource and code. every interface contain self resource, you want to oper the resource, you must be call the interface.
but now it is diffcult to code (like scripting language), compile check is no useful.

## compile
```
use cmake to compile
simple:
mkdir build && cd build
cmake ..
