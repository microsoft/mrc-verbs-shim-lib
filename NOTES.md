# Writeup for verbs-mrc

- [x] Framework to load MRC symbols, verbs symbols and test them
	- Use `dlvsym/dlsym` to load the symbols from the shared library into a structure of function pointers
	- The structure is obtained via `vmrc_symbols_get()` function. The function pointers have the suffix `_internal`
<br/><br/>

- [x] Framework to overwrite verbs calls and test the loaded symbols
	- In `vmrc_ibv_overwrites.c`, we have the overwrites for all the verbs calls
	- In `Makefile`, you will see that there are `tests` and `tests_internal`
	- Targets under `tests` are linked against `libibverbs.so` (for e.g., `tests/check_ibv_overwrites.c`) and are used to check the overwrites
	- Targets under `tests_internal` are linked against `libverbs_mrc.so` and are used to check the functionality of components within verbs-mrc (hashtable, symbols)
	- To test the verb overwrites with targets under `tests`, we `LD_PRELOAD` the verbs-mrc shared library. See `run-verbs-mrc.sh`
<br/><br/>

- [x] Framework for the hashtable to pair up a `verbs_context` with a `mrc_context`.
	- Used Knuth's multiplicative hashing to map 64 bits to a number between 0 and 2^7-1
	- We have functions to insert and search entries in hashtable
<br/><br/>

- [x] `ibv_open_device`
	 - Create verbs context.
	 - Query MRC capabilities of the device.
	 - If insufficient capability, error out.
	 - If sufficient capability, create mrc context.
	 - Add key = verbs context, value = mrc context to the hashtable.
	 - Return the verbs context.
<br/><br/>

- [x] `ibv_close_device`
	- Get mrc context for the corresponding verbs context
	- Destroy the mrc context
	- Then, close the device with the verbs context
<br/><br/>

- [x] `ibv_create_cq`
	- We will create use the inputs to create MRC cq.
	- Return the pointer to MRC cq.
<br/><br/>

- [ ] `ibv_poll_cq`
<br/><br/>

- [ ] `ibv_destroy_cq`
<br/><br/>


- [ ] `ibv_create_qp`
	- Get verbs_context from the `pd->context`.
	- Get the corresponding mrc_context from the hashtable.
	- Fill mrc_qp_init_attr using the input ibv_qp_init_attr.
	- Create the MRC qp.
	- Get the qp number of MRC qp.
	- Calloc a `struct ibv_qp`. 
	- Put the MRC qp number in ibv_qp's qp_num. The application will exchange this qp_num field via OOB (sockets)
	- Put the pointer to MRC qp in ibv_qp's qp_context.
	- Return the ibv_qp.
<br/><br/>

- [ ] `ibv_create_qp_ex`
	- perftest uses ibv_create_qp_ex
<br/><br/>

- [ ] `ibv_modify_qp`
<br/><br/>

- [ ] `ibv_destroy_qp`
<br/><br/>

- [ ] `ibv_post_send`
<br/><br/>

- [ ] `ibv_post_recv`
<br/><br/>

