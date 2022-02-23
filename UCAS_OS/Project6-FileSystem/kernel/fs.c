#include <os/fs.h>
#include <sbi.h>
#include <os/stdio.h>
#include <screen.h>
#include <os/time.h>
#include <os/string.h>
#include <pgtable.h>

inode_t *current_inode;// Current file/dir's inode pointer
fd_t fd_table[MAX_FILE_NUM];

int check_fs(){
    sbi_sd_read(SUPERBLOCK_MEM_ADDR,1,START_SECTOR);
    superblock_t *superblock=(superblock_t *)pa2kva(SUPERBLOCK_MEM_ADDR);

    if(superblock->magic==MAGIC_NUM){
        prints("File System has already existed!\n\r");
        current_inode=(inode_t *)pa2kva(INODE_MEM_ADDR);
        sbi_sd_read(INODE_MEM_ADDR,1,START_SECTOR+INODE_SD_OFFSET);
        return 1;
    }else{
        return 0;
    }
}

void do_mkfs(int mode){
    sbi_sd_read(SUPERBLOCK_MEM_ADDR,1,START_SECTOR);
    superblock_t *superblock=(superblock_t *)pa2kva(SUPERBLOCK_MEM_ADDR);
    printk("[FS] Start initialize filesystem!\n\r");
    printk("[FS] Setting superblock...\n\r");
    kmemset(superblock,0,SECTOR_SIZE);
    superblock->magic=MAGIC_NUM;
    superblock->size=FS_SIZE;
    superblock->start_sector=START_SECTOR;
    superblock->blockmap_offset=BMAP_SD_OFFSET;
    superblock->blockmap_num=BLOCKMAP_NUM;
    superblock->inodemap_offset=IMAP_SD_OFFSET;
    superblock->inodemap_num=INODEMAP_NUM;
    superblock->inode_offset=INODE_SD_OFFSET;
    superblock->inode_num=INODE_NUM;
    superblock->data_block_offset=DATA_SD_OFFSET;
    superblock->data_block_num=DATA_BLOCK_NUM;

    printk("magic : 0x%x\n\r",superblock->magic);
    printk("num sector : %d,start_sector : %d\n\r",superblock->size,superblock->start_sector);
    printk("block map offset : %d(%d)\n\r",superblock->blockmap_offset,superblock->blockmap_num);
    printk("inode map offset : %d(%d)\n\r",superblock->inodemap_offset,superblock->inodemap_num);
    printk("data offset : %d(%d)\n\r",superblock->data_block_offset,superblock->data_block_num);
    printk("inode entry size: 56B,dir entry size : 32B\n\r");
    printk("[FS] Setting block-map...\n\r");
    uint8_t *blockmap=(uint8_t *)pa2kva(BMAP_MEM_ADDR);
    int i;
    kmemset(blockmap,0,SECTOR_SIZE*BLOCKMAP_NUM);
    for(i=0;i<69;i++) 
        blockmap[i/8]|=(1<<(7-(i%8)));

    printk("[FS] Setting inode-map...\n\r");
    uint8_t *inodemap=(uint8_t *)pa2kva(IMAP_MEM_ADDR);
    kmemset(inodemap,0,SECTOR_SIZE*INODEMAP_NUM);
    inodemap[0]=(1<<7);

    sbi_sd_write(SUPERBLOCK_MEM_ADDR,34,START_SECTOR);

    printk("[FS] Setting inode...\n\r");
    inode_t *inode=(inode_t *)pa2kva(INODE_MEM_ADDR);
    inode[0].inum=0;
    inode[0].type=TYPE_DIRECTORY;
    inode[0].access=A_RW;
    inode[0].ref=0;
    inode[0].size=64;
    inode[0].modified_time=get_timer();
    inode[0].direct_pointer[0]=alloc_datablock();
    for(i=1;i<DIRECT_NUM;i++){
        inode[0].direct_pointer[i]=0;
    }
    for(i=0;i<2;i++){
        inode[0].indirect1_pointer[i]=0;
    }

    write_inode_sector(inode[0].inum);
    uint8_t *datablock=(uint8_t *)pa2kva(DATA_MEM_ADDR);
    kmemset(datablock,0,SECTOR_SIZE*0x1000);

    for(i=0;i<4;i++)
        sbi_sd_write(DATA_MEM_ADDR,0x1000,START_SECTOR+DATA_SD_OFFSET+i*0x1000);

    dentry_t *dentry=(dentry_t *)pa2kva(DATA_MEM_ADDR);
    kmemset(dentry,0,4096);
    dentry[0].type=TYPE_DIRECTORY;
    dentry[0].inum=0;
    kstrcpy(dentry[0].name,(char *)".");
    dentry[1].type=TYPE_DIRECTORY;
    dentry[1].inum=0;
    kstrcpy(dentry[1].name,(char *)"..");
    sbi_sd_write(DATA_MEM_ADDR,1,START_SECTOR+inode[0].direct_pointer[0]*8);

    printk("[FS] Initialize filesystem finished!\n\r");
    current_inode=inode;
}

void do_statfs(){
    sbi_sd_read(SUPERBLOCK_MEM_ADDR,1,START_SECTOR);
    superblock_t *superblock=(superblock_t *)pa2kva(SUPERBLOCK_MEM_ADDR);
    int i,j;
    uint32_t used_blocks=0;
    uint32_t used_inodes=0;
    sbi_sd_read(BMAP_MEM_ADDR,32,START_SECTOR+BMAP_SD_OFFSET);
    uint8_t *blockmap=(uint8_t *)(BMAP_MEM_ADDR+MEM_OFFSET);
    for(i=0;i<superblock->blockmap_num*0x200/8;i++){}
        for(j=0;j<8;j++)
            used_blocks +=(blockmap[i]>>j) & 1;
    sbi_sd_read(IMAP_MEM_ADDR,32,START_SECTOR+IMAP_SD_OFFSET);
    uint8_t *inodemap=(uint8_t *)(IMAP_MEM_ADDR+MEM_OFFSET);
    for(i=0;i<superblock->inodemap_num*0x200/8;i++)
        for(j=0;j<8;j++)
            used_inodes +=(inodemap[i]>>j) & 1;
    printk("magic_num : 0x%x\n\r",superblock->magic);
    printk("used block: %d/%d,start sector: %d(0x%x)\n\r",used_blocks,superblock->size,superblock->start_sector,superblock->start_sector);
    printk("block map offset: %d,occupied sector: %d,used: %d\n\r",superblock->blockmap_offset,superblock->blockmap_num,used_blocks);
    printk("inode map offset: %d,occupied sector: %d,used: %d\n\r",superblock->inodemap_offset,superblock->inodemap_num,used_inodes);
    printk("inode offset: %d,occupied sector: %d\n\r",superblock->inode_offset,superblock->inode_num);
    printk("data offset: %d,occupied sector: %d\n\r",superblock->data_block_offset,superblock->data_block_num);
    printk("inode entry size: 56B,dir entry size : 32B\n\r");
}


void do_mkdir(char *directory){

    inode_t *parent_inode=current_inode;
    inode_t *inode=directory_serach(parent_inode,directory);

    if(inode!=0 && inode->type==TYPE_DIRECTORY){
        prints("[Error] The directory has already existed!\n\r");
        return;
    }else if(inode!=0 && inode->type==TYPE_FILE){
        prints("[Error] The file of same name has already existed!\n\r");
        return;
    }

    uint32_t inum;
    if((inum=alloc_inode())==0){
        prints("[Error] No available inode\n\r");
        return;
    }
    inode=(inode_t *)pa2kva(INODE_MEM_ADDR+sizeof(inode_t)*inum);
    inode->inum=inum;
    inode->type=TYPE_DIRECTORY;
    inode->access=A_RW;
    inode->ref=0;
    inode->size=64;
    inode->modified_time=get_timer();
    inode->direct_pointer[0]=alloc_datablock();
    for(int i=1;i<DIRECT_NUM;i++){
        inode->direct_pointer[i]=0;
    }
    for(int i=0;i<2;i++){
      inode->indirect1_pointer[i]=0;
    }
    write_inode_sector(inode->inum);
    dentry_t *dentry=(dentry_t *)pa2kva(DATA_MEM_ADDR);
    kmemset(dentry,0,4096);
    dentry[0].type=TYPE_DIRECTORY;
    dentry[0].inum=inode->inum;
    kstrcpy(dentry[0].name,(char *)".");
    dentry[1].type=TYPE_DIRECTORY;
    dentry[1].inum=parent_inode->inum;
    kstrcpy(dentry[1].name,(char *)"..");
    sbi_sd_write(DATA_MEM_ADDR,1,START_SECTOR+inode[0].direct_pointer[0]*8);
    sbi_sd_read(DATA_MEM_ADDR,8,START_SECTOR+(parent_inode->direct_pointer[0])*8);

    dentry=(dentry_t *)pa2kva(DATA_MEM_ADDR+parent_inode->size);
    dentry->type=TYPE_DIRECTORY;
    dentry->inum=inode->inum;
    kstrcpy(dentry->name,directory);
    sbi_sd_write(DATA_MEM_ADDR,8,START_SECTOR+(parent_inode->direct_pointer[0])*8);
    parent_inode->ref++;
    parent_inode->size += DENTRY_SIZE;
    parent_inode->modified_time=get_timer();
    write_inode_sector(parent_inode->inum);
    prints("[Success] Create directory %s(inum:%d)\n\r",directory,inode->inum);
}

void do_rmdir(char *directory){

    inode_t *parent_inode=current_inode;
    inode_t *inode;
    inode=directory_serach(parent_inode,directory);
    if(inode==0 ||(inode!=0 && inode->type==TYPE_FILE)){
        prints("[Error] No such directory\n\r");
        return;
    }

    release(inode);

    sbi_sd_read(DATA_MEM_ADDR,1,START_SECTOR+(parent_inode->direct_pointer[0])*8);
    dentry_t *dentry=(dentry_t *)pa2kva(DATA_MEM_ADDR);
    int flag=0;
    for(int i=0;i<parent_inode->size;i += DENTRY_SIZE){
        if(flag!=0){
            kmemcpy((dentry-1),dentry,DENTRY_SIZE);
        }
        if(!kstrcmp(dentry->name,directory)){
            flag=1;
        }
        dentry++;
    }
    sbi_sd_write(DATA_MEM_ADDR,1,START_SECTOR+(parent_inode->direct_pointer[0])*8);
    parent_inode->ref--;
    parent_inode->size -= DENTRY_SIZE;
    parent_inode->modified_time=get_timer();
    write_inode_sector(parent_inode->inum);
    prints("[Success] Remove directory %s!\n\r",directory);
}

void do_cd(char *directory){

    inode_t *temp_inode=current_inode;
    char *path=directory;
    if(current_inode->type!=TYPE_DIRECTORY){
        prints("[Error] %s is not a directory!\n\r",directory);
        return;
    }else if(!find_path(path,TYPE_DIRECTORY) ){
        current_inode=temp_inode;
        prints("[Error] %s is not a directory!\n\r",directory);
        return;
    }
}

void do_ls(char *directory){
    inode_t *temp_inode=current_inode;
    char *path=directory;

    if(current_inode->type!=TYPE_DIRECTORY){
        prints("[Error] %s is not a directory!\n\r",directory);
        return;
    }else if(!find_path(path,TYPE_DIRECTORY) ){
        current_inode=temp_inode;
        prints("[Error] %s is not a directory!\n\r",directory);
        return;
    }

    sbi_sd_read(DATA_MEM_ADDR,1,START_SECTOR+(current_inode->direct_pointer[0])*8);
    dentry_t *dentry=(dentry_t *)pa2kva(DATA_MEM_ADDR);
    for(int i=0;i<current_inode->size;i += DENTRY_SIZE){
        prints("%s ",dentry->name);
        dentry++;
    }
  
    prints("\n\r");
    current_inode=temp_inode;
}

void do_ls_l(char *directory){
    inode_t *temp_inode=current_inode;
    char *path=directory;

    if(current_inode->type!=TYPE_DIRECTORY){
        prints("[Error] %s is not a directory!\n\r",directory);
        return;
    }else if(!find_path(path,TYPE_DIRECTORY) ){
        current_inode=temp_inode;
        prints("[Error] %s is not a directory!\n\r",directory);
        return;
    }

    sbi_sd_read(DATA_MEM_ADDR,1,START_SECTOR+(current_inode->direct_pointer[0])*8);
    dentry_t *dentry=(dentry_t *)pa2kva(DATA_MEM_ADDR);
    for(int i=0;i<current_inode->size;i += DENTRY_SIZE){
        prints("%s(",dentry->name);
        inode_t *inode=get_inode(dentry->inum);
        prints("inum:%d,ref: %d,size:%d)\n\r",inode->inum,inode->ref,inode->size);
        dentry++;
    }
    current_inode=temp_inode;
}

void do_touch(char *file_name)
{
    inode_t *parent_inode=current_inode;
    inode_t *inode=directory_serach(parent_inode,(char *)file_name);
    if(inode!=0 && inode->type==TYPE_FILE){
        prints("[Error] The file has existed!\n\r");
        return;
    }else if(inode!=0 && inode->type==TYPE_DIRECTORY){
        prints("[Error] The directory of same name has existed!\n\r");
        return;
    }

    uint32_t inum;
    if(!(inum=alloc_inode())){
        prints("[Error] No available inode\n\r");
        return;
    }

    inode=get_inode(inum);
    inode->inum=inum;
    inode->type=TYPE_FILE;
    inode->access=A_RW;
    inode->ref=0;
    inode->size=0;
    inode->modified_time=get_timer();
    inode->direct_pointer[0]=alloc_datablock();
    for(int i=1;i<DIRECT_NUM;i++)
        inode->direct_pointer[i]=0;
    for(int i=0;i<2;i++){
      inode->indirect1_pointer[i]=0;
    }
    write_inode_sector(inode->inum);
    sbi_sd_read(DATA_MEM_ADDR,1,START_SECTOR+(parent_inode->direct_pointer[0])*8);
    dentry_t *dentry=(dentry_t *)pa2kva(DATA_MEM_ADDR+parent_inode->size);
    dentry->type=TYPE_FILE;
    dentry->inum=inode->inum;
    kstrcpy(dentry->name,file_name);
    sbi_sd_write(DATA_MEM_ADDR,1,START_SECTOR+(parent_inode->direct_pointer[0])*8);
    parent_inode->ref++;
    parent_inode->size += DENTRY_SIZE;
    parent_inode->modified_time=get_timer();
    write_inode_sector(parent_inode->inum);
    prints("[Success] Create file %s(inum:%d)\n\r",file_name,inode->inum);
}


void do_rm(char *file_name)
{

    inode_t *parent_inode=current_inode;
    inode_t *inode;
    inode=directory_serach(parent_inode,file_name);
    if((inode==0)||(inode!=0 && inode->type==TYPE_DIRECTORY)){
        prints("[Error] No such file\n\r");
        return;
    }

    int block=inode->size/BLOCK_SIZE;
    uint32_t *indirect1_block=(uint32_t *)pa2kva(DATA_MEM_ADDR+BLOCK_SIZE);
    if(block<0x6){
        for(int i=0;i<=(inode->size/BLOCK_SIZE);i++)
            if(inode->direct_pointer[i]!=0)
                clear_blockmap(inode->direct_pointer[i]);
    }else if(block<0x406){
        for(int i=0;i<6;i++){
            if(inode->direct_pointer[i]!=0){
                clear_blockmap(inode->direct_pointer[i]);
            }
        }
        
        sbi_sd_read(DATA_MEM_ADDR+BLOCK_SIZE,8,START_SECTOR+(inode->indirect1_pointer[0])*8);
        for(int i=0;i<=block-0x6;i++){
            if(indirect1_block[i]!=0){
                clear_blockmap(indirect1_block[i]);
            }
        }   
    }else if(block<0x806){
        for(int i=0;i<6;i++)
            if(inode->direct_pointer[i]!=0)
                clear_blockmap(inode->direct_pointer[i]);
 
            sbi_sd_read(DATA_MEM_ADDR+BLOCK_SIZE,8,START_SECTOR+(inode->indirect1_pointer[0])*8);
            for(int i=0;i<0x400;i++)
                if(indirect1_block[i]!=0)
                    clear_blockmap(indirect1_block[i]);
            
            sbi_sd_read(DATA_MEM_ADDR+BLOCK_SIZE,8,START_SECTOR+(inode->indirect1_pointer[1])*8);
            for(int i=0;i<=block-0x406;i++)
                if(indirect1_block[i]!=0)
                    clear_blockmap(indirect1_block[i]);
    }
    clear_inodemap(inode->inum);

    sbi_sd_read(DATA_MEM_ADDR,1,START_SECTOR+(parent_inode->direct_pointer[0])*8);
    dentry_t *dentry=(dentry_t *)pa2kva(DATA_MEM_ADDR);
    int flag=0;
    for(int i=0;i<parent_inode->size;i += DENTRY_SIZE){
        if(flag)
            kmemcpy((dentry-1),dentry,DENTRY_SIZE);
        if(!kstrcmp(dentry->name,file_name))
            flag=1;
        dentry++;
    }
    sbi_sd_write(DATA_MEM_ADDR,1,START_SECTOR+(parent_inode->direct_pointer[0])*8);
    parent_inode->ref--;
    parent_inode->size -= DENTRY_SIZE;
    parent_inode->modified_time=get_timer();
    write_inode_sector(parent_inode->inum);
    prints("[Success] Remove file %s!\n\r",file_name);
}

void do_cat(char *file_name){

    inode_t *temp_inode=current_inode;
    char *path=file_name;
    if(!find_path(path,TYPE_FILE)){
        prints("[Error] File not found\n\r");
        current_inode=temp_inode;
        return;
    }

    uint32_t position=0;
    int i,j,k;
    char *content_buff=(char *)pa2kva(DATA_MEM_ADDR);
    for(i=0;i<current_inode->size;i += BLOCK_SIZE){
        kmemset(pa2kva(DATA_MEM_ADDR),0,SECTOR_SIZE*8);
        j=i/BLOCK_SIZE;
        sbi_sd_read(DATA_MEM_ADDR,8,START_SECTOR+(current_inode->direct_pointer[j])*8);
        for(k=0;position< current_inode->size && k<BLOCK_SIZE;position++,k++)
            prints("%c",content_buff[k]);
    }
    prints("\n\r");
    current_inode=temp_inode;
}

void do_ln(char *src_name,char *link_name){
    inode_t *temp_inode=current_inode;
    if(find_path(src_name,TYPE_FILE)==0){
        prints("[Error] No such file\n\r");
        current_inode=temp_inode;
        return;
    }
    current_inode->ref++;
    write_inode_sector(current_inode->inum);
    uint32_t inum=current_inode->inum;
    current_inode=temp_inode;
    inode_t *parent_inode=current_inode;
    inode_t *inode=directory_serach(parent_inode,link_name);
    if(inode!=0 && inode->type==TYPE_FILE){
        prints("[Error] The file has already existed!\n\r");
        return;
    }else if(inode!=0 && inode->type==TYPE_DIRECTORY){
        prints("[Error] The directory of same name has already existed!\n\r");
        return;
    }

    sbi_sd_read(DATA_MEM_ADDR,8,START_SECTOR+(parent_inode->direct_pointer[0])*8);
    dentry_t *dentry=(dentry_t *)pa2kva(DATA_MEM_ADDR+parent_inode->size);
    dentry->type=TYPE_FILE;
    dentry->inum=inum;
    kstrcpy(dentry->name,link_name);
    sbi_sd_write(DATA_MEM_ADDR,8,START_SECTOR+(parent_inode->direct_pointer[0])*8);
    parent_inode->ref++;
    parent_inode->size += DENTRY_SIZE;
    parent_inode->modified_time=get_timer();
    write_inode_sector(parent_inode->inum);
    prints("[Success] Create hard-link file %s(inum=%d)\n\r",link_name,inum);
}

long do_fopen(char *name,int access){
    inode_t *temp_inode=current_inode;

    if(!find_path(name,TYPE_FILE)){
        prints("Create file %s...\n\r",name);
        inode_t *parent_inode=current_inode;
        uint32_t inum;
        if(!(inum=alloc_inode())){
            prints("[Error] No available inode\n\r");
            return -1;
        }
        inode_t *inode=get_inode(inum);

        inode->inum=inum;
        inode->type=TYPE_FILE;
        inode->access=A_RW;
        inode->ref=0;
        inode->size=0;
        inode->modified_time=get_timer();
        inode->direct_pointer[0]=alloc_datablock();
        for(int i=1;i<DIRECT_NUM;i++)
            inode->direct_pointer[i]=0;
        for(int i=0;i<2;i++){
            inode->indirect1_pointer[i]=0;
        }
        write_inode_sector(inode->inum);
        sbi_sd_read(DATA_MEM_ADDR,1,START_SECTOR+(parent_inode->direct_pointer[0])*8);
        dentry_t *dentry=(dentry_t *)pa2kva(DATA_MEM_ADDR+parent_inode->size);
        dentry->type=TYPE_FILE;
        dentry->inum=inode->inum;
        kstrcpy(dentry->name,name);
        sbi_sd_write(DATA_MEM_ADDR,1,START_SECTOR+(parent_inode->direct_pointer[0])*8);
        parent_inode->ref++;
        parent_inode->size += DENTRY_SIZE;
        parent_inode->modified_time=get_timer();
        write_inode_sector(parent_inode->inum);
        current_inode=inode;
    }

    if(current_inode->access!=A_RW && current_inode->access!=access){
        prints("[Error] Cannot access %s !\n\r",name);
        return -1;
    }

    int i,fd;
    for(i=0;i<MAX_FILE_NUM;i++){
        if(fd_table[i].inum==0){
            fd=i;
            break;
        }
    }

    if(i==MAX_FILE_NUM){
        prints("[Error] No available fd\n\r");
        return -1;
    }

    fd_table[fd].inum=current_inode->inum;
    fd_table[fd].access=access;
    fd_table[fd].read_pointer=0;
    fd_table[fd].write_pointer=0;
    current_inode=temp_inode;
    return fd;
}

void do_fread(int fd,char *buff,int size){
    if(fd_table[fd].inum==0 || fd_table[fd].access==A_W){
        prints("[Error] Cannot read\n\r");
        return ;
    }
    inode_t *inode=get_inode(fd_table[fd].inum);
    uint32_t start_position=fd_table[fd].read_pointer;
    uint32_t start_block=start_position/BLOCK_SIZE;
    uint32_t end_block=(start_position+size)/BLOCK_SIZE;
    uint32_t cur_position=start_position;
    char *content_buff=(char *)pa2kva(DATA_MEM_ADDR);
    uint32_t *indirect1_block=(uint32_t *)pa2kva(DATA_MEM_ADDR+BLOCK_SIZE);
    for(int i=start_block;i<=end_block;i++){
        if((start_position+size-cur_position)>=BLOCK_SIZE){
            if(i<0x6){ 
                sbi_sd_read(DATA_MEM_ADDR,8,START_SECTOR+(inode->direct_pointer[i])*8);
                kmemcpy((uint8_t *)(buff+cur_position-start_position),(uint8_t *)(content_buff+cur_position % BLOCK_SIZE),BLOCK_SIZE-cur_position % BLOCK_SIZE);
                cur_position +=(BLOCK_SIZE-cur_position % BLOCK_SIZE);
            }else if(i<0x406){ 
                sbi_sd_read(DATA_MEM_ADDR+BLOCK_SIZE,8,START_SECTOR+(inode->indirect1_pointer[0])*8);
                int offset=i-0x6;
                sbi_sd_read(DATA_MEM_ADDR,8,START_SECTOR+indirect1_block[offset]*8);
                kmemcpy((uint8_t *)(buff+cur_position-start_position),(uint8_t *)(content_buff+cur_position % BLOCK_SIZE),BLOCK_SIZE-cur_position % BLOCK_SIZE);
                cur_position +=(BLOCK_SIZE-cur_position % BLOCK_SIZE);
            }else if(i<0x806){
                sbi_sd_read(DATA_MEM_ADDR+BLOCK_SIZE,8,START_SECTOR+(inode->indirect1_pointer[1])*8);
                int offset=i-0x406;
                sbi_sd_read(DATA_MEM_ADDR,8,START_SECTOR+indirect1_block[offset]*8);
                kmemcpy((uint8_t *)(buff+cur_position-start_position),(uint8_t *)(content_buff+cur_position % BLOCK_SIZE),BLOCK_SIZE-cur_position % BLOCK_SIZE);
                cur_position +=(BLOCK_SIZE-cur_position % BLOCK_SIZE);
            }
        }else{
            if(i<0x6){ 
                sbi_sd_read(DATA_MEM_ADDR,8,START_SECTOR+(inode->direct_pointer[i])*8);
                kmemcpy((uint8_t *)(buff+cur_position-start_position),(uint8_t *)(content_buff+cur_position % BLOCK_SIZE),start_position+size-cur_position);
                cur_position +=(start_position+size-cur_position);
            }else if(i<0x406){ 
                sbi_sd_read(DATA_MEM_ADDR+BLOCK_SIZE,8,START_SECTOR+(inode->indirect1_pointer[0])*8);
                int offset=i-0x6;
                sbi_sd_read(DATA_MEM_ADDR,8,START_SECTOR+indirect1_block[offset]*8);
                kmemcpy((uint8_t *)(buff+cur_position-start_position),(uint8_t *)(content_buff+cur_position % BLOCK_SIZE),start_position+size-cur_position);
                cur_position +=(start_position+size-cur_position);
            }else if(i<0x806){
                sbi_sd_read(DATA_MEM_ADDR+BLOCK_SIZE,8,START_SECTOR+(inode->indirect1_pointer[1])*8);
                int offset=i-0x406;
                sbi_sd_read(DATA_MEM_ADDR,8,START_SECTOR+indirect1_block[offset]*8);
                kmemcpy((uint8_t *)(buff+cur_position-start_position),(uint8_t *)(content_buff+cur_position % BLOCK_SIZE),start_position+size-cur_position);
                cur_position +=(start_position+size-cur_position);
            }
        }
    }
    fd_table[fd].read_pointer=cur_position;
    return;
}

void do_fwrite(int fd,char *buff,int size){
    if(fd_table[fd].inum==0 || fd_table[fd].access==A_R){
        prints("[Error] Cannot write\n\r");
        return ;
    }
    inode_t *inode=get_inode(fd_table[fd].inum);
    uint32_t start_position=fd_table[fd].write_pointer;
    uint32_t start_block=start_position/BLOCK_SIZE;
    uint32_t end_block=(start_position+size)/BLOCK_SIZE;

    uint32_t cur_position=start_position;
    char *content_buff=(char *)pa2kva(DATA_MEM_ADDR);
    uint32_t *indirect1_block=(uint32_t *)pa2kva(DATA_MEM_ADDR+BLOCK_SIZE);

    int i;
    for(i=start_block;i<=end_block;i++){
        if((size+start_position-cur_position)>=BLOCK_SIZE){
            if(i<0x6){
                sbi_sd_read(DATA_MEM_ADDR,8,START_SECTOR+(inode->direct_pointer[i])*8);
                kmemcpy((uint8_t *)(content_buff+cur_position % BLOCK_SIZE),(uint8_t *)(buff+cur_position-start_position),BLOCK_SIZE-cur_position % BLOCK_SIZE);
                cur_position +=(BLOCK_SIZE-cur_position % BLOCK_SIZE);
                sbi_sd_write(DATA_MEM_ADDR,8,START_SECTOR+(inode->direct_pointer[i])*8);
            }else if(i<0x406){
                sbi_sd_read(DATA_MEM_ADDR+BLOCK_SIZE,8,START_SECTOR+(inode->indirect1_pointer[0])*8);
                int offset=i-0x6;
                sbi_sd_read(DATA_MEM_ADDR,8,START_SECTOR+indirect1_block[offset]*8);
                kmemcpy((uint8_t *)(content_buff+cur_position % BLOCK_SIZE),(uint8_t *)(buff+cur_position-start_position),BLOCK_SIZE-cur_position % BLOCK_SIZE);
                cur_position +=(BLOCK_SIZE-cur_position % BLOCK_SIZE);
                sbi_sd_write(DATA_MEM_ADDR,8,START_SECTOR+indirect1_block[offset]*8);
            }else if(i<0x806){
                sbi_sd_read(DATA_MEM_ADDR+BLOCK_SIZE,8,START_SECTOR+(inode->indirect1_pointer[1])*8);
                int offset=i-0x406;
                sbi_sd_read(DATA_MEM_ADDR,8,START_SECTOR+indirect1_block[offset]*8);
                kmemcpy((uint8_t *)(content_buff+cur_position % BLOCK_SIZE),(uint8_t *)(buff+cur_position-start_position),BLOCK_SIZE-cur_position % BLOCK_SIZE);
                cur_position +=(BLOCK_SIZE-cur_position % BLOCK_SIZE);
                sbi_sd_write(DATA_MEM_ADDR,8,START_SECTOR+indirect1_block[offset]*8);
            }
        }else{
            if(i<0x6){ 
                sbi_sd_read(DATA_MEM_ADDR,8,START_SECTOR+(inode->direct_pointer[i])*8);
                kmemcpy((uint8_t *)(content_buff+cur_position % BLOCK_SIZE),(uint8_t *)(buff+cur_position-start_position),start_position+size-cur_position);
                cur_position +=(start_position+size-cur_position);
                sbi_sd_write(DATA_MEM_ADDR,8,START_SECTOR+(inode->direct_pointer[i])*8);
            }else if(i<0x406){ 
                sbi_sd_read(DATA_MEM_ADDR+BLOCK_SIZE,8,START_SECTOR+(inode->indirect1_pointer[0])*8);
                int offset=i-0x6;
                sbi_sd_read(DATA_MEM_ADDR,8,START_SECTOR+indirect1_block[offset]*8);
                kmemcpy((uint8_t *)(content_buff+cur_position % BLOCK_SIZE),(uint8_t *)(buff+cur_position-start_position),start_position+size-cur_position);
                cur_position +=(start_position+size-cur_position);
                sbi_sd_write(DATA_MEM_ADDR,8,START_SECTOR+indirect1_block[offset]*8);
            }else if(i<0x806){ 
                sbi_sd_read(DATA_MEM_ADDR+BLOCK_SIZE,8,START_SECTOR+(inode->indirect1_pointer[1])*8);
                int offset=i-0x406;
                sbi_sd_read(DATA_MEM_ADDR,8,START_SECTOR+indirect1_block[offset]*8);
                kmemcpy((uint8_t *)(content_buff+cur_position % BLOCK_SIZE),(uint8_t *)(buff+cur_position-start_position),start_position+size-cur_position);
                cur_position +=(start_position+size-cur_position);
                sbi_sd_write(DATA_MEM_ADDR,8,START_SECTOR+indirect1_block[offset]*8);
            }
        }
    }
    inode->modified_time=get_timer();
    write_inode_sector(inode->inum);
    fd_table[fd].write_pointer=cur_position;
    return ;
}

void do_lseek(int fd,int offset,int whence){
    switch (whence){
    case 0:
        fd_table[fd].read_pointer=offset;
        fd_table[fd].write_pointer=offset;
        break;
    case 1:
        fd_table[fd].read_pointer += offset;
        fd_table[fd].write_pointer += offset;
        break;
    case 2:
        fd_table[fd].read_pointer=get_inode(fd_table[fd].inum)->size+offset;
        fd_table[fd].write_pointer=get_inode(fd_table[fd].inum)->size+offset;
        break;
    
    default:
        break;
    }

    return ;
}

void do_fclose(int fd){
    uint32_t inum=fd_table[fd].inum;
    inode_t *inode=get_inode(inum);
    inode->modified_time=get_timer();
    write_inode_sector(inum);
    fd_table[fd].inum=0;
    fd_table[fd].access=0;
}


void set_blockmap(uint32_t block_num){
    uint8_t *blockmap=(uint8_t *)pa2kva(BMAP_MEM_ADDR);
    sbi_sd_read(BMAP_MEM_ADDR,32,START_SECTOR+BMAP_SD_OFFSET);
    blockmap[block_num/8]=blockmap[block_num/8]|(1<<(7-(block_num%8)));
    sbi_sd_write(BMAP_MEM_ADDR,32,START_SECTOR+BMAP_SD_OFFSET);
}

void set_inodemap(uint32_t inode_num){
    uint8_t *inodemap=(uint8_t *)pa2kva(IMAP_MEM_ADDR);
    sbi_sd_read(IMAP_MEM_ADDR,1,START_SECTOR+IMAP_SD_OFFSET);
    inodemap[inode_num/8]=inodemap[inode_num/8]|(1<<(7-(inode_num%8)));
    sbi_sd_write(IMAP_MEM_ADDR,1,START_SECTOR+IMAP_SD_OFFSET);
}

void clear_blockmap(uint32_t block_num){
    uint8_t *blockmap=(uint8_t *)pa2kva(BMAP_MEM_ADDR);
    sbi_sd_read(BMAP_MEM_ADDR,32,START_SECTOR+BMAP_SD_OFFSET);
    blockmap[block_num/8]=blockmap[block_num/8] & (0xff-(1 <<(7 -(block_num % 8))));
    sbi_sd_write(BMAP_MEM_ADDR,32,START_SECTOR+BMAP_SD_OFFSET);
}

void clear_inodemap(uint32_t inode_num){
    uint8_t *inodemap=(uint8_t *)pa2kva(IMAP_MEM_ADDR);
    sbi_sd_read(IMAP_MEM_ADDR,1,START_SECTOR+IMAP_SD_OFFSET);
    inodemap[inode_num/8]=inodemap[inode_num/8] & (0xff-(1 <<(7 -(inode_num % 8))));
    sbi_sd_write(IMAP_MEM_ADDR,1,START_SECTOR+IMAP_SD_OFFSET);
}

void release(inode_t *inode){
    for(int i=0;i <=(inode->size/BLOCK_SIZE);i++)
        clear_blockmap(inode->direct_pointer[i]);
    clear_inodemap(inode->inum);
    if(inode->type==TYPE_DIRECTORY && inode->size > 64){
        uint8_t tmp[0x1000];
        dentry_t *dentry=(dentry_t *)tmp;
        sbi_sd_read(kva2pa(dentry),8,START_SECTOR+(inode->direct_pointer[0])*8);
        dentry += 2;
        for(int i=0;i<inode->size-64;i += DENTRY_SIZE)
        {
            release(get_inode(dentry->inum));
            dentry++;
        }
    }
}

uint32_t alloc_datablock(){
    sbi_sd_read(BMAP_MEM_ADDR,32,START_SECTOR+BMAP_SD_OFFSET);
    uint8_t *blockmap=(uint8_t *)pa2kva(BMAP_MEM_ADDR);
    uint32_t free_block=0;
    for(int i=0;i<32*512/8;i++)
        for(int j=0;j<8;j++)
            if(!(blockmap[i] &(0x80>>j))){
                free_block=8*i+j;
                set_blockmap(free_block);
                return free_block;
            }
    return -1;
}

uint32_t alloc_inode(){
    sbi_sd_read(IMAP_MEM_ADDR,1,START_SECTOR+IMAP_SD_OFFSET);
    uint8_t *inodemap=(uint8_t *)pa2kva(IMAP_MEM_ADDR);
    int i,j;
    uint32_t free_inode=0;
    for(i=0;i<512/8;i++){
        for(j=0;j<8;j++)
            if(!(inodemap[i] &(0x80>>j))){
                free_inode=8*i+j;
                set_inodemap(free_inode);
                return free_inode;
            }
    }
    return free_inode;
}

void write_inode_sector(uint32_t inum){
    uint32_t sector_offset=inum*INODE_OFFSET_IN_SECTOR;
    sbi_sd_write(INODE_MEM_ADDR+sector_offset*512,1,START_SECTOR+INODE_SD_OFFSET+sector_offset);
}

inode_t *get_inode(uint32_t inum){
    inode_t *inode=(inode_t *)pa2kva(INODE_MEM_ADDR+sizeof(inode_t)*inum);
    uint32_t sector_offset=inum*INODE_OFFSET_IN_SECTOR;
    sbi_sd_read(pa2kva(INODE_MEM_ADDR+512*sector_offset),1,START_SECTOR+INODE_SD_OFFSET+sector_offset);
    return inode;
}

inode_t *directory_serach(inode_t *inode,char *name){
    dentry_t *dentry=(dentry_t *)pa2kva(DATA_MEM_ADDR);
    if(inode->type!=TYPE_DIRECTORY)
        return 0;
    sbi_sd_read(DATA_MEM_ADDR,8,START_SECTOR+(inode->direct_pointer[0])*8);
    for(int i=0;i<inode->size;i += DENTRY_SIZE){
        if(!strcmp(name,dentry->name))
            return get_inode(dentry->inum);
        dentry++;
    }
    return 0;
}

static char cur_directory[MAX_FILE_NAME];
static char next_directory[MAX_FILE_NAME];
int find_path(char *path,int type){
    inode_t *temp_inode=current_inode;
    int i,j;
    for(i=0,j=0;i<strlen(path) && path[i]!='/';i++,j++)
        cur_directory[j]=path[i];
    cur_directory[j]='\0';
    i++;
    for(j=0;i<strlen(path);i++,j++)
        next_directory[j]=path[i];
    next_directory[j]='\0';
    if(cur_directory[0]=='\0')
        return 0;
    temp_inode=directory_serach(current_inode,cur_directory);
    if(temp_inode!=0){
        if(temp_inode->type!=type){
            prints("[Error] Type Confliction\n\r");
            return;
        }
        current_inode=temp_inode;
        if(next_directory[0]=='\0'){
            return 1;
        }
        return find_path(next_directory,type);
    }else{
        return 0;
    }
}
