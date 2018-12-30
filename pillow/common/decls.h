#ifndef DECLS_H
#define DECLS_H

#include <sys/types.h>
#include <xpc/xpc.h>
#include <dispatch/dispatch.h>

extern int SANDBOX_CHECK_NO_REPORT;
int sandbox_check_by_audit_token(audit_token_t token, const char* operation, int flags, ...);

void xpc_dictionary_get_audit_token(xpc_object_t dict, audit_token_t* token);

typedef boolean_t (*dispatch_mig_callback_t)(mach_msg_header_t *message,
		mach_msg_header_t *reply);
mach_msg_return_t dispatch_mig_server(dispatch_source_t ds, size_t maxmsgsz, dispatch_mig_callback_t callback);

#endif
