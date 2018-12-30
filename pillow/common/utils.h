#ifndef UTILS_H
#define UTILS_H

#define MAX_MSG_SIZE (8 * 1024 - MAX_TRAILER_SIZE)

#define ASSERT(c) if (!(c)) { printf("[-] assertion \"" #c "\" failed!\n"); exit(-1); }
#define ASSERT_MACH_SUCCESS(r, name) if (r != 0) { printf("[-] %s failed: %s!\n", name, mach_error_string(r)); exit(-1); }
#define ASSERT_SUCCESS(r, name) if (r != 0) { printf("[-] %s failed!\n", name); exit(-1); }
#define ASSERT_KERN_SUCCESS(r, name) if (r != 0) { printf("[-] %s failed!\n", name); exit(-1); }

#endif
