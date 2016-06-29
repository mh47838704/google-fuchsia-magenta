// Copyright 2016 The Fuchsia Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <fcntl.h>
#include <magenta/processargs.h>
#include <magenta/syscalls.h>
#include <mxio/debug.h>
#include <mxio/util.h>
#include <runtime/process.h>
#include <runtime/thread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "devmgr.h"

#define VC_COUNT 4

int devicehost(int argc, char** argv) {
    devhost_handle = mxr_process_get_handle(MX_HND_INFO(MX_HND_TYPE_USER1, 0));
    if (devhost_handle <= 0) {
        printf("devhost: no rpc handle?!\n");
        return -1;
    }
    if (argc != 3) {
        return -1;
    }
    if (!strncmp(argv[1], "pci=", 4)) {
        uint32_t index = strtoul(argv[1] + 4, NULL, 10);
        uintptr_t id = strtoull(argv[2], NULL, 10);

        printf("devhost: pci host %d: driver: %p\n", index, (void*)id);
        devmgr_init(true);
        mx_device_t* pcidev;
        if (devmgr_create_pcidev(&pcidev, index)) {
            printf("devhost: cannot create pci device\n");
            return -1;
        }
        device_add(pcidev, devmgr_device_root());
        printf("devhost: load drivers\n");
        devmgr_init_builtin_drivers();
        printf("devhost: message loop\n");
        devmgr_handle_messages();
    }
    printf("devhost: exiting\n");
    return 0;
}

int console_starter(void* arg) {
    printf("devmgr: vc startup\n");
    // don't start a shell on vc0, since it is the debug console
    for (uint i = 1; i < VC_COUNT;) {
        int fd;
        char name[64];
        snprintf(name, sizeof(name), "/dev/protocol/char/vc%u", i);
        char pname[32];
        snprintf(pname, sizeof(pname), "mxsh:vc%u", i);
        //printf("? %s\n", name);
        if ((fd = open(name, O_RDWR)) < 0) {
            _magenta_nanosleep(100000000ULL);
            continue;
        }
        close(fd);
        devmgr_launch(pname, "/boot/bin/mxsh", name);
        i++;
    }
    return 0;
}

int main(int argc, char** argv) {
    mx_handle_t bootfs_vmo;
    mx_status_t status;
    uint64_t bootfs_size;
    uintptr_t bootfs_val;

    devmgr_io_init();

    if (argc > 1) {
        return devicehost(argc, argv);
    }

    bootfs_vmo = mxr_process_get_handle(MX_HND_INFO(MX_HND_TYPE_USER0, 0));
    status = _magenta_vm_object_get_size(bootfs_vmo, &bootfs_size);
    if (status < 0) {
        cprintf("devmgr: failed to get bootfs size (%d)\n", status);
        return -1;
    }
    status = _magenta_process_vm_map(0, bootfs_vmo, 0, bootfs_size,
                                     &bootfs_val, MX_VM_FLAG_PERM_READ);
    if (status < 0) {
        cprintf("devmgr: failed to map bootfs (%d)\n", status);
        return -1;
    }

    printf("devmgr: main()\n");
    devmgr_init(false);
    devmgr_vfs_init((void*)bootfs_val, bootfs_size);

    printf("devmgr: load drivers\n");
    devmgr_init_builtin_drivers();

    mxr_thread_t* t;
    if ((mxr_thread_create(console_starter, NULL, "console-starter", &t)) == 0) {
        mxr_thread_detach(t);
    }

#if !WITH_APP_SHELL
    // if no kernel shell on serial uart, start a mxsh there
    printf("devmgr: shell startup\n");
    devmgr_launch("mxsh:console", "/boot/bin/mxsh", "/dev/console");
#endif

    devmgr_launch("netsvc", "/boot/bin/netsvc", "/dev/console");

    devmgr_handle_messages();
    printf("devmgr: message handler returned?!\n");
    return 0;
}