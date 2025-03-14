# Writeup for verbs-mrc

- [x] Framework to load MRC symbols, verbs symbols and test them
	
	- Use `dlvsym/dlsym` to load the symbols from the shared library into a structure of function pointers. The structure is obtained via `vmrc_symbols_get()` function. The function pointers have the suffix `_internal`.

- [x] Framework to overwrite verbs calls and test the loaded symbols




