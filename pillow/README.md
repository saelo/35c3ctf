# Pillow

macOS sandbox escape challenge.

The challenge consists of two services: capsd and shelld. Both are reachable from within the sandbox. The goal is to achieve code execution outside of the sandbox.

## Setup

Copy the files in distrib/ to the filesystem under the same path, then start the two services:

    sudo launchctl bootstrap system /System/Library/LaunchDaemons/net.saelo.capsd.plist
    sudo launchctl bootstrap system /System/Library/LaunchDaemons/net.saelo.shelld.plist

To later stop or restart the services again do

    sudo launchctl bootout system /System/Library/LaunchDaemons/net.saelo.capsd.plist
    sudo launchctl bootout system /System/Library/LaunchDaemons/net.saelo.shelld.plist
