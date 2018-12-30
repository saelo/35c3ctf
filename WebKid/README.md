# WebKid

WebKit pwnable.
Patch applies to commit 5ef29f6546523aa48e18da64735aa164c9bc7de6. That's also what the release binaries were built from.

To run minibrowser (WebKid being the uncompressed build directory):

    DYLD_LIBRARY_PATH=/Path/to/WebKid DYLD_FRAMEWORK_PATH=/Path/to/WebKid /Path/to/WebKid/MiniBrowser.app/Contents/MacOS/MiniBrowser

To set default URL to open:

    defaults write org.webkit.MiniBrowser DefaultURL "http://saelo.net"
