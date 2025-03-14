# Writeup for verbs-mrc

[X] Framework to load MRC symbols, verbs symbols and test them
	
- Use `dlvsym/dlsym` to load the symbols from the shared library into a structure of function pointers. The structure is obtained via `vmrc_symbols_get()` function. The function pointers have the suffix `_internal`.

[X] Framework to overwrite verbs calls and test the loaded symbols




