# ksuid-tcl

TCL module for K-Sortable Unique Identifiers (KSUIDs).
A TCL/C implementation of [Segment's KSUID library](https://github.com/segmentio/ksuid).

## What is a KSUID?

KSUID is for K-Sortable Unique IDentifier.
It is a kind of globally unique identifier similar to a 
[UUID](https://en.wikipedia.org/wiki/Universally_unique_identifier),
built from the ground-up to be "naturally" sorted by generation
timestamp without any special type-aware logic.
In short, running a set of KSUIDs through the UNIX `sort`
command will result in a list ordered by generation time.

## Build for TCL
    
```bash
wget https://github.com/jerily/ksuid-tcl/archive/refs/tags/v1.0.0.tar.gz
tar -xzf v1.0.0.tar.gz
cd ksuid-tcl-1.0.0
export KSUID_TCL_DIR=`pwd`
mkdir build
cd build
cmake ..
make
make test
make install
```

## Build for NaviServer

```bash
cd ${KSUID_TCL_DIR}
make
make install
```


## TCL Commands

* **::ksuid::generate_ksuid**
  - returns a ksuid
* **::ksuid::ksuid_to_parts** *ksuid*
  - returns a dict of the parts (timestamp and hex-encoded payload) of the ksuid
* **::ksuid::parts_to_ksuid** *parts_dict*
  - returns a ksuid from a dict of the parts (timestamp and hex-encoded payload) of the ksuid
* **::ksuid::next_ksuid** *ksuid*
  - returns the next ksuid
* **::ksuid::prev_ksuid** *ksuid*
  - returns the previous ksuid
* **::ksuid::hex_encode** *bytes*
  - returns a hex-encoded string
* **::ksuid::hex_decode** *hex_string*
  - returns a bytes object