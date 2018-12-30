#include <bootstrap.h>
#include <bsm/libbsm.h>
#include <CoreFoundation/CoreFoundation.h>
#include <os/log.h>
#include <stdio.h>
#include <stdlib.h>
#include <xpc/xpc.h>

#include <common/utils.h>
#include <common/decls.h>

CFMutableDictionaryRef capabilities_by_pid;

boolean_t capsd_server
	(mach_msg_header_t *InHeadP, mach_msg_header_t *OutHeadP);

CFMutableDictionaryRef get_or_create_capabilities_for_pid(pid_t pid) {
    // Check if the process exists. This is racy though...
    if (kill(pid, 0) != 0 && errno == ESRCH) {
        return NULL;
    }

    CFNumberRef key = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &pid);

    CFMutableDictionaryRef capabilities;
    if (!CFDictionaryGetValueIfPresent(capabilities_by_pid, key, (const void**)&capabilities)) {
        capabilities = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFDictionaryAddValue(capabilities_by_pid, key, capabilities);
        CFRelease(capabilities);

        dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_PROC, pid, DISPATCH_PROC_EXIT, dispatch_get_main_queue());
        dispatch_source_set_event_handler(source, ^{
            os_log(OS_LOG_DEFAULT, "cleaning up capabilities for dead client %d", pid);

            CFDictionaryRemoveValue(capabilities_by_pid, key);

            CFRelease(key);

            dispatch_source_cancel(source);
            dispatch_release(source);
        });
        dispatch_resume(source);
    } else {
        CFRelease(key);
    }

    return capabilities;
}

kern_return_t grant_capability_internal(audit_token_t token, pid_t target, const char* op, const char* arg) {
    if (sandbox_check_by_audit_token(token, op, SANDBOX_CHECK_NO_REPORT, arg, NULL, NULL, NULL) == 0) {
        CFMutableDictionaryRef capabilities = get_or_create_capabilities_for_pid(target);
        if (!capabilities)
            return KERN_FAILURE;

        CFStringRef operation = CFStringCreateWithCString(kCFAllocatorDefault, op, kCFStringEncodingASCII);
        CFStringRef argument = CFStringCreateWithCString(kCFAllocatorDefault, arg, kCFStringEncodingASCII);

        CFMutableSetRef arguments;
        if (!CFDictionaryGetValueIfPresent(capabilities, operation, (const void**)&arguments)) {
            arguments = CFSetCreateMutable(kCFAllocatorDefault, 0, &kCFTypeSetCallBacks);
            CFDictionaryAddValue(capabilities, operation, arguments);
            CFRelease(arguments);
        }

        CFSetSetValue(arguments, argument);

        CFRelease(operation);
        CFRelease(argument);
        return KERN_SUCCESS;
    } else {
        return KERN_FAILURE;
    }
}

int has_capability_internal(pid_t pid, const char* op, const char* arg) {
    int result = 0;

    CFNumberRef key = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &pid);
    CFStringRef operation = CFStringCreateWithCString(kCFAllocatorDefault, op, kCFStringEncodingASCII);
    CFStringRef argument = CFStringCreateWithCString(kCFAllocatorDefault, arg, kCFStringEncodingASCII);

    CFMutableDictionaryRef capabilities;
    if (CFDictionaryGetValueIfPresent(capabilities_by_pid, key, (const void**)&capabilities)) {
        CFMutableSetRef arguments;
        if (CFDictionaryGetValueIfPresent(capabilities, operation, (const void**)&arguments)) {
            result = CFSetContainsValue(arguments, argument);
        }
    }

    CFRelease(key);
    CFRelease(operation);
    CFRelease(argument);

    return result;
}

kern_return_t grant_capability(mach_port_t server, audit_token_t token, pid_t target, const char* op, const char* arg) {
    return grant_capability_internal(token, target, op, arg);
}

kern_return_t has_capability(mach_port_t server, pid_t pid, const char* op, const char* arg, int* out) {
    *out = has_capability_internal(pid, op, arg);
    return KERN_SUCCESS;
}

#define ACTION_GRANT_CAPABILITY 1
#define ACTION_HAS_CAPABILITY 2

int main(int argc, const char *argv[]) {
    os_log(OS_LOG_DEFAULT, "net.saelo.capsd starting");
    
    capabilities_by_pid = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    // Set up MIG service
    kern_return_t kr;

    mach_port_t bootstrap_port, service_port;
    task_get_special_port(mach_task_self(), TASK_BOOTSTRAP_PORT, &bootstrap_port);

    kr = bootstrap_check_in(bootstrap_port, "net.saelo.capsd", &service_port);
    ASSERT_MACH_SUCCESS(kr, "bootstrap_check_in");
    dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_MACH_RECV, service_port, 0, dispatch_get_main_queue());

    dispatch_source_set_event_handler(source, ^{
        dispatch_mig_server(source, MAX_MSG_SIZE, capsd_server);
    });

    dispatch_resume(source);

    // Set up XPC service
    xpc_connection_t service = xpc_connection_create_mach_service("net.saelo.capsd.xpc", NULL, XPC_CONNECTION_MACH_SERVICE_LISTENER);
    xpc_connection_set_target_queue(service, dispatch_get_main_queue());

    xpc_connection_set_event_handler(service, ^(xpc_object_t connection) {
        if (xpc_get_type(connection) == XPC_TYPE_CONNECTION) {
            xpc_connection_set_target_queue(connection, dispatch_get_main_queue());
            xpc_connection_set_event_handler(connection, ^(xpc_object_t msg) {
                if (xpc_get_type(msg) == XPC_TYPE_DICTIONARY) {
                    xpc_object_t reply = xpc_dictionary_create_reply(msg);
                    if (!reply)
                        return;

                    int action = xpc_dictionary_get_uint64(msg, "action");

                    if (action == ACTION_GRANT_CAPABILITY) {
                        audit_token_t creds;
                        // TODO check xpc_dictionary_set_audit_token
                        xpc_dictionary_get_audit_token(msg, &creds);
                        pid_t target = xpc_dictionary_get_int64(msg, "pid");
                        const char* operation = xpc_dictionary_get_string(msg, "operation");
                        const char* argument = xpc_dictionary_get_string(msg, "argument");

                        if (operation && argument) {
                            xpc_dictionary_set_bool(reply, "success", grant_capability_internal(creds, target, operation, argument) == KERN_SUCCESS);
                        } else {
                            xpc_dictionary_set_bool(reply, "success", false);
                        }
                    } else if (action == ACTION_HAS_CAPABILITY) {
                        pid_t target = xpc_dictionary_get_int64(msg, "pid");
                        const char* operation = xpc_dictionary_get_string(msg, "operation");
                        const char* argument = xpc_dictionary_get_string(msg, "argument");
                        xpc_dictionary_set_bool(reply, "success", has_capability_internal(target, operation, argument));
                    } else {
                        xpc_dictionary_set_bool(reply, "success", false);
                    }

                    xpc_connection_send_message(connection, reply);
                } else {
                    if (xpc_get_type(msg) != XPC_TYPE_ERROR || msg != XPC_ERROR_CONNECTION_INVALID) {
                        char* description = xpc_copy_description(msg);
                        os_log(OS_LOG_DEFAULT, "Received unexpected event on connection: %{public}s\n", description);
                        free(description);
                    }
                }
            });
            xpc_connection_resume(connection);
        } else {
            char* description = xpc_copy_description(connection);
            os_log(OS_LOG_DEFAULT, "Received unexpected event: %{public}s\n", description);
            free(description);
        }
    });
    xpc_connection_resume(service);
 
    // Start the main loop
    dispatch_main();
 
    // dispatch_main() never returns
    exit(EXIT_FAILURE);
}
