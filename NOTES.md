# Writeup for verbs-mrc

[x] Framework to load MRC symbols, verbs symbols and test them
	- Use `dlvsym/dlsym` to load the symbols from the shared library into a structure of function pointers. The structure is obtained via `vmrc_symbols_get()` function. The function pointers have the suffix `_internal`.\

[x] Framework to overwrite verbs calls and test the loaded symbols
	- In `vmrc_ibv_overwrites.c`, we have the overwrites for all the verbs calls. While compiling `libverbs_mrc.so`, we will not use `libibverbs` to link.
	- In `Makefile`, you will see that there are `tests` and `tests_internal`. 
	- Targets under `tests` are linked against `libibverbs.so` (for e.g., `tests/check_ibv_overwrites.c`) while targets under `tests_internal` are linked against `libverbs_mrc.so`.
	- To test the verb overwrites with targets under `tests`, you will have to `LD_PRELOAD` the verbs-mrc shared library. See `run-verbs-mrc.sh`.

- [x] Framework for the hashtable to pair up a `verbs_context` with a `mrc_context`.
	- Used Knuth's multiplicative hashing to map 64 bits to a number between 0 and 2^7-1.

- [x] `ibv_open_device`
	 - Create verbs context.
	 - Query MRC capabilities of the device.
	 - If insufficient capability, error out.
	 - If sufficient capability, create mrc context.
	 - Add key = verbs context, value = mrc context to the hashtable.
	 - Return the verbs context.





