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
### C-layer only domains
* Rings (now ring only)
* Buffers (now only implicit toy buffers)
* Registry
### Common domains
* Ops
  * Files
  * Sockets
* Macroses
* 