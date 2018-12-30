#include <stdio.h>
#include <stdlib.h>
#include <xpc/xpc.h>
#include <os/log.h>

#define ACTION_GRANT_CAPABILITY 1
#define ACTION_HAS_CAPABILITY 2

int main(int argc, const char *argv[]) {
    xpc_connection_t connection = xpc_connection_create_mach_service("net.saelo.capsd.xpc", NULL, 0);
    xpc_connection_set_event_handler(connection, ^(xpc_object_t event) {
    });
    xpc_connection_resume(connection);

    pid_t pid;
    puts("Enter pid:");
    scanf("%d", &pid);
    printf("Adding capability 'process-exec*' for resource '/bin/bash' to process %d\n", pid);

    xpc_object_t msg = xpc_dictionary_create(NULL, NULL, 0);
    xpc_dictionary_set_uint64(msg, "action", ACTION_GRANT_CAPABILITY);
    xpc_dictionary_set_int64(msg, "pid", pid);
    xpc_dictionary_set_string(msg, "operation", "process-exec*");
    xpc_dictionary_set_string(msg, "argument", "/bin/bash");
    xpc_object_t reply = xpc_connection_send_message_with_reply_sync(connection, msg);

    char* description = xpc_copy_description(reply);
    printf("Reply: %s\n", description);

    return 0;
}
