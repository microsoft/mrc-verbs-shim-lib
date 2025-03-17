# Writeup for verbs-mrc

Framework to load MRC symbols, verbs symbols and test them (Done)
- Use `dlvsym/dlsym` to load the symbols from the shared library into a structure of function pointers
- The structure is obtained via `vmrc_symbols_get()` function. The function pointers have the suffix `_internal`

Framework to overwrite verbs calls and test the loaded symbols (Done)
- In `vmrc_ibv_overwrites.c`, we have the overwrites for all the verbs calls
- In `Makefile`, you will see that there are `tests` and `tests_internal`
- Targets under `tests` are linked against `libibverbs.so` (for e.g., `tests/check_ibv_overwrites.c`) and are used to check the overwrites
- Targets under `tests_internal` are linked against `libverbs_mrc.so` and are used to check the functionality of components within verbs-mrc (hashtable, symbols)
- To test the verb overwrites with targets under `tests`, we `LD_PRELOAD` the verbs-mrc shared library. See `run-verbs-mrc.sh`

Framework for the hashtable to pair up a `verbs_context` with a `mrc_context` (Done).
- Used Knuth's multiplicative hashing to map 64 bits to a number between 0 and 2^7-1
- We have functions to insert and search entries in hashtable

`ibv_open_device` (Done)
 - Create verbs context.
 - Query MRC capabilities of the device.
 - If insufficient capability, error out.
 - If sufficient capability, create mrc context.
 - Add key = verbs context, value = mrc context to the hashtable.
 - Return the verbs context.

`ibv_close_device` (Done)
- Get mrc context for the corresponding verbs context
- Destroy the mrc context
- Then, close the device with the verbs context

`ibv_create_cq` (Done)
- We will create use the inputs to create MRC cq.
- Return the pointer to MRC cq as the verbs cq pointer.

`ibv_poll_cq`
- Simply map this to `mrc_poll_cq (cq, num_entries, wc)`.

`ibv_destroy_cq`
- Simply map this to `mrc_destroy_cq`.

`ibv_create_qp`
- NCCL uses `ibv_create_qp` to create the QPs.
- This takes pd as input. Get `verbs_context` from the `pd->context`.
- Get the corresponding `mrc_context` from the hashtable.
- Fill `mrc_qp_init_attr` using the input `ibv_qp_init_attr`.
- Create the MRC qp.
- Get the qp number of MRC qp.
- Calloc a `struct ibv_qp`. 
- Put the MRC qp number in `ibv_qp->qp_num` field. The application will exchange this `qp_num` field via OOB (sockets)
- Put the passed `qp_context` in `ibv_qp->qp_context` in case the application uses this.
- Put the pointer to MRC qp in `ibv_qp->send_cq`. I don't think applications will directly use `send_cq`.
- Return the `ibv_qp`.

`ibv_create_qp_ex`
- perftest uses `ibv_create_qp_ex`. So, this must be incorporated too.
- Since `ibv_create_qp_ex` takes `ibv_context` as input, we don't need to do anything special to get the verbs context.
- Fill `mrc_qp_init_attr` using the input `ibv_qp_init_attr`.
- Create the MRC qp. 	
- Get the qp number of MRC qp.
- Calloc a `struct ibv_qp`. 
- Put the MRC qp number in `ibv_qp->qp_num` field. The application will exchange this `qp_num` field via OOB (sockets)
- Put the passed `qp_context` in `ibv_qp->qp_context` in case the application uses this.
- Put the pointer to MRC qp in `ibv_qp->send_cq`.
- Return the `ibv_qp`.

`ibv_modify_qp`
- Here, get the MRC QP pointer from `ibv_qp->send_cq`.
- If the new state is INIT, 
    - then `memset(&mrc_qp_attr, 0, sizeof(mrc_qp_attr));` and `mrc_qp_attr_mask = 0;` 
    - call `mrc_modify_qp(mrc_qp, &ibv_qp_attr, ibv_qp_attr_mask, &mrc_qp_attr, mrc_qp_attr_mask)`
- If the new state is RTR, then create an `mrc_ev_array`. The list of EVs can be obtained from the `system.json` file. The 
source IP and destination IP is used to obtain the actual EV list. Put the EV array in the dummy ibv qp's recv_cq. 
    - Accordingly, create the `mrc_qp_attr` with `mrc_qp_attr.ev_array = mrc_ev_array` and other entries similar in the example ev_explicit.md
    ```
    memset(&mrc_qp_attr, 0, sizeof(mrc_qp_attr));
    mrc_qp_attr.num_ev               = num_ev;
    mrc_qp_attr.min_active_ev_per_qp = ev_min_active; /* no change */
    mrc_qp_attr.ev_array             = mrc_ev_array;
    /* set MRC QP attributes mask... */
    mrc_qp_attr_mask  = 0;
    mrc_qp_attr_mask |= MRC_QP_MAX_EV_COUNT; 
    mrc_qp_attr_mask |= MRC_QP_EV_MIN_ACTIVE;
    mrc_qp_attr_mask |= MRC_QP_EV_ARRAY;
    ```
    - call `mrc_modify_qp(mrc_qp, &ibv_qp_attr, ibv_qp_attr_mask, &mrc_qp_attr, mrc_qp_attr_mask)`
- If the new state is RTS,

`ibv_destroy_qp`
- Get the MRC QP pointer from `ibv_qp->send_cq` and destroy via `mrc_destroy_qp`.
- Get the MRC EV array from `ibv_qp->recv_cq` and destroy it.

`ibv_post_send`
- Map to `mrc_post_send` using `ibv_qp->send_cq` as the QP.

`ibv_post_recv`
- Map to `mrc_post_recv` using `ibv_qp->send_cq` as the QP.