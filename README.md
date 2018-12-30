# 35c3ctf

Source code, binaries, and example exploits for the 35c3ctf challenges "WebKid", "pillow", and "chaingineering".

## WebKid

A modified WebKit with a new optimization which breaks some invariants of the JavaScript engine. Exploiting these will result in shellcode execution inside the WebContent sandbox. The sandbox was modified to allow read access to /flag1 and IPC lookup of the "pillow" services.

## Pillow

Two custom macOS system services. The challenge was inspired by https://github.com/bazad/blanket and allows one to hijack the IPC connection between the two services to finally run arbitrary code outside of the sandbox. The challenge was hosted on a seperate VM and one could read /flag3 once outside of the sandbox.

## Chaingineering

The combination of the previous two challenges. One has to combine the WebKit and sandbox escape exploit into a single chain, then read /flag2 from outside the sandbox on the WebKid VM.
