## Principles:
* Strong layers between Python interface and C-functionality
* Trying to stick to DDD principles

## Domains could be present at:
* Only in `Python` layer
* Only in `C` layer
* In both layers

## Main Domains:
### Python-layer only domains
* Initialization
* Loop
* Signals
* Future
* Reader
* Macroses
  * GIL Releaser
  * Loop Thread Sentinel
### C-layer only domains
* Rings (now ring only)
* Buffers (now only implicit toy buffers)
* Registry
### Common domains
* Ops
  * Files
  * Sockets

## Fluidity design remark:
DDD means iterative architecture designing: while you are building system, you are learning more about it. So for this moment in code some domains are not as explicit as they could be.

#TODO: Describe Domains
