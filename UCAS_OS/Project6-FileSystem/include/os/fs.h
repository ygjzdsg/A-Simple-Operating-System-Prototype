#include <type.h>

#define BLOCK_SIZE 4096       // block size: 4KB
#define SECTOR_SIZE 512       // secotor size: 512B
#define START_SECTOR 1048576  // start sector_num in disk
#define FS_SIZE 1048576    


#define SUPERBLOCK_MEM_ADDR    0x5d000000
#define BMAP_MEM_ADDR          0x5d000200
#define IMAP_MEM_ADDR          0x5d004200
#define INODE_MEM_ADDR         0x5d004400
#define DATA_MEM_ADDR          0x5d044400
    

#define SUPERBLOCK_SD_OFFSET 0
#define BMAP_SD_OFFSET 1
#define IMAP_SD_OFFSET 33
#define INODE_SD_OFFSET 34
#define DATA_SD_OFFSET 546

#define MAGIC_NUM 0x666

#define ROOTINUM 0 
#define TYPE_FILE 0
#define TYPE_DIRECTORY 1

#define SUPERBLOCK_NUM 1
#define BLOCKMAP_NUM 32
#define INODEMAP_NUM 1
#define INODE_NUM 512
#define DATA_BLOCK_NUM 1048030

#define DIRECT_NUM 6    

#define DENTRY_SIZE 32 
#define MAX_FILE_NAME 24
#define MAX_FILE_NUM 16 


#define A_R 1
#define A_W 2
#define A_RW 3

#define INODE_OFFSET_IN_SECTOR sizeof(inode_t)/512

typedef struct superblock{
    uint32_t magic;
    uint32_t size;
    uint32_t start_sector;

    uint32_t blockmap_offset;
    uint32_t blockmap_num;

    uint32_t inodemap_offset;
    uint32_t inodemap_num;

    uint32_t inode_offset;
    uint32_t inode_num;

    uint32_t data_block_offset;
    uint32_t data_block_num;

} superblock_t;

typedef struct inode{
    uint32_t inum;
    uint8_t type;                      
    uint8_t access;                    
    uint16_t ref;                       
    uint32_t size;                     
    uint32_t direct_pointer[DIRECT_NUM]; 
    uint32_t indirect1_pointer[2];      
    uint64_t modified_time;            
} inode_t;
inode_t *current_inode;

typedef struct dentry{
    uint32_t type; 
    uint32_t inum;
    char name[MAX_FILE_NAME];
} dentry_t;

typedef struct fd{
    uint32_t inum;
    uint8_t  access;
    uint32_t read_pointer;
    uint32_t write_pointer; 
} fd_t;
fd_t fd_table[MAX_FILE_NUM];


void do_mkfs();
int check_fs();
void do_statfs();


void do_cd(char *directory);
void do_mkdir(char *directory);
void do_rmdir(char *directory);
void do_ls(char *directory);
void do_ls_l(char *directory);


void do_touch(char *file_name);
void do_cat(char *file_name);
long do_fopen(char *name, int access);
void do_fread(int fd, char *buff, int size);
void do_fwrite(int fd, char *buff, int size);
void do_lseek(int fd,int offset,int whence); 
void do_fclose(int fd);
void do_rm(char *file_name);
void do_ln(char *src_name, char *link_name);

void release(inode_t *inode);
void set_blockmap(uint32_t block_id);
void set_inodemap(uint32_t inode_id);
void clear_blockmap(uint32_t block_id);
void clear_inodemap(uint32_t inode_id);
uint32_t alloc_datablock();
uint32_t alloc_inode();
void write_inode_sector(uint32_t inum);
void init_dentry(uint32_t block_num, uint32_t cur_inum, uint32_t parent_inum);
inode_t *get_inode(uint32_t inum);
inode_t *directory_serach(inode_t *dp, char *name);
int find_path(char *path, int type);
int alloc_fd();
