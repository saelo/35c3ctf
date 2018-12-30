#ifndef	_shelld_user_
#define	_shelld_user_

/* Module shelld */

#include <string.h>
#include <mach/ndr.h>
#include <mach/boolean.h>
#include <mach/kern_return.h>
#include <mach/notify.h>
#include <mach/mach_types.h>
#include <mach/message.h>
#include <mach/mig_errors.h>
#include <mach/port.h>
	
/* BEGIN VOUCHER CODE */

#ifndef KERNEL
#if defined(__has_include)
#if __has_include(<mach/mig_voucher_support.h>)
#ifndef USING_VOUCHERS
#define USING_VOUCHERS
#endif
#ifndef __VOUCHER_FORWARD_TYPE_DECLS__
#define __VOUCHER_FORWARD_TYPE_DECLS__
#ifdef __cplusplus
extern "C" {
#endif
	extern boolean_t voucher_mach_msg_set(mach_msg_header_t *msg) __attribute__((weak_import));
#ifdef __cplusplus
}
#endif
#endif // __VOUCHER_FORWARD_TYPE_DECLS__
#endif // __has_include(<mach/mach_voucher_types.h>)
#endif // __has_include
#endif // !KERNEL
	
/* END VOUCHER CODE */

	
/* BEGIN MIG_STRNCPY_ZEROFILL CODE */

#if defined(__has_include)
#if __has_include(<mach/mig_strncpy_zerofill_support.h>)
#ifndef USING_MIG_STRNCPY_ZEROFILL
#define USING_MIG_STRNCPY_ZEROFILL
#endif
#ifndef __MIG_STRNCPY_ZEROFILL_FORWARD_TYPE_DECLS__
#define __MIG_STRNCPY_ZEROFILL_FORWARD_TYPE_DECLS__
#ifdef __cplusplus
extern "C" {
#endif
	extern int mig_strncpy_zerofill(char *dest, const char *src, int len) __attribute__((weak_import));
#ifdef __cplusplus
}
#endif
#endif /* __MIG_STRNCPY_ZEROFILL_FORWARD_TYPE_DECLS__ */
#endif /* __has_include(<mach/mig_strncpy_zerofill_support.h>) */
#endif /* __has_include */
	
/* END MIG_STRNCPY_ZEROFILL CODE */


#ifdef AUTOTEST
#ifndef FUNCTION_PTR_T
#define FUNCTION_PTR_T
typedef void (*function_ptr_t)(mach_port_t, char *, mach_msg_type_number_t);
typedef struct {
        char            *name;
        function_ptr_t  function;
} function_table_entry;
typedef function_table_entry   *function_table_t;
#endif /* FUNCTION_PTR_T */
#endif /* AUTOTEST */

#ifndef	shelld_MSG_COUNT
#define	shelld_MSG_COUNT	4
#endif	/* shelld_MSG_COUNT */

#include <mach/std_types.h>
#include <mach/mig.h>
#include <mach/mig.h>
#include <mach/mach_types.h>
#include <mach_debug/mach_debug_types.h>
#include "../common/types.h"

#ifdef __BeforeMigUserHeader
__BeforeMigUserHeader
#endif /* __BeforeMigUserHeader */

#include <sys/cdefs.h>
__BEGIN_DECLS


/* Routine shelld_create_session */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t shelld_create_session
(
	mach_port_t server,
	string name
);

/* Routine shell_exec */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t shell_exec
(
	mach_port_t server,
	string session,
	string command
);

/* Routine register_completion_listener */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t register_completion_listener
(
	mach_port_t server,
	string session,
	mach_port_t listener
);

/* Routine unregister_completion_listener */
#ifdef	mig_external
mig_external
#else
extern
#endif	/* mig_external */
kern_return_t unregister_completion_listener
(
	mach_port_t server,
	string session
);

__END_DECLS

/********************** Caution **************************/
/* The following data types should be used to calculate  */
/* maximum message sizes only. The actual message may be */
/* smaller, and the position of the arguments within the */
/* message layout may vary from what is presented here.  */
/* For example, if any of the arguments are variable-    */
/* sized, and less than the maximum is sent, the data    */
/* will be packed tight in the actual message to reduce  */
/* the presence of holes.                                */
/********************** Caution **************************/

/* typedefs for all requests */

#ifndef __Request__shelld_subsystem__defined
#define __Request__shelld_subsystem__defined

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		mach_msg_type_number_t nameOffset; /* MiG doesn't use it */
		mach_msg_type_number_t nameCnt;
		char name[4096];
	} __Request__shelld_create_session_t __attribute__((unused));
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		mach_msg_type_number_t sessionOffset; /* MiG doesn't use it */
		mach_msg_type_number_t sessionCnt;
		char session[4096];
		mach_msg_type_number_t commandOffset; /* MiG doesn't use it */
		mach_msg_type_number_t commandCnt;
		char command[4096];
	} __Request__shell_exec_t __attribute__((unused));
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		/* start of the kernel processed data */
		mach_msg_body_t msgh_body;
		mach_msg_port_descriptor_t listener;
		/* end of the kernel processed data */
		NDR_record_t NDR;
		mach_msg_type_number_t sessionOffset; /* MiG doesn't use it */
		mach_msg_type_number_t sessionCnt;
		char session[4096];
	} __Request__register_completion_listener_t __attribute__((unused));
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		mach_msg_type_number_t sessionOffset; /* MiG doesn't use it */
		mach_msg_type_number_t sessionCnt;
		char session[4096];
	} __Request__unregister_completion_listener_t __attribute__((unused));
#ifdef  __MigPackStructs
#pragma pack()
#endif
#endif /* !__Request__shelld_subsystem__defined */

/* union of all requests */

#ifndef __RequestUnion__shelld_subsystem__defined
#define __RequestUnion__shelld_subsystem__defined
union __RequestUnion__shelld_subsystem {
	__Request__shelld_create_session_t Request_shelld_create_session;
	__Request__shell_exec_t Request_shell_exec;
	__Request__register_completion_listener_t Request_register_completion_listener;
	__Request__unregister_completion_listener_t Request_unregister_completion_listener;
};
#endif /* !__RequestUnion__shelld_subsystem__defined */
/* typedefs for all replies */

#ifndef __Reply__shelld_subsystem__defined
#define __Reply__shelld_subsystem__defined

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
	} __Reply__shelld_create_session_t __attribute__((unused));
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
	} __Reply__shell_exec_t __attribute__((unused));
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
	} __Reply__register_completion_listener_t __attribute__((unused));
#ifdef  __MigPackStructs
#pragma pack()
#endif

#ifdef  __MigPackStructs
#pragma pack(4)
#endif
	typedef struct {
		mach_msg_header_t Head;
		NDR_record_t NDR;
		kern_return_t RetCode;
	} __Reply__unregister_completion_listener_t __attribute__((unused));
#ifdef  __MigPackStructs
#pragma pack()
#endif
#endif /* !__Reply__shelld_subsystem__defined */

/* union of all replies */

#ifndef __ReplyUnion__shelld_subsystem__defined
#define __ReplyUnion__shelld_subsystem__defined
union __ReplyUnion__shelld_subsystem {
	__Reply__shelld_create_session_t Reply_shelld_create_session;
	__Reply__shell_exec_t Reply_shell_exec;
	__Reply__register_completion_listener_t Reply_register_completion_listener;
	__Reply__unregister_completion_listener_t Reply_unregister_completion_listener;
};
#endif /* !__RequestUnion__shelld_subsystem__defined */

#ifndef subsystem_to_name_map_shelld
#define subsystem_to_name_map_shelld \
    { "shelld_create_session", 133700 },\
    { "shell_exec", 133701 },\
    { "register_completion_listener", 133702 },\
    { "unregister_completion_listener", 133703 }
#endif

#ifdef __AfterMigUserHeader
__AfterMigUserHeader
#endif /* __AfterMigUserHeader */

#endif	 /* _shelld_user_ */
