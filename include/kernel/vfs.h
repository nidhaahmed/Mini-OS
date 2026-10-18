#ifndef VFS_H
#define VFS_H

#include <stdint.h>
#include <stddef.h>

#define VFS_TYPE_FILE 1
#define VFS_TYPE_DIR  2

#define MAX_PATH_LEN  64
#define MAX_FILES     32

typedef struct vfs_node {
    uint32_t inode_id;
    char path[MAX_PATH_LEN];
    uint32_t type;
    size_t size;
    uint8_t* data;
    uint32_t created_tick;
    uint32_t modified_tick;
    struct vfs_node* next;
} vfs_node_t;

void vfs_init(void);
vfs_node_t* vfs_create(const char* path, uint32_t type);
int vfs_write(const char* path, const void* buffer, size_t size);
int vfs_read(const char* path, void* buffer, size_t max_size);
int vfs_delete(const char* path);
vfs_node_t* vfs_lookup(const char* path);
void vfs_list(void);

#endif /* VFS_H */
