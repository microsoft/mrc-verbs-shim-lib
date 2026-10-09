// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

/* Overwrite ibverbs calls. */

#define _GNU_SOURCE

#include <arpa/inet.h>
#include <dlfcn.h>
#include <errno.h>
#include <infiniband/verbs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "include/vmrc_ht.h"
#include "include/vmrc_log.h"
#include "include/vmrc_symbols.h"
#include "mrc.h"

#define VMRC_DEF_VIS __attribute__((visibility("default")))

/* Versioned symbol alias and defines a wrapper that calls <name>_internal. */
#define VMRC_WRAP_SYMVER(name, ver, rettype, params, args)      \
  __asm__(".symver ovwrt_" #name ", " #name "@@" ver);          \
  VMRC_DEF_VIS rettype ovwrt_##name params {                    \
    struct vmrc_symbols_t* symbols;                             \
    VMRC_DEBUG_PRINT("In " #name);                              \
    symbols = vmrc_symbols_get();                               \
    VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols"); \
    return symbols->name##_internal args;                       \
  }

/* Pass through IBverbs symbols. */
VMRC_WRAP_SYMVER(ibv_fork_init, "IBVERBS_1.1", int, (void), ())
VMRC_WRAP_SYMVER(ibv_get_device_list, "IBVERBS_1.1", struct ibv_device**, (int* num_devices), (num_devices))
VMRC_WRAP_SYMVER(ibv_free_device_list, "IBVERBS_1.1", void, (struct ibv_device * *list), (list))
VMRC_WRAP_SYMVER(ibv_get_device_name, "IBVERBS_1.1", const char*, (struct ibv_device * device), (device))
VMRC_WRAP_SYMVER(ibv_get_async_event, "IBVERBS_1.1", int, (struct ibv_context * context, struct ibv_async_event* event),
                 (context, event))
VMRC_WRAP_SYMVER(ibv_ack_async_event, "IBVERBS_1.1", void, (struct ibv_async_event * event), (event))
VMRC_WRAP_SYMVER(ibv_query_device, "IBVERBS_1.1", int,
                 (struct ibv_context * context, struct ibv_device_attr* device_attr), (context, device_attr))
#undef ibv_query_port  // Undefine the macro and redefine after instantiating pass through for ibv_query_port.
VMRC_WRAP_SYMVER(ibv_query_port, "IBVERBS_1.1", int,
                 (struct ibv_context * context, uint8_t port_num, struct ibv_port_attr* port_attr),
                 (context, port_num, port_attr))
#define ibv_query_port(context, port_num, port_attr) ___ibv_query_port(context, port_num, port_attr)
VMRC_WRAP_SYMVER(ibv_query_gid, "IBVERBS_1.1", int,
                 (struct ibv_context * context, uint8_t port_num, int index, union ibv_gid* gid),
                 (context, port_num, index, gid))
VMRC_WRAP_SYMVER(ibv_alloc_pd, "IBVERBS_1.1", struct ibv_pd*, (struct ibv_context * context), (context))
VMRC_WRAP_SYMVER(ibv_dealloc_pd, "IBVERBS_1.1", int, (struct ibv_pd * pd), (pd))
#undef ibv_reg_mr  // Undefine the macro and redefine after instantiating pass through for ibv_reg_mr.
VMRC_WRAP_SYMVER(ibv_reg_mr, "IBVERBS_1.1", struct ibv_mr*, (struct ibv_pd * pd, void* addr, size_t length, int access),
                 (pd, addr, length, access))
#define ibv_reg_mr(pd, addr, length, access) \
  __ibv_reg_mr(pd, addr, length, access, __builtin_constant_p(((int)(access) & IBV_ACCESS_OPTIONAL_RANGE) == 0))
VMRC_WRAP_SYMVER(ibv_reg_mr_iova2, "IBVERBS_1.8", struct ibv_mr*,
                 (struct ibv_pd * pd, void* addr, size_t length, uint64_t iova, unsigned int access),
                 (pd, addr, length, iova, access))
VMRC_WRAP_SYMVER(ibv_reg_dmabuf_mr, "IBVERBS_1.12", struct ibv_mr*,
                 (struct ibv_pd * pd, uint64_t offset, size_t length, uint64_t iova, int fd, int access),
                 (pd, offset, length, iova, fd, access))
VMRC_WRAP_SYMVER(ibv_dereg_mr, "IBVERBS_1.1", int, (struct ibv_mr * mr), (mr))
VMRC_WRAP_SYMVER(ibv_event_type_str, "IBVERBS_1.1", const char*, (enum ibv_event_type event), (event))

/* Print info to not use the verb and exit. */
#define VMRC_WRAP_SYMVER_ERR(name, ver, rettype, params, args)                                           \
  __asm__(".symver ovwrt_" #name ", " #name "@@" ver);                                                   \
  VMRC_DEF_VIS rettype ovwrt_##name params {                                                             \
    VMRC_CHECK_PRINT_EXIT(NULL, 1, #name " cannot be used with verbs-mrc");                              \
    return 1; /* To satisfy the compiler. This line will never be reached since the above line exits. */ \
  }

VMRC_WRAP_SYMVER_ERR(ibv_query_ece, "IBVERBS_1.10", int, (struct ibv_qp * qp, struct ibv_ece* ece), (qp, ece))
VMRC_WRAP_SYMVER_ERR(ibv_set_ece, "IBVERBS_1.10", int, (struct ibv_qp * qp, struct ibv_ece* ece), (qp, ece))

/* Forward declarations for post_send/post_recv used in create_qp_ex. */
int vmrc_ibv_overwrite_post_send(struct ibv_qp* qp, struct ibv_send_wr* wr, struct ibv_send_wr** bad_wr);
int vmrc_ibv_overwrite_post_recv(struct ibv_qp* qp, struct ibv_recv_wr* wr, struct ibv_recv_wr** bad_wr);

/*
 * Overwrite of the create_qp_ex op (reached via ibv_create_qp_ex()).
 *
 * Builds an MRC-backed QP and wraps it in a dummy ibv_qp we fully control. This op
 * is reached from applications calling ibv_create_qp_ex, and would also be reached
 * from libmrc if it called ibv_create_qp_ex internally. However, libmrc's internal
 * provider-QP creation uses the classic ibv_create_qp() path (verified in mrc_ibv.c:76-77,
 * which dlsyms "ibv_create_qp", not "ibv_create_qp_ex"), so there is no recursion
 * and no original op needs to be saved.
 *
 * This matches the create_cq_ex pattern: directly call MRC, build a wrapper, return.
 */
struct ibv_qp* vmrc_ibv_overwrite_create_qp_ex(struct ibv_context* context,
                                               struct ibv_qp_init_attr_ex* qp_init_attr_ex) {
  struct vmrc_symbols_t* symbols;
  struct ibv_context* dummy_verbs_context;
  struct vmrc_ht* hashtable;
  struct mrc_context* vmrc_context;
  struct mrc_qp* vmrc_qp;
  struct ibv_qp* verbs_qp;
  struct mrc_qp_init_attr mrc_qp_attr;
  struct ibv_pd* pd;
  void* ptr;
  int mrc_errno;

  VMRC_DEBUG_PRINT("In ibv_create_qp_ex");

  /* Validate QP type. */
  VMRC_CHECK_PRINT_EXIT(qp_init_attr_ex->qp_type == IBV_QPT_RC, 1, "create_qp_ex: Only RC QP types are supported");

  /* ibv_create_qp_ex() routes a PD-only comp_mask to ibv_create_qp(), so this op is
   * normally reached only when the app asked for more than a PD. MRC supports none of
   * the extensions (xrcd, create_flags, TSO, RSS, send_ops_flags). send_ops_flags in
   * particular requests the ibv_wr_* API: ibv_qp_to_qp_ex() then reads the QP as a
   * struct verbs_qp, which the dummy ibv_qp built below is not. Reject rather than
   * silently drop what the app asked for. */
  VMRC_CHECK_PRINT_EXIT(qp_init_attr_ex->comp_mask == IBV_QP_INIT_ATTR_PD, 1,
                        "create_qp_ex: only IBV_QP_INIT_ATTR_PD is supported in comp_mask "
                        "(no xrcd/create_flags/max_tso_header/rwq_ind_tbl/rx_hash/send_ops_flags; "
                        "perftest: use --use_old_post_send)");
  pd = qp_init_attr_ex->pd;
  VMRC_CHECK_PRINT_EXIT(pd, 1, "create_qp_ex: NULL pd in qp_init_attr_ex");

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "create_qp_ex: Could not get symbols");

  /* Get MRC context from hashtable. */
  hashtable = vmrc_ht_get();
  VMRC_CHECK_PRINT_EXIT(hashtable, 1, "create_qp_ex: Could not get hashtable");
  vmrc_context = vmrc_ht_search(hashtable, context);
  VMRC_CHECK_PRINT_EXIT_VA_ARGS(vmrc_context, 1,
                                "create_qp_ex: Could not find the matching MRC context for verbs context %p", context);

  /* Fill MRC QP attributes from extended attributes. */
  memset(&mrc_qp_attr, 0, sizeof(struct mrc_qp_init_attr));
  mrc_qp_attr.qp_context = qp_init_attr_ex->qp_context;
  mrc_qp_attr.send_cq = (void*)qp_init_attr_ex->send_cq->channel;
  mrc_qp_attr.recv_cq = (void*)qp_init_attr_ex->recv_cq->channel;
  mrc_qp_attr.pd = pd;
  mrc_qp_attr.cap = qp_init_attr_ex->cap;
  mrc_qp_attr.sq_sig_all = qp_init_attr_ex->sq_sig_all;

  /* Create MRC QP. */
  vmrc_qp = symbols->mrc_create_qp_internal(vmrc_context, &mrc_qp_attr);
  VMRC_CHECK_PRINT_EXIT(vmrc_qp, 1, "create_qp_ex: Error while calling mrc_create_qp");

  /* Allocate a dummy ibv_qp. */
  verbs_qp = calloc(1, sizeof(struct ibv_qp));
  VMRC_CHECK_PRINT_EXIT(verbs_qp, 1, "create_qp_ex: Unable to allocate the dummy verbs QP");

  /* Put MRC qp_num in verbs_qp->qp_num. */
  mrc_errno = symbols->mrc_get_qpn_internal(vmrc_qp, &verbs_qp->qp_num);
  VMRC_CHECK_PRINT_EXIT(mrc_errno == 0, 1, "create_qp_ex: Unable to call mrc_get_qpn");

  /* Put qp_context. */
  verbs_qp->qp_context = qp_init_attr_ex->qp_context;

  /* Put MRC QP in send_cq. */
  verbs_qp->send_cq = (void*)vmrc_qp;

  /* Allocate dummy verbs context. */
  dummy_verbs_context = calloc(1, sizeof(struct ibv_context));
  VMRC_CHECK_PRINT_EXIT(dummy_verbs_context, 1, "create_qp_ex: Unable to allocate the dummy verbs context");

  /* Put overwrites of post_send, post_recv. */
  dummy_verbs_context->ops.post_send = &vmrc_ibv_overwrite_post_send;
  dummy_verbs_context->ops.post_recv = &vmrc_ibv_overwrite_post_recv;

  /* Put the dummy verbs context in the returned QP's qp->context. */
  verbs_qp->context = dummy_verbs_context;

  /* Put the actual verbs context in verbs_qp->pd and verbs_qp->recv_cq. */
  verbs_qp->pd = pd;
  verbs_qp->recv_cq = (void*)context;

  /* Allocate storage for gid raw and store the input send_cq and recv_cq. */
  verbs_qp->srq = (void*)calloc(16 + 2 * (sizeof(void*) / sizeof(uint8_t)), sizeof(uint8_t));
  ptr = (void*)verbs_qp->srq;
  ptr = ptr + (16 / sizeof(void*));
  ptr = (void*)qp_init_attr_ex->send_cq;
  ptr = ptr + 1;
  ptr = (void*)qp_init_attr_ex->recv_cq;

  return verbs_qp;
}

/* Forward declaration: the create_cq_ex op is wired in ovwrt_ibv_open_device
 * (below), but the implementation lives after the classic CQ handlers. */
struct ibv_cq_ex* vmrc_ibv_overwrite_create_cq_ex(struct ibv_context* context, struct ibv_cq_init_attr_ex* cq_attr);

#ifdef HAVE_IONIC_DV
/*
 * Resolve a genuine provider ionic_dv_* symbol.
 *
 * We first try RTLD_NEXT (works when the real ionic provider is already in the
 * global scope, e.g. when the app/ANP links libionic directly). When ANP is
 * built WITHOUT -lionic, the real symbols live only in the rdma-core provider
 * plugin (libionic-rdmav34.so), which libibverbs dlopen's RTLD_LOCAL and is thus
 * invisible to RTLD_NEXT. In that case we fall back to an explicit dlopen of
 * libionic.so.1 (overridable via VMRC_LIBIONIC_SO), which pulls in the versioned
 * ionic_dv_* symbols. The handle is cached process-wide.
 *
 * The fallback loads RTLD_LOCAL on purpose: we only ever look the symbols up on
 * our own handle, and RTLD_GLOBAL would publish libionic's ionic_dv_* into the
 * global scope, where objects dlopen'd later could bind to the genuine provider
 * helpers instead of the interceptors below -- silently defeating the shim.
 */
static void* vmrc_resolve_ionic_sym(const char* name, const char* ver) {
  void* sym = dlvsym(RTLD_NEXT, name, ver);
  if (sym == NULL) sym = dlsym(RTLD_NEXT, name);
  if (sym != NULL) return sym;

  static void* ionic_handle = NULL;
  if (ionic_handle == NULL) {
    const char* path = getenv("VMRC_LIBIONIC_SO");
    if (path == NULL) path = "libionic.so.1";
    ionic_handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
  }
  if (ionic_handle == NULL) return NULL;

  sym = dlvsym(ionic_handle, name, ver);
  if (sym == NULL) sym = dlsym(ionic_handle, name);
  return sym;
}

/*
 * Recover the real provider QP hiding behind the dummy ibv_qp the shim hands to
 * applications.
 *
 * CRITICAL ABI ASSUMPTION: struct mrc_qp first member layout.
 *
 * The ionic_dv_* interceptors below rely on this libmrc layout contract:
 *
 *   struct mrc_qp {
 *       struct ibv_qp *ibv_qp;    // <-- MUST be the first member
 *       ...                        // other members
 *   };
 *
 * WHY THIS MATTERS:
 * - The shim returns a dummy ibv_qp to applications, whose send_cq field stores
 *   the mrc_qp* (not a real CQ -- it is used as storage, see ovwrt_ibv_create_qp)
 * - The provider-specific helpers need the REAL ibv_qp libmrc created internally
 * - struct mrc_qp is opaque in the public mrc.h, so we copy out its leading
 *   pointer-sized field rather than declaring the layout or type-punning through
 *   a cast; memcpy of a pointer is well defined and compiles to a single load
 *
 * This works ONLY because ibv_qp* is the first member. If libmrc changes this
 * layout, the shim will break. This is a documented ABI contract.
 *
 * VERIFICATION: this matches libmrc's internal mrc_defines.h as of the current
 * version. Do NOT include mrc_defines.h (internal header).
 *
 * Returns NULL if either indirection is missing; callers report and exit.
 */
static struct ibv_qp* vmrc_real_qp_of(struct ibv_qp* verbs_qp) {
  struct mrc_qp* vmrc_qp = (struct mrc_qp*)verbs_qp->send_cq;
  struct ibv_qp* real_qp;

  if (vmrc_qp == NULL) return NULL;

  memcpy(&real_qp, vmrc_qp, sizeof real_qp);
  return real_qp;
}

/*
 * Define an interceptor for an ionic_dv_qp_* helper.
 *
 * perftest and ANP (the RCCL net plugin) call these provider-specific helpers
 * directly on the QP returned by ibv_create_qp(). Under the shim that QP is a
 * dummy, so the real libionic would reject it (is_ionic_qp() -> EPERM). Each
 * interceptor recovers the real ionic QP and forwards to the genuine provider
 * symbol, which is resolved once and cached. The provider stores the settings and
 * carries them to HW on its own modify_qp(RTR) path -- we issue no NIC command
 * here. Trailing struct arguments are passed straight through as opaque pointers
 * so the shim does not have to mirror libionic's struct definitions.
 *
 * name   - provider symbol, also the versioned symbol we export
 * ver     - symbol version to export at and to resolve against
 * params - full parameter list; the leading ibv_qp* MUST be named verbs_qp
 * args   - forwarding argument list, passing real_qp in place of verbs_qp
 */
#define VMRC_WRAP_IONIC_QP(name, ver, params, args)                                    \
  static int(*g_real_##name) params = NULL;                                            \
                                                                                       \
  __asm__(".symver ovwrt_" #name ", " #name "@@" ver);                                 \
  VMRC_DEF_VIS int ovwrt_##name params {                                               \
    struct ibv_qp* real_qp;                                                            \
    VMRC_DEBUG_PRINT("In " #name);                                                     \
    VMRC_CHECK_PRINT_EXIT(verbs_qp, 1, #name ": NULL qp");                             \
    real_qp = vmrc_real_qp_of(verbs_qp);                                               \
    VMRC_CHECK_PRINT_EXIT(real_qp, 1, #name ": no real provider QP behind the dummy"); \
    /* Resolve the genuine provider symbol (skips our own export). Copy rather         \
     * than punning through *(void**)& -- dlsym returns void*, and assigning it        \
     * to a function pointer is only well defined via an object-representation         \
     * copy (POSIX's documented workaround). */                                        \
    if (g_real_##name == NULL) {                                                       \
      void* sym = vmrc_resolve_ionic_sym(#name, ver);                                  \
      VMRC_CHECK_PRINT_EXIT(sym, 1, #name ": cannot resolve real provider symbol");    \
      memcpy(&g_real_##name, &sym, sizeof sym);                                        \
    }                                                                                  \
    return g_real_##name args;                                                         \
  }

VMRC_WRAP_IONIC_QP(ionic_dv_qp_set_multiplane_conn_info, "IONIC_1.2", (struct ibv_qp * verbs_qp, const void* conn_info),
                   (real_qp, conn_info))
VMRC_WRAP_IONIC_QP(ionic_dv_qp_set_gda, "IONIC_1.0", (struct ibv_qp * verbs_qp, _Bool enable_send, _Bool enable_recv),
                   (real_qp, enable_send, enable_recv))
VMRC_WRAP_IONIC_QP(ionic_dv_qp_set_puec_plane_route, "IONIC_1.0",
                   (struct ibv_qp * verbs_qp, uint8_t plane_idx, void* route), (real_qp, plane_idx, route))

/*
 * Intercept ionic_dv_pd_set_udma_mask.
 *
 * Unlike the QP helpers above, ibv_alloc_pd() is a straight pass-through in the
 * shim, so the PD the app holds is already a genuine ionic PD (not a dummy).
 * We therefore forward directly to the real provider symbol (resolved via
 * RTLD_NEXT) with the app's PD unchanged -- no dummy recovery needed. This
 * export exists so ANP (which hard-links libionic) resolves the symbol from the
 * shim instead of requiring libionic to load, which would drag in the native
 * verbs provider ABI (IBVERBS_PRIVATE_34) the shim does not provide.
 */
static int (*g_real_ionic_pd_set_udma_mask)(struct ibv_pd*, uint8_t) = NULL;

__asm__(
    ".symver ovwrt_ionic_dv_pd_set_udma_mask, "
    "ionic_dv_pd_set_udma_mask@@IONIC_1.0");
VMRC_DEF_VIS int ovwrt_ionic_dv_pd_set_udma_mask(struct ibv_pd* pd, uint8_t udma_mask) {
  VMRC_DEBUG_PRINT("In ionic_dv_pd_set_udma_mask");

  VMRC_CHECK_PRINT_EXIT(pd, 1, "set_udma_mask: NULL pd");

  if (g_real_ionic_pd_set_udma_mask == NULL) {
    void* sym = vmrc_resolve_ionic_sym("ionic_dv_pd_set_udma_mask", "IONIC_1.0");
    VMRC_CHECK_PRINT_EXIT(sym, 1, "set_udma_mask: cannot resolve real provider symbol");
    memcpy(&g_real_ionic_pd_set_udma_mask, &sym, sizeof sym);
  }

  return g_real_ionic_pd_set_udma_mask(pd, udma_mask);
}
#endif /* HAVE_IONIC_DV */

/*
 * Overwrites ibv_open_device. Queries if the device supports MRC. Errors out if the required capability is not present.
 * Creates an ibv_context. Also, creates an mrc_context. Keeps the (ibv_context, mrc_context) key-value pair in the hash
 * table. Returns the created ibv_context. The returned verbs context can be used to alloc pd and register memory.
 */
__asm__(".symver ovwrt_ibv_open_device, ibv_open_device@@IBVERBS_1.1");
VMRC_DEF_VIS struct ibv_context* ovwrt_ibv_open_device(struct ibv_device* device) {
  struct mrc_device_attr attr;
  struct vmrc_symbols_t* symbols;
  struct ibv_context* verbs_context;
  struct mrc_context* vmrc_context;
  struct mrc_context_attr vmrc_context_attr;
  struct vmrc_ht* hashtable;
  int mrc_errno;
  struct verbs_context* vctx;
  int mrc_supported;
  void* addr_of_value;

  VMRC_DEBUG_PRINT("In ibv_open_device");

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  verbs_context = symbols->ibv_open_device_internal(device);
  VMRC_CHECK_PRINT_EXIT(verbs_context, 1, "ibv_open_device failed");

  /* Query the device if it has sufficient MRC capability. */
  mrc_errno = symbols->mrc_query_device_internal(verbs_context, &attr, &mrc_supported);
  if (mrc_errno != 0) {
    VMRC_DEBUG_PRINT_VA_ARGS(
        "Dev %s does not appear to support MRC. mrc_errno = %d. Simply returning the verbs context",
        symbols->ibv_get_device_name_internal(device), mrc_errno);
    return verbs_context;
  }
  if (!mrc_supported) {
    VMRC_DEBUG_PRINT_VA_ARGS("MRC not supported for dev = %s. Returning verbs context.",
                             symbols->ibv_get_device_name_internal(device));
    return verbs_context;
  }

  /* Create the MRC context. */
  memset(&vmrc_context_attr, 0, sizeof(vmrc_context_attr));
  vmrc_context = symbols->mrc_create_context_internal(verbs_context, &vmrc_context_attr);
  VMRC_CHECK_PRINT_EXIT(vmrc_context, 1, "Could not create MRC context");

  /* Get the hashtable. */
  hashtable = vmrc_ht_get();
  VMRC_CHECK_PRINT_EXIT(hashtable, 1, "Could not get hashtable");

  /* Insert key, value. */
  vmrc_ht_insert(hashtable, verbs_context, vmrc_context);

  /* Overwrite create_qp_ex so apps that call ibv_create_qp_ex() go through MRC.
   * Unlike create_cq_ex (where recursion was possible because some providers'
   * internal ibv_create_cq might dispatch through create_cq_ex), libmrc's
   * internal provider-QP creation uses ibv_create_qp (not _ex), verified at
   * mrc_ibv.c:76-77, so no recursion occurs and no original pointer needs saving. */
  vctx = verbs_get_ctx_op(verbs_context, create_qp_ex);
  if (!vctx) {
    vmrc_ht_delete(hashtable, verbs_context);
    mrc_errno = symbols->mrc_destroy_context_internal(vmrc_context);
    VMRC_CHECK_PRINT_EXIT(mrc_errno == 0, 1, "Error in mrc_destroy_context");
    errno = EOPNOTSUPP;
    return NULL;
  }

  vctx->create_qp_ex = &vmrc_ibv_overwrite_create_qp_ex;

  /* Overwrite create_cq_ex so apps that call ibv_create_cq_ex() (e.g. perftest
   * built with cq_ex support) get an MRC-backed CQ instead of a genuine provider
   * CQ that would bypass MRC. Unlike create_qp_ex, this op is only ever reached
   * from the application: libmrc's own internal provider-CQ creation uses the
   * classic ibv_create_cq() path, so there is no recursion and no original op to
   * save. vctx is the same verbs_context resolved above. */
  vctx->create_cq_ex = &vmrc_ibv_overwrite_create_cq_ex;

  return verbs_context;
}

/* Close the device. Here, before calling close with the verbs context, destroy the MRC context. */
__asm__(".symver ovwrt_ibv_close_device, ibv_close_device@@IBVERBS_1.1");
VMRC_DEF_VIS int ovwrt_ibv_close_device(struct ibv_context* verbs_context) {
  struct vmrc_symbols_t* symbols;
  struct vmrc_ht* hashtable;
  struct mrc_context* vmrc_context;
  int mrc_errno;
  void* addr_of_value;

  VMRC_DEBUG_PRINT("In ibv_close_device");

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  hashtable = vmrc_ht_get();
  VMRC_CHECK_PRINT_EXIT(hashtable, 1, "Could not get context hashtable");

  /* Retrieving MRC context and destroying it. */
  vmrc_context = (struct mrc_context*)vmrc_ht_search_plus_addr(hashtable, verbs_context, &addr_of_value);
  if (vmrc_context == NULL) {
    VMRC_DEBUG_PRINT("No matching MRC context found. Simply closing the device");
    return symbols->ibv_close_device_internal(verbs_context);
  }

  VMRC_CHECK_PRINT_EXIT_VA_ARGS(vmrc_context, 1, "Could not find the matching MRC context for verbs context %p",
                                verbs_context);

  /* Destroy all the QP hints. */
  struct vmrc_ht_linked_list* attr = vmrc_ht_attr_get(addr_of_value, VMRC_HT_ATTR_QP_HINT_IDX);
  while (attr != NULL) {
    struct mrc_qp_hint* qp_hint = (struct mrc_qp_hint*)attr->ptr_and_next[VMRC_HT_LL_PTR];
    mrc_errno = symbols->mrc_destroy_qp_hint_internal(qp_hint);
    VMRC_CHECK_PRINT_EXIT(mrc_errno == 0, 1, "Error in mrc_destroy_qp_hint");
    attr = (struct vmrc_ht_linked_list*)attr->ptr_and_next[VMRC_HT_LL_NEXT];
  }

  /* The saved original create_qp_ex attribute node is freed by vmrc_ht_delete
   * below, along with all other attr nodes for this entry. */

  mrc_errno = symbols->mrc_destroy_context_internal(vmrc_context);
  VMRC_CHECK_PRINT_EXIT(mrc_errno == 0, 1, "Error in mrc_destroy_context");

  /* Delete the entry from the hashtable. */
  vmrc_ht_delete(hashtable, verbs_context);

  /* Destroy the verbs context. */
  return symbols->ibv_close_device_internal(verbs_context);
}

/*
 * Extended-CQ (ibv_cq_ex) wrapper.
 *
 * ibv_cq_ex has no ops dispatch table: ibv_start_poll/ibv_next_poll/ibv_end_poll
 * and the ibv_wc_read_* accessors are function pointers stored inside the struct
 * itself, invoked directly by the app-side inlines in <infiniband/verbs.h>. We
 * therefore embed an ibv_cq_ex we fully own, plus a cached ibv_wc that the
 * accessors read from between start_poll/next_poll and end_poll.
 *
 * cq_ex MUST be the first member: the value returned to the app is a struct
 * ibv_cq_ex*, and ibv_cq_ex_to_cq() casts it straight to struct ibv_cq*. Because
 * ibv_cq and ibv_cq_ex share their leading layout, cq_ex.channel doubles as the
 * mrc_cq* slot (same convention as the classic CQ path) and cq_ex.context holds
 * our dummy verbs context carrying ops.poll_cq (so classic ibv_poll_cq on the
 * down-cast CQ still routes through MRC -- which is exactly how perftest polls).
 * This shared layout also lets the classic ovwrt_ibv_destroy_cq free an ex CQ
 * unchanged: free(context), mrc_destroy_cq(channel), free(base pointer) -- and
 * free(&wrap->cq_ex) frees the whole wrapper since cq_ex is first.
 */
struct vmrc_cq_ex {
  struct ibv_cq_ex cq_ex; /* MUST be first: returned pointer aliases ibv_cq*. */
  struct ibv_wc cur_wc;   /* Completion cached for the read_* accessors. */
};

/* Defined just below; the ex CQ wires it into its dummy context's ops.poll_cq. */
int vmrc_ibv_overwrite_poll_cq(struct ibv_cq* cq, int num_entries, struct ibv_wc* wc);

static inline struct vmrc_cq_ex* vmrc_cq_ex_of(struct ibv_cq_ex* cq) { return (struct vmrc_cq_ex*)cq; }

/* start_poll/next_poll pull exactly one completion from MRC into cur_wc and
 * surface status/wr_id on the cq_ex, matching rdma-core semantics: return 0 when
 * a completion is available, ENOENT when the CQ is empty, and the errno on error. */
static int vmrc_cq_ex_poll_one(struct ibv_cq_ex* cq) {
  struct vmrc_symbols_t* symbols;
  struct vmrc_cq_ex* wrap = vmrc_cq_ex_of(cq);
  int ne;

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "cq_ex poll: could not get symbols");

  ne = symbols->mrc_poll_cq_internal((struct mrc_cq*)cq->channel, 1, &wrap->cur_wc);
  if (ne < 0) return errno ? errno : EIO;
  if (ne == 0) return ENOENT;

  cq->status = wrap->cur_wc.status;
  cq->wr_id = wrap->cur_wc.wr_id;
  return 0;
}

static int vmrc_cq_ex_start_poll(struct ibv_cq_ex* cq, struct ibv_poll_cq_attr* attr) {
  (void)attr;
  return vmrc_cq_ex_poll_one(cq);
}

static int vmrc_cq_ex_next_poll(struct ibv_cq_ex* cq) { return vmrc_cq_ex_poll_one(cq); }

static void vmrc_cq_ex_end_poll(struct ibv_cq_ex* cq) { (void)cq; }

/* read_* accessors: every field MRC populates in the standard ibv_wc. Timestamp,
 * SLID/SL/path-bits, cvlan, flow_tag and tm_info are intentionally absent -- MRC's
 * ibv_wc does not carry them, and create rejects any wc_flags requesting them. */
static enum ibv_wc_opcode vmrc_cq_ex_read_opcode(struct ibv_cq_ex* cq) { return vmrc_cq_ex_of(cq)->cur_wc.opcode; }
static uint32_t vmrc_cq_ex_read_vendor_err(struct ibv_cq_ex* cq) { return vmrc_cq_ex_of(cq)->cur_wc.vendor_err; }
static uint32_t vmrc_cq_ex_read_byte_len(struct ibv_cq_ex* cq) { return vmrc_cq_ex_of(cq)->cur_wc.byte_len; }
static __be32 vmrc_cq_ex_read_imm_data(struct ibv_cq_ex* cq) { return vmrc_cq_ex_of(cq)->cur_wc.imm_data; }
static uint32_t vmrc_cq_ex_read_qp_num(struct ibv_cq_ex* cq) { return vmrc_cq_ex_of(cq)->cur_wc.qp_num; }
static uint32_t vmrc_cq_ex_read_src_qp(struct ibv_cq_ex* cq) { return vmrc_cq_ex_of(cq)->cur_wc.src_qp; }
static unsigned int vmrc_cq_ex_read_wc_flags(struct ibv_cq_ex* cq) { return vmrc_cq_ex_of(cq)->cur_wc.wc_flags; }

/*
 * Overwrite of the create_cq_ex op (reached via ibv_create_cq_ex()).
 *
 * Builds an MRC-backed CQ and wraps it in a struct vmrc_cq_ex we fully control.
 * The wrapper is returned as an ibv_cq_ex*; because its first member aliases
 * ibv_cq*, apps that down-cast with ibv_cq_ex_to_cq() and poll classically go
 * through ops.poll_cq -> MRC, while apps using the extended accessors go through
 * the function pointers wired below.
 */
struct ibv_cq_ex* vmrc_ibv_overwrite_create_cq_ex(struct ibv_context* verbs_context,
                                                  struct ibv_cq_init_attr_ex* cq_attr) {
  struct vmrc_ht* hashtable;
  struct mrc_context* vmrc_context;
  struct vmrc_symbols_t* symbols;
  struct vmrc_cq_ex* wrap;
  struct mrc_cq* vmrc_cq;
  struct ibv_context* dummy_verbs_context;

  VMRC_DEBUG_PRINT("In ibv_create_cq_ex");

  VMRC_CHECK_PRINT_EXIT(cq_attr, 1, "create_cq_ex: NULL cq_attr");
  VMRC_CHECK_PRINT_EXIT(cq_attr->channel == NULL, 1, "create_cq_ex: non-NULL completion channel not yet supported");

  /* MRC's ibv_wc does not populate: pkey_index, slid, sl, dlid_path_bits (the
   * standard ibv_wc has them but MRC's internal wc leaves them uninitialized),
   * nor the extended fields (timestamp, cvlan, flow_tag, tm_info) since MRC has
   * no hardware completion timestamp and no encap metadata. Reject rather than
   * return silently-wrong values. The bits MRC does support (byte_len, imm_data,
   * qp_num, src_qp, wc_flags themselves) are in IBV_WC_STANDARD_FLAGS, but that
   * constant is only in recent rdma-core (>=v28). So we invert the check: reject
   * anything NOT in the standard set. */
#define MRC_UNSUPPORTED_WC_FLAGS                                                                               \
  (IBV_WC_EX_WITH_COMPLETION_TIMESTAMP | IBV_WC_EX_WITH_COMPLETION_TIMESTAMP_WALLCLOCK | IBV_WC_EX_WITH_SLID | \
   IBV_WC_EX_WITH_SL | IBV_WC_EX_WITH_DLID_PATH_BITS | IBV_WC_EX_WITH_CVLAN | IBV_WC_EX_WITH_FLOW_TAG |        \
   IBV_WC_EX_WITH_TM_INFO)
  VMRC_CHECK_PRINT_EXIT((cq_attr->wc_flags & MRC_UNSUPPORTED_WC_FLAGS) == 0, 1,
                        "create_cq_ex: unsupported wc_flags requested (MRC does not populate "
                        "timestamp/slid/sl/dlid_path_bits/cvlan/flow_tag/tm_info)");

  /* SINGLE_THREADED only promises the app serializes CQ access, so ignoring it is
   * safe. IGNORE_OVERRUN and a parent domain (thread domain, custom allocators)
   * change provider CQ behavior that MRC cannot honor. */
  VMRC_CHECK_PRINT_EXIT((cq_attr->comp_mask & ~IBV_CQ_INIT_ATTR_MASK_FLAGS) == 0, 1,
                        "create_cq_ex: only IBV_CQ_INIT_ATTR_MASK_FLAGS is supported in comp_mask "
                        "(no parent_domain; perftest: do not use --no_lock)");
  VMRC_CHECK_PRINT_EXIT(!(cq_attr->comp_mask & IBV_CQ_INIT_ATTR_MASK_FLAGS) ||
                            (cq_attr->flags & ~IBV_CREATE_CQ_ATTR_SINGLE_THREADED) == 0,
                        1, "create_cq_ex: only IBV_CREATE_CQ_ATTR_SINGLE_THREADED is supported in flags");

  hashtable = vmrc_ht_get();
  VMRC_CHECK_PRINT_EXIT(hashtable, 1, "create_cq_ex: could not get context hashtable");

  vmrc_context = (struct mrc_context*)vmrc_ht_search(hashtable, verbs_context);
  VMRC_CHECK_PRINT_EXIT_VA_ARGS(vmrc_context, 1, "create_cq_ex: no matching MRC context for verbs context %p",
                                verbs_context);

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "create_cq_ex: could not get symbols");

  vmrc_cq =
      symbols->mrc_create_cq_internal(vmrc_context, cq_attr->cqe, cq_attr->cq_context, NULL, cq_attr->comp_vector);
  VMRC_CHECK_PRINT_EXIT(vmrc_cq, 1, "create_cq_ex: error in mrc_create_cq");

  wrap = calloc(1, sizeof(struct vmrc_cq_ex));
  VMRC_CHECK_PRINT_EXIT(wrap, 1, "create_cq_ex: unable to allocate the dummy verbs CQ");

  /* mrc_cq* lives in channel (same slot the classic path and destroy_cq use). */
  wrap->cq_ex.channel = (void*)vmrc_cq;
  wrap->cq_ex.cq_context = cq_attr->cq_context;
  wrap->cq_ex.cqe = cq_attr->cqe;

  /* Extended poll + read entry points. */
  wrap->cq_ex.start_poll = &vmrc_cq_ex_start_poll;
  wrap->cq_ex.next_poll = &vmrc_cq_ex_next_poll;
  wrap->cq_ex.end_poll = &vmrc_cq_ex_end_poll;
  wrap->cq_ex.read_opcode = &vmrc_cq_ex_read_opcode;
  wrap->cq_ex.read_vendor_err = &vmrc_cq_ex_read_vendor_err;
  wrap->cq_ex.read_byte_len = &vmrc_cq_ex_read_byte_len;
  wrap->cq_ex.read_imm_data = &vmrc_cq_ex_read_imm_data;
  wrap->cq_ex.read_qp_num = &vmrc_cq_ex_read_qp_num;
  wrap->cq_ex.read_src_qp = &vmrc_cq_ex_read_src_qp;
  wrap->cq_ex.read_wc_flags = &vmrc_cq_ex_read_wc_flags;

  /* Dummy verbs context carrying ops.poll_cq, so classic ibv_poll_cq() on the
   * down-cast CQ (ibv_cq_ex_to_cq) still routes through MRC. */
  dummy_verbs_context = calloc(1, sizeof(struct ibv_context));
  VMRC_CHECK_PRINT_EXIT(dummy_verbs_context, 1, "create_cq_ex: unable to allocate the dummy verbs context");
  dummy_verbs_context->ops.poll_cq = &vmrc_ibv_overwrite_poll_cq;
  wrap->cq_ex.context = dummy_verbs_context;

  return &wrap->cq_ex;
}

/* Overwrite for poll_cq. This is passed as a function pointer. */
int vmrc_ibv_overwrite_poll_cq(struct ibv_cq* cq, int num_entries, struct ibv_wc* wc) {
  struct vmrc_symbols_t* symbols;
  struct mrc_cq* vmrc_cq;
  int ret;

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols in verbs-mrc shim layer");

  vmrc_cq = (void*)cq->channel;
  ret = symbols->mrc_poll_cq_internal(vmrc_cq, num_entries, wc);

  return ret;
}

/* Overwrite for ibv_create_cq. */
__asm__(".symver ovwrt_ibv_create_cq, ibv_create_cq@@IBVERBS_1.1");
VMRC_DEF_VIS struct ibv_cq* ovwrt_ibv_create_cq(struct ibv_context* verbs_context, int cqe, void* cq_context,
                                                struct ibv_comp_channel* channel, int comp_vector) {
  struct vmrc_ht* hashtable;
  struct mrc_context* vmrc_context;
  struct vmrc_symbols_t* symbols;
  struct ibv_cq* verbs_cq;
  struct mrc_cq* vmrc_cq;
  struct ibv_context* dummy_verbs_context;

  VMRC_DEBUG_PRINT("In ibv_create_cq");

  VMRC_CHECK_PRINT_EXIT(channel == NULL, 1, "Non-NULL completion channel not yet supported");

  hashtable = vmrc_ht_get();
  VMRC_CHECK_PRINT_EXIT(hashtable, 1, "Could not get context hashtable");

  vmrc_context = (struct mrc_context*)vmrc_ht_search(hashtable, verbs_context);
  VMRC_CHECK_PRINT_EXIT_VA_ARGS(vmrc_context, 1, "Could not find the matching MRC context for verbs context %p",
                                verbs_context);

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols in verbs-mrc shim layer");

  vmrc_cq = symbols->mrc_create_cq_internal(vmrc_context, cqe, cq_context, NULL, comp_vector);
  VMRC_CHECK_PRINT_EXIT(vmrc_cq, 1, "Error in mrc_create_cq");

  /* Allocate dummy verbs cq. */
  verbs_cq = calloc(1, sizeof(struct ibv_cq));
  VMRC_CHECK_PRINT_EXIT(verbs_cq, 1, "Unable to allocate the dummy verbs CQ");

  /* Store vmrc_cq in verbs_cq->channel. */
  verbs_cq->channel = (void*)vmrc_cq;

  /* Put the input cq_context in verbs_cq->cq_context. */
  verbs_cq->cq_context = cq_context;

  /* Allocate dummy verbs context. */
  dummy_verbs_context = calloc(1, sizeof(struct ibv_context));
  VMRC_CHECK_PRINT_EXIT(dummy_verbs_context, 1, "Unable to allocate the dummy verbs context");

  /* Replace poll_cq in the dummy verbs context. */
  dummy_verbs_context->ops.poll_cq = &vmrc_ibv_overwrite_poll_cq;

  /* Put the dummy verbs context in verbs_cq's context. */
  verbs_cq->context = dummy_verbs_context;

  return verbs_cq;
}

/* Overwrite for ibv_destroy_cq. */
__asm__(".symver ovwrt_ibv_destroy_cq, ibv_destroy_cq@@IBVERBS_1.1");
VMRC_DEF_VIS int ovwrt_ibv_destroy_cq(struct ibv_cq* verbs_cq) {
  struct vmrc_symbols_t* symbols;
  struct mrc_cq* vmrc_cq;
  int mrc_errno;

  VMRC_DEBUG_PRINT("In ovwrt_ibv_destroy_cq");

  /* Free the dummy verbs context. */
  free(verbs_cq->context);

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols in verbs-mrc shim layer");

  /* Free MRC cq. */
  vmrc_cq = (void*)verbs_cq->channel;
  mrc_errno = symbols->mrc_destroy_cq_internal(vmrc_cq);
  VMRC_CHECK_PRINT_EXIT(mrc_errno == 0, 1, "mrc_destroy_cq failed");

  /* Free the dummy verbs cq. */
  free(verbs_cq);

  return 0;
}

/* Overwrite of ibv_post_send. */
int vmrc_ibv_overwrite_post_send(struct ibv_qp* qp, struct ibv_send_wr* wr, struct ibv_send_wr** bad_wr) {
  struct vmrc_symbols_t* symbols;
  struct mrc_qp* vmrc_qp;
  int mrc_errno;
  struct ibv_send_wr* tmp_wr = wr;

  VMRC_DEBUG_PRINT("In vmrc_ibv_overwrite_post_send");

  while (tmp_wr != NULL) {
    VMRC_CHECK_PRINT_EXIT_VA_ARGS(
        tmp_wr->opcode == IBV_WR_RDMA_WRITE || tmp_wr->opcode == IBV_WR_RDMA_WRITE_WITH_IMM, 1,
        "Expected opcode to be either RDMA_WRITE or RDMA_WRITE_WITH_IMM. Instead, it is %d", tmp_wr->opcode);
    tmp_wr = tmp_wr->next;
  }

  /* Get MRC QP from QP's send_cq. */
  vmrc_qp = (void*)qp->send_cq;
  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");
  mrc_errno = symbols->mrc_post_send_internal(vmrc_qp, wr, bad_wr);

  return mrc_errno;
}

/* Overwrite of ibv_post_recv. */
int vmrc_ibv_overwrite_post_recv(struct ibv_qp* qp, struct ibv_recv_wr* wr, struct ibv_recv_wr** bad_wr) {
  struct vmrc_symbols_t* symbols;
  struct mrc_qp* vmrc_qp;
  int mrc_errno;

  VMRC_DEBUG_PRINT("In vmrc_ibv_overwrite_post_recv");

  /* Get MRC QP from QP's send_cq. */
  vmrc_qp = (void*)qp->send_cq;
  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");
  mrc_errno = symbols->mrc_post_recv_internal(vmrc_qp, wr, bad_wr);

  return mrc_errno;
}

/* Create a dummy struct ibv_qp. Fill the required quantities in it and send it back. */
__asm__(".symver ovwrt_ibv_create_qp, ibv_create_qp@@IBVERBS_1.1");
VMRC_DEF_VIS struct ibv_qp* ovwrt_ibv_create_qp(struct ibv_pd* pd, struct ibv_qp_init_attr* qp_init_attr) {
  struct vmrc_symbols_t* symbols;
  struct ibv_context *verbs_context, *dummy_verbs_context;
  struct vmrc_ht* hashtable;
  struct mrc_context* vmrc_context;
  struct mrc_qp* vmrc_qp;
  struct ibv_qp* verbs_qp;
  struct mrc_qp_init_attr mrc_qp_attr;
  void* ptr;
  int mrc_errno;

  VMRC_DEBUG_PRINT("In ibv_create_qp");

  /* Check if the QP type is RC. Other QP types will cause an error. */
  VMRC_CHECK_PRINT_EXIT(qp_init_attr->qp_type == IBV_QPT_RC, 1, "Only RC QP types are supported");

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  /* Get verbs context and get the corresponding MRC context through hashtable. */
  verbs_context = pd->context;
  VMRC_CHECK_PRINT_EXIT(verbs_context, 1, "pd->context turned out to be NULL");
  hashtable = vmrc_ht_get();
  VMRC_CHECK_PRINT_EXIT(hashtable, 1, "Could not get hashtable");
  vmrc_context = vmrc_ht_search(hashtable, verbs_context);
  VMRC_CHECK_PRINT_EXIT_VA_ARGS(vmrc_context, 1, "Could not find the matching MRC context for verbs context %p",
                                verbs_context);

  /* Fill MRC QP attributes. */
  memset(&mrc_qp_attr, 0, sizeof(struct mrc_qp_init_attr));
  mrc_qp_attr.qp_context = qp_init_attr->qp_context;
  mrc_qp_attr.send_cq = (void*)qp_init_attr->send_cq->channel;
  mrc_qp_attr.recv_cq = (void*)qp_init_attr->recv_cq->channel;
  mrc_qp_attr.pd = pd;
  mrc_qp_attr.cap = qp_init_attr->cap; /* Copy the entire struct. */
  mrc_qp_attr.sq_sig_all = qp_init_attr->sq_sig_all;

  /* Create MRC QP. */
  vmrc_qp = symbols->mrc_create_qp_internal(vmrc_context, &mrc_qp_attr);
  VMRC_CHECK_PRINT_EXIT(vmrc_qp, 1, "Error while calling mrc_create_qp");

  /* Allocate a dummy ibv qp. */
  verbs_qp = calloc(1, sizeof(struct ibv_qp));
  VMRC_CHECK_PRINT_EXIT(verbs_qp, 1, "Unable to allocate the dummy verbs QP");

  /* Put MRC qp_num in verbs_qp->qp_num. */
  mrc_errno = symbols->mrc_get_qpn_internal(vmrc_qp, &verbs_qp->qp_num);
  VMRC_CHECK_PRINT_EXIT(mrc_errno == 0, 1, "Unable to call mrc_get_qpn");

  /* Put qp_context. */
  verbs_qp->qp_context = qp_init_attr->qp_context;

  /* Put MRC QP in send_cq. */
  verbs_qp->send_cq = (void*)vmrc_qp;

  /* Allocate dummy verbs context. */
  dummy_verbs_context = calloc(1, sizeof(struct ibv_context));
  VMRC_CHECK_PRINT_EXIT(dummy_verbs_context, 1, "Unable to allocate the dummy verbs context");

  /* Put overwrites of post_send, post_recv. */
  dummy_verbs_context->ops.post_send = &vmrc_ibv_overwrite_post_send;
  dummy_verbs_context->ops.post_recv = &vmrc_ibv_overwrite_post_recv;

  /* Put the dummy verbs context in the returned QP's qp->context. */
  verbs_qp->context = dummy_verbs_context;

  /* Put the actual verbs context in verbs_qp->pd. This will be used to get gid of this QP when the QP is transitioned
   * to INIT and to get the matching MRC context while creating EV array. Put the verbs context in verbs_qp->recv_cq. */
  verbs_qp->pd = pd;
  verbs_qp->recv_cq = (void*)verbs_context;

  /* Allocate 128 bits (16 uint8_t) + 2 void * entries and assign the pointer to srq. This will be used to store the gid
   * raw of this QP and for storing the input send_cq and recv_cq. */
  verbs_qp->srq = (void*)calloc(16 + 2 * (sizeof(void*) / sizeof(uint8_t)), sizeof(uint8_t));
  ptr = (void*)verbs_qp->srq;
  ptr = ptr + (16 / sizeof(void*)); /* 16 bytes aka 128 bits */
  ptr = (void*)qp_init_attr->send_cq;
  ptr = ptr + 1;
  ptr = (void*)qp_init_attr->recv_cq;

  return verbs_qp;
}

/*
 * Overwrite of ibv_query_qp.
 */

__asm__(".symver ovwrt_ibv_query_qp, ibv_query_qp@@IBVERBS_1.1");
VMRC_DEF_VIS int ovwrt_ibv_query_qp(struct ibv_qp* verbs_qp, struct ibv_qp_attr* vattr, int vattr_mask,
                                    struct ibv_qp_init_attr* vinit_attr) {
  struct vmrc_symbols_t* symbols;
  int mrc_errno;
  struct mrc_qp_init_attr mrc_init_attr;
  struct mrc_qp* vmrc_qp;
  struct mrc_qp_attr mrc_attr;
  int mrc_attr_mask = 0;
  void* ptr;

  VMRC_DEBUG_PRINT("In ibv_query_qp");

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  /* Get MRC qp. */
  vmrc_qp = (void*)verbs_qp->send_cq;

  /* Call mrc query qp internal using mrc_qp and mrc_init_attr. */
  mrc_errno = symbols->mrc_query_qp_internal(vmrc_qp, vattr, vattr_mask, &mrc_attr, mrc_attr_mask, &mrc_init_attr);
  VMRC_CHECK_PRINT_EXIT(mrc_errno == 0, 1, "mrc_query_qp failed");

  /* From the returned mrc_qp_init_attr, fill init_attr (verbs attr). */
  vinit_attr->qp_context = mrc_init_attr.qp_context;
  ptr = (void*)verbs_qp->srq;
  ptr = ptr + (16 / sizeof(void*)); /* First 128 bits are for the raw gid. */
  vinit_attr->send_cq = (void*)ptr;
  ptr = ptr + 1;
  vinit_attr->recv_cq = (void*)ptr;
  vinit_attr->qp_type = IBV_QPT_RC;
  vinit_attr->cap = mrc_init_attr.cap;
  vinit_attr->sq_sig_all = mrc_init_attr.sq_sig_all;

  return 0;
}

/* Overwrite for ibv_destroy_qp. */
__asm__(".symver ovwrt_ibv_destroy_qp, ibv_destroy_qp@@IBVERBS_1.1");
VMRC_DEF_VIS int ovwrt_ibv_destroy_qp(struct ibv_qp* verbs_qp) {
  struct vmrc_symbols_t* symbols;
  struct mrc_qp* vmrc_qp;
  int mrc_errno;

  VMRC_DEBUG_PRINT("In ovwrt_ibv_destroy_qp");

  /* Free the dummy verbs context. */
  free(verbs_qp->context);

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols in verbs-mrc shim layer");

  /* Free MRC qp. */
  vmrc_qp = (void*)verbs_qp->send_cq;
  mrc_errno = symbols->mrc_destroy_qp_internal(vmrc_qp);
  VMRC_CHECK_PRINT_EXIT(mrc_errno == 0, 1, "mrc_destroy_qp failed");

  /* Free the dummy srq. */
  free(verbs_qp->srq);

  /* Free the dummy verbs qp. */
  free(verbs_qp);

  return 0;
}

/* Overwrite for ibv_modify_qp. */
__asm__(".symver ovwrt_ibv_modify_qp, ibv_modify_qp@@IBVERBS_1.1");
VMRC_DEF_VIS int ovwrt_ibv_modify_qp(struct ibv_qp* verbs_qp, struct ibv_qp_attr* vattr, int vattr_mask) {
  struct vmrc_symbols_t* symbols;
  struct mrc_qp* vmrc_qp;
  struct mrc_qp_attr mrc_attr;
  int mrc_attr_mask;
  struct ibv_context* verbs_context;
  struct vmrc_ht* hashtable;
  struct mrc_context* vmrc_context;
  uint8_t* gid_raw;
  struct mrc_qp_hint_init_attr vmrc_qp_hint_init_attr;
  struct mrc_qp_hint* vmrc_qp_hint;
  void* addr_of_value = NULL;
  int mrc_errno;

  VMRC_DEBUG_PRINT("In ibv_modify_qp");

  symbols = vmrc_symbols_get();
  VMRC_CHECK_PRINT_EXIT(symbols, 1, "Could not get symbols");

  /* Get MRC QP from send_cq. */
  vmrc_qp = (void*)verbs_qp->send_cq;

  if (vattr->qp_state == IBV_QPS_INIT) {
    /* Get MRC context. */
    verbs_context = (void*)verbs_qp->recv_cq;
    hashtable = vmrc_ht_get();
    VMRC_CHECK_PRINT_EXIT(hashtable, 1, "Could not get context hashtable");
    vmrc_context = (struct mrc_context*)vmrc_ht_search_plus_addr(hashtable, verbs_context, &addr_of_value);
    VMRC_CHECK_PRINT_EXIT(vmrc_context, 1, "Could not find matching MRC context");

    /* Create MRC QP hint. */
    memset(&vmrc_qp_hint_init_attr, 0, sizeof(struct mrc_qp_hint_init_attr));
    vmrc_qp_hint_init_attr.attr.num_qps_per_peer = 1;
    vmrc_qp_hint_init_attr.attr.num_send_peers = 1;
    vmrc_qp_hint = symbols->mrc_create_qp_hint_internal(vmrc_context, &vmrc_qp_hint_init_attr);
    vmrc_ht_attr_insert(addr_of_value, (void*)vmrc_qp_hint, VMRC_HT_ATTR_QP_HINT_IDX);

    mrc_attr_mask = MRC_QP_HINT;
    memset(&mrc_attr, 0, sizeof(struct mrc_qp_attr));
    mrc_attr.qp_hint = vmrc_qp_hint;

  } else if (vattr->qp_state == IBV_QPS_RTR) {
    union ibv_gid my_gid;

    VMRC_CHECK_PRINT_EXIT(vattr->ah_attr.is_global == 1, 1,
                          "vattr->ah_attr.is_global is not 1. verbs_mrc only accepts global gids\n");

    /* Get the GID of this QP and store it in verbs_qp->srq. Assume correct port_num is passed during RTR transition. */
    verbs_context = (void*)verbs_qp->recv_cq;
    VMRC_CHECK_PRINT_EXIT(symbols->ibv_query_gid_internal(verbs_context, vattr->ah_attr.port_num,
                                                          vattr->ah_attr.grh.sgid_index, &my_gid) == 0,
                          1, "ibv_query_gid failed");

    /* Store my_gid.raw (128 bits) in (void *) verbs_qp->srq. */
    memcpy((void*)verbs_qp->srq, my_gid.raw, 16);

    /* Set the mrc_attr and mrc_attr_mask to pass in the mrc_ev_array. */
    mrc_attr_mask = 0U;
    memset(&mrc_attr, 0, sizeof(mrc_attr));

  } else if (vattr->qp_state == IBV_QPS_RTS) {
    mrc_attr_mask = 0U; /* No MRC related attr mask. */
  }
  mrc_errno = symbols->mrc_modify_qp_internal(vmrc_qp, vattr, vattr_mask, &mrc_attr, mrc_attr_mask);
  /* Reflect the new state in the dummy verbs qp. NCCL accesses ->state in ibvModifyQpLog */
  if (mrc_errno == 0) verbs_qp->state = vattr->qp_state;

  return mrc_errno;
}
