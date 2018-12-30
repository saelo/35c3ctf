#include <bootstrap.h>
#include <stdio.h>
#include <stdlib.h>
#include <xpc/xpc.h>
#include <os/log.h>

#include <mig/shelld.h>
#include <mig/shelld_client.h>

#include <common/utils.h>
#include <common/decls.h>

boolean_t shelld_client_server(
		mach_msg_header_t *InHeadP,
		mach_msg_header_t *OutHeadP);


kern_return_t shelld_client_notify(mach_port_t listener, int status, const char* output) {
    printf("Command finished with status %d and output: %s\n", status, output);
    return KERN_SUCCESS;
}

int main() {
    printf("PID: %d\n", getpid());
    puts("Press enter to continue...");
    getchar();

    mach_port_t bp, sp;
    task_get_special_port(mach_task_self(), TASK_BOOTSTRAP_PORT, &bp);
    kern_return_t kr = bootstrap_look_up(bp, "net.saelo.shelld", &sp);
    ASSERT_SUCCESS(kr, "bootstrap_look_up");

    mach_port_t listener, listener_send_right;
    mach_msg_type_name_t aquired_right;
    mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &listener);
    mach_port_extract_right(mach_task_self(), listener, MACH_MSG_TYPE_MAKE_SEND, &listener_send_right, &aquired_right);

    if (shelld_create_session(sp, "foo") != KERN_SUCCESS) {
        puts("Failed to create session");
        exit(-1);
    }

    register_completion_listener(sp, "foo", listener_send_right);
    mach_port_deallocate(mach_task_self(), listener_send_right);

    dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_MACH_RECV, listener, 0, dispatch_get_main_queue());
    dispatch_source_set_event_handler(source, ^{
        dispatch_mig_server(source, MAX_MSG_SIZE, shelld_client_server);
    });
    dispatch_activate(source);

    printf("%d\n", shell_exec(sp, "foo", "echo Hello World > bar"));
    printf("%d\n", shell_exec(sp, "foo", "cat bar"));
    printf("%d\n", shell_exec(sp, "foo", "cat bar"));

    dispatch_main();
    return 0;
}
