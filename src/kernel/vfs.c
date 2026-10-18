#include "kernel/vfs.h"
#include "kernel/memory.h"
#include "kernel/timer.h"
#include "kernel/console.h"
#include "lib/string.h"

static vfs_node_t* vfs_root = NULL;
static uint32_t next_inode = 1;

void vfs_init(void) {
    vfs_root = NULL;
    next_inode = 1;

    // Create root directory
    vfs_create("/", VFS_TYPE_DIR);

    // Pre-populate system and educational files
    const char* readme_text =
        "Welcome to Mini OS!\n"
        "An educational x86-64 operating system kernel demonstrating\n"
        "booting, paging, CPU exceptions, preemptive scheduling, synchronization, and VFS.\n";
    vfs_create("/readme.txt", VFS_TYPE_FILE);
    vfs_write("/readme.txt", readme_text, strlen(readme_text));

    const char* sysinfo_text =
        "OS Name   : Mini OS\n"
        "Arch      : x86-64 (AMD64 / Long Mode)\n"
        "Author    : nidhaahmed\n"
        "Boot Mode : Multiboot 1 / Freestanding\n"
        "Paging    : 4-Level Hardware Paging Active\n"
        "Scheduler : Preemptive Round-Robin\n";
    vfs_create("/sysinfo.txt", VFS_TYPE_FILE);
    vfs_write("/sysinfo.txt", sysinfo_text, strlen(sysinfo_text));

    const char* hello_c_text =
        "#include <stdio.h>\n"
        "#include <unistd.h>\n"
        "\n"
        "int main(void) {\n"
        "    printf(\"Hello from Mini OS Process!\\n\");\n"
        "    return 0;\n"
        "}\n";
    vfs_create("/experiments/hello.c", VFS_TYPE_FILE);
    vfs_write("/experiments/hello.c", hello_c_text, strlen(hello_c_text));

    kprintf("[+] RAMFS VFS    : Initialized In-Memory File System with default files\n");
}

vfs_node_t* vfs_lookup(const char* path) {
    if (!path) return NULL;

    vfs_node_t* cur = vfs_root;
    while (cur) {
        if (strcmp(cur->path, path) == 0) {
            return cur;
        }
        cur = cur->next;
    }
    return NULL;
}

vfs_node_t* vfs_create(const char* path, uint32_t type) {
    if (!path) return NULL;

    vfs_node_t* existing = vfs_lookup(path);
    if (existing) {
        return existing;
    }

    vfs_node_t* node = (vfs_node_t*)kmalloc(sizeof(vfs_node_t));
    if (!node) return NULL;
    memset(node, 0, sizeof(vfs_node_t));

    node->inode_id = next_inode++;
    strncpy(node->path, path, sizeof(node->path) - 1);
    node->type = type;
    node->size = 0;
    node->data = NULL;
    node->created_tick = (uint32_t)timer_get_ticks();
    node->modified_tick = node->created_tick;

    node->next = vfs_root;
    vfs_root = node;

    return node;
}

int vfs_write(const char* path, const void* buffer, size_t size) {
    vfs_node_t* node = vfs_lookup(path);
    if (!node) {
        node = vfs_create(path, VFS_TYPE_FILE);
        if (!node) return -1;
    }

    if (node->data) {
        kfree(node->data);
        node->data = NULL;
    }

    node->data = (uint8_t*)kmalloc(size + 1);
    if (!node->data) return -1;

    memcpy(node->data, buffer, size);
    node->data[size] = '\0';
    node->size = size;
    node->modified_tick = (uint32_t)timer_get_ticks();

    return (int)size;
}

int vfs_read(const char* path, void* buffer, size_t max_size) {
    vfs_node_t* node = vfs_lookup(path);
    if (!node || !node->data || node->type != VFS_TYPE_FILE) {
        return -1;
    }

    size_t to_read = node->size < max_size ? node->size : max_size;
    memcpy(buffer, node->data, to_read);

    return (int)to_read;
}

int vfs_delete(const char* path) {
    if (!path || strcmp(path, "/") == 0) return -1;

    vfs_node_t* prev = NULL;
    vfs_node_t* cur = vfs_root;

    while (cur) {
        if (strcmp(cur->path, path) == 0) {
            if (prev) {
                prev->next = cur->next;
            } else {
                vfs_root = cur->next;
            }

            if (cur->data) {
                kfree(cur->data);
            }
            kfree(cur);
            return 0;
        }
        prev = cur;
        cur = cur->next;
    }
    return -1;
}

void vfs_list(void) {
    kprintf("\n=============================== IN-MEMORY VFS LISTING ===============================\n");
    kprintf("INODE | TYPE | SIZE (BYTES) | MODIFIED TICK | PATH\n");
    kprintf("------+------+--------------+---------------+----------------------------------------\n");

    vfs_node_t* cur = vfs_root;
    while (cur) {
        const char* type_str = (cur->type == VFS_TYPE_DIR) ? "DIR " : "FILE";
        kprintf("  %u   | %s | %u bytes", cur->inode_id, type_str, (uint32_t)cur->size);
        if (cur->size < 10) kprintf("      ");
        else if (cur->size < 100) kprintf("     ");
        else if (cur->size < 1000) kprintf("    ");
        else kprintf("   ");

        kprintf("| Tick %u", cur->modified_tick);
        if (cur->modified_tick < 10) kprintf("        ");
        else if (cur->modified_tick < 100) kprintf("       ");
        else if (cur->modified_tick < 1000) kprintf("      ");
        else kprintf("     ");

        kprintf("| %s\n", cur->path);
        cur = cur->next;
    }
    kprintf("=====================================================================================\n");
}
