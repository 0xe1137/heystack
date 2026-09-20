# heystack

filesystem index. low memory footprint, pretty fast.  capable of 
exact, substring, prefix match searching.

## some benchmarks

```bash
/Users/elliot/CLionProjects/heystack/cmake-build-release/heystack

No usable index cache. Scanning filesystem...
[TIMER] Filesystem indexing: 16288.8 ms
[TIMER] Index Serialization: 39.0958 ms
Indexed 678910 records.
FileRecord size: 24 bytes
> java
[TIMER] Search: 32.1823 ms
1000 results
...

> php
[TIMER] Search: 32.2118 ms
1000 results
...

> calculus 
[TIMER] Search: 42.5973 ms
4 results
/Users/elliot/School/Math/Calculus Early Transcendentals (James Stewart Daniel Clegg Saleem Watson) (z-library.sk, 1lib.sk, z-lib.sk).pdf
/Users/elliot/Papers/A Calculus of Communicating Systems[1980].pdf
/Users/elliot/Desktop/James Stewart - Calculus - Early transcendentals (8th Edition).pdf
/Users/elliot/Desktop/Precalculus _ mathematics for calculus, high school edition -- Stewart, James, Redlin, Lothar, Watson, Saleem -- Seventh edition, [Place of -- isbn13 9781305071759 -- a09709b630868a355979993b5340d205 -- Anna’s Archive.pdf

```

Roughly `25 MiB` resident memory footprint for `680,000` indexed files on my M4 Mac Mini.

Initial indexing takes about 16 seconds on my machine, subsequent loads from disk take about `108 ms` to 
re-read the entire index back into memory.
