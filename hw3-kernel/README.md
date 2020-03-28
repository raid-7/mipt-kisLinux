Phone Directory Kernel Module
===========================

Module is tested against Linux 5.5.13.

### Directory structure

- client - userspace clients: `ch_dircli` works through character device and `sys_dircli` leverages syscall interface
- kernel - loadable kernel module
- linux-5.5.y - changed in-tree files

### Scripts

- build.sh - build module

  Attention: rebuild the kernel with changed files first. You probably also have to `mkdir build` manually.

- install.sh - install module and setup character device

- test.sh - try it out

  Note: `ch_dircli` will probably require root.
  

