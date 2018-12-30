#include <bootstrap.h>
#include <bsm/libbsm.h>
#include <CoreFoundation/CoreFoundation.h>
#include <os/log.h>
#include <sandbox.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#include <mig/capsd.h>
#include <mig/shelld_client.h>

#include <common/decls.h>
#include <common/utils.h>


boolean_t shelld_server(
		mach_msg_header_t *InHeadP,
		mach_msg_header_t *OutHeadP);

const char* sb_profile_template =   "(version 1)\n"
                                    "(deny default)\n"
                                    "(import \"system.sb\")\n"
                                    "(allow process-fork)\n"
                                    "(allow file-read* file-write* (subpath \"/private/tmp/shelld/%s\"))\n"
                                    "(allow file-read-data file-write-data (subpath \"/dev/tty\"))\n"
                                    "(allow file-read* process-exec (subpath \"/bin/\"))\n"
                                    "(allow file-read* process-exec (subpath \"/usr/bin/\"))\n"
                                    "(allow file-read* process-exec (subpath \"/usr/sbin/\"))\n";

CFMutableDictionaryRef sessions;
mach_port_t capsd_service_port;

CFMutableDictionaryRef lookup_session(const char* name, audit_token_t client) {
    CFStringRef key = CFStringCreateWithCString(kCFAllocatorDefault, name, kCFStringEncodingASCII);

    CFMutableDictionaryRef session = NULL;
    if (CFDictionaryGetValueIfPresent(sessions, key, (const void**)&session)) {
        CFNumberRef cf_owner_pid = CFDictionaryGetValue(session, CFSTR("pid"));
        int owner_pid;
        ASSERT(CFNumberGetValue(cf_owner_pid, kCFNumberSInt32Type, &owner_pid));
        if (owner_pid != audit_token_to_pid(client))
            session = NULL;
    }

    CFRelease(key);

    return session;
}

int sandbox_check_with_capabilities(audit_token_t creds, const char* operation, int flags, const char* arg) {
    int result = sandbox_check_by_audit_token(creds, operation, flags, arg);
    if (result != 1) {
        return result;
    }

    int client_has_capability = 0;
    pid_t pid = audit_token_to_pid(creds);
    has_capability(capsd_service_port, pid, operation, arg, &client_has_capability);

    return !client_has_capability;
}

void handle_process_exited(pid_t pid, CFMutableDictionaryRef session, int output_fileno) {
    int status;
    waitpid(pid, &status, 0);

    os_log(OS_LOG_DEFAULT, "shelld: child %d exited with status %d", pid, status);

    char output[4096];
    size_t nread = read(output_fileno, output, sizeof(output) - 1);
    output[nread] = 0;

    CFNumberRef value;
    if (CFDictionaryGetValueIfPresent(session, CFSTR("listener"), (const void**)&value)) {
        mach_port_t listener;
        ASSERT(CFNumberGetValue(value, kCFNumberSInt32Type, &listener));
        shelld_client_notify(listener, status, output);
    }

    close(output_fileno);
    CFRelease(session);
}

kern_return_t shell_exec(mach_port_t server, const char* session_name, const char* command, audit_token_t creds) {
    if (!command || strlen(command) == 0)
        return KERN_FAILURE;

    if (sandbox_check_with_capabilities(creds, "process-exec*", SANDBOX_CHECK_NO_REPORT, "/bin/bash")) {
        os_log(OS_LOG_DEFAULT, "shelld: denying request to sandboxed client %d\n", audit_token_to_pid(creds));
        return KERN_FAILURE;
    }

    CFMutableDictionaryRef session = lookup_session(session_name, creds);
    if (!session)
        return KERN_FAILURE;

    int fds[2];
    ASSERT(pipe(fds) == 0);

    int pid = fork();
    if (pid == 0) {
        char* argv[] = {"/bin/bash", "-c", (char*)command, NULL};
        char* envp[] = {"PATH=/bin:/usr/bin:/usr/sbin", NULL};

        char cwd[1024];
        snprintf(cwd, sizeof(cwd), "/private/tmp/shelld/%s", session_name);
        chdir(cwd);

        char profile[4096];
        snprintf(profile, sizeof(profile), sb_profile_template, session_name);
        sandbox_init(profile, 0, NULL);

        dup2(fds[1], STDOUT_FILENO);
        close(STDERR_FILENO);
        close(STDIN_FILENO);

        close(fds[0]);
        close(fds[1]);

        execve("/bin/bash", argv, envp);
        _exit(-1);
    } else if (pid < 0) {
        return KERN_FAILURE;
    }

    // Keep the session alive until the callback is run
    CFRetain(session);

    close(fds[1]);
    int rfd = fds[0];

    __block int running = true;

    os_log(OS_LOG_DEFAULT, "shelld: bash spawned: %d\n", pid);
    dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_PROC, pid, DISPATCH_PROC_EXIT, dispatch_get_main_queue());
    dispatch_source_set_event_handler(source, ^{
        running = false;
        handle_process_exited(pid, session, rfd);
        dispatch_source_cancel(source);
        dispatch_release(source);
    });
    dispatch_resume(source);

    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, 60 * NSEC_PER_SEC), dispatch_get_main_queue(), ^{
        if (!running)
            return;
        os_log(OS_LOG_DEFAULT, "shelld: killing process %d due to timeout", pid);
        kill(pid, SIGKILL);
    });

    return KERN_SUCCESS;
}

kern_return_t remove_listener(CFMutableDictionaryRef session) {
    CFNumberRef value;

    if (CFDictionaryGetValueIfPresent(session, CFSTR("listener"), (const void**)&value)) {
        mach_port_t listener;
        ASSERT(CFNumberGetValue(value, kCFNumberSInt32Type, &listener));
        mach_port_deallocate(mach_task_self(), listener);
        CFDictionaryRemoveValue(session, CFSTR("listener"));
        return KERN_SUCCESS;
    } else {
        return KERN_FAILURE;
    }
}

kern_return_t register_completion_listener(mach_port_t server, const char* session_name, mach_port_t listener, audit_token_t client) {
    CFMutableDictionaryRef session = lookup_session(session_name, client);
    if (!session) {
        mach_port_deallocate(mach_task_self(), listener);
        return KERN_FAILURE;
    }

    CFNumberRef value = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &listener);
    CFDictionaryAddValue(session, CFSTR("listener"), value);
    CFRelease(value);

    return KERN_SUCCESS;
}

kern_return_t unregister_completion_listener(mach_port_t server, const char* session_name, audit_token_t client) {
    CFMutableDictionaryRef session = lookup_session(session_name, client);
    if (!session)
        return KERN_FAILURE;

    return remove_listener(session);
}

kern_return_t shelld_create_session(mach_port_t server, const char* session_name, audit_token_t client) {
    for (const char* ptr = session_name; *ptr; ptr++) {
        if (!isalnum(*ptr)) {
            os_log(OS_LOG_DEFAULT, "shelld: denying invalid session name: %s", session_name);
            return KERN_FAILURE;
        }
    }

    CFStringRef key = CFStringCreateWithCString(kCFAllocatorDefault, session_name, kCFStringEncodingASCII);
    if (CFDictionaryContainsKey(sessions, key)) {
        os_log(OS_LOG_DEFAULT, "shelld: session already exists: %s", session_name);
        CFRelease(key);
        return KERN_FAILURE;
    }

    CFMutableDictionaryRef session = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    CFDictionaryAddValue(sessions, key, session);

    pid_t pid = audit_token_to_pid(client);

    CFNumberRef cf_pid = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &pid);
    CFDictionaryAddValue(session, CFSTR("pid"), cf_pid);
    CFRelease(cf_pid);

    char workdir[1024];
    snprintf(workdir, sizeof(workdir), "/private/tmp/shelld/%s", session_name);
    mkdir(workdir, 0777);

    // Note: this is racy: the client could exit and spawn a priviliged process into its PID before the server
    // gets here... Not too easy to exploit though from inside the sandbox so should be fine for a CTF :)
    dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_PROC, pid, DISPATCH_PROC_EXIT, dispatch_get_main_queue());
    dispatch_source_set_event_handler(source, ^{
        os_log(OS_LOG_DEFAULT, "shelld: cleaning up session for dead client %d", pid);

        remove_listener(session);
        CFDictionaryRemoveValue(sessions, key);

        // TODO unlink directory here as well

        CFRelease(session);
        CFRelease(key);

        dispatch_source_cancel(source);
        dispatch_release(source);
    });
    dispatch_resume(source);

    return KERN_SUCCESS;
}

int main(int argc, const char *argv[]) {
    kern_return_t kr;
    mach_port_t bootstrap_port, service_port;

    sessions = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);

    mkdir("/private/tmp/shelld", 0777);

    task_get_special_port(mach_task_self(), TASK_BOOTSTRAP_PORT, &bootstrap_port);

    kr = bootstrap_look_up(bootstrap_port, "net.saelo.capsd", &capsd_service_port);
    ASSERT_KERN_SUCCESS(kr, "bootstrap_look_up");

    kr = bootstrap_check_in(bootstrap_port, "net.saelo.shelld", &service_port);
    ASSERT_KERN_SUCCESS(kr, "bootstrap_check_in");

    dispatch_source_t source = dispatch_source_create(DISPATCH_SOURCE_TYPE_MACH_RECV, service_port, 0, dispatch_get_main_queue());

    dispatch_source_set_event_handler(source, ^{
        dispatch_mig_server(source, MAX_MSG_SIZE, shelld_server);
    });

    dispatch_resume(source);
    dispatch_main();
    exit(-1);
}
