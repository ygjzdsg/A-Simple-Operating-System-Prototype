#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_FILE "./image"
#define ARGS "[--extended] [--vm] <bootblock> <executable-file> ..."


/* structure to store command line options */
static struct {
    int vm;
    int extended;
} options;

/* prototypes of local functions */
static void create_image(int nfiles, char *files[]);
static void error(char *fmt, ...);
static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp);
static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr);
static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes, int *first, int *kernel_nbytes);
static void write_os_size(int kernel_nbytes, FILE * img, int kernel_choice);

int main(int argc, char **argv)
{
    char *progname = argv[0];

    /* process command line options */
    options.vm = 0;
    options.extended = 0;
    while ((argc > 1) && (argv[1][0] == '-') && (argv[1][1] == '-')) {
        char *option = &argv[1][2];

        if (strcmp(option, "vm") == 0) {
            options.vm = 1;
        } else if (strcmp(option, "extended") == 0) {
            options.extended = 1;
        } else {
            error("%s: invalid option\nusage: %s %s\n", progname,
                  progname, ARGS);
        }
        argc--;
        argv++;
    }
    if (options.vm == 1) {
        error("%s: option --vm not implemented\n", progname);
    }
    if (argc < 3) {
        /* at least 3 args (createimage bootblock kernel) */
        error("usage: %s %s\n", progname, ARGS);
    }
    create_image(argc - 1, argv + 1);
    return 0;
}

static void create_image(int nfiles, char *files[])
{
    int ph, nbytes = 0, first = 1;
    int kernel_nbytes=0;
    FILE *fp, *img;
    Elf64_Ehdr ehdr;
    Elf64_Phdr phdr;
    int kernel_choice=0;

    /* open the image file */
    img=fopen(IMAGE_FILE,"w+");
    /* for each input file */
    while (nfiles-- > 0) {

        kernel_nbytes=0;

        /* open input file */
        fp=fopen(*files,"r");
        /* read ELF header */
        read_ehdr(&ehdr, fp);
        printf("0x%04lx: %s\n", ehdr.e_entry, *files);

        /* for each program header */
        for (ph = 0; ph < ehdr.e_phnum; ph++) {

            /* read program header */
            read_phdr(&phdr, fp, ph, ehdr);

            /* write segment to the image */
            write_segment(ehdr, phdr, fp, img, &nbytes, &first,&kernel_nbytes);
            if(options.extended){
                printf("\tsegment %d\n",ph);
                printf("\t\toffset 0x%lx\tvaddr 0x%lx\n",phdr.p_offset,phdr.p_vaddr);
                printf("\t\tfilez 0x%lx\tmemz 0x%lx\n",phdr.p_filesz,phdr.p_memsz);
                if(phdr.p_memsz>0){
                    printf("\t\twriting 0x%lx bytes\n",phdr.p_memsz);
                    printf("\t\tpadding up to 0x%x\n", nbytes);
                }
            }
        }
        //
        fclose(fp);
        files++; 
        if(kernel_choice==0){
            ;
        }else{
            write_os_size(kernel_nbytes, img, kernel_choice);
        }    
       
        kernel_choice++;
        
    }
   
    fclose(img);
}

static void read_ehdr(Elf64_Ehdr * ehdr, FILE * fp)
{
    fread(ehdr, sizeof(Elf64_Ehdr),1,fp);
    return;
    
}

static void read_phdr(Elf64_Phdr * phdr, FILE * fp, int ph,
                      Elf64_Ehdr ehdr)
{
    long offset;
    offset = ehdr.e_phoff + ehdr.e_phentsize*ph;
    fseek(fp,offset,SEEK_SET);
    fread(phdr, sizeof(Elf64_Phdr),1,fp);
    return;
}

static void write_segment(Elf64_Ehdr ehdr, Elf64_Phdr phdr, FILE * fp,
                          FILE * img, int *nbytes, int *first,int *kernel_nbytes)
{   
    
    int memz_sector,filez_sector;
    
    memz_sector = (phdr.p_memsz+511)/512;
    filez_sector = (phdr.p_filesz+511)/512;
    
    char temp[512];

    fseek(fp,phdr.p_offset,SEEK_SET ); 
    fseek(img,*nbytes,SEEK_SET);
    while(memz_sector>0){ 
        memset(temp,0,sizeof(temp));
        if(filez_sector>0){
            fread(temp,1,512,fp);
        }
        fwrite(temp,1,512,img);
        memz_sector--;
        filez_sector--;
    }
    memz_sector = (phdr.p_memsz+511)/512;
    filez_sector = (phdr.p_filesz+511)/512;
    (*nbytes) += memz_sector*512;

    if(*first==1){
        *first=0;
    }else{
        *kernel_nbytes += memz_sector *512;
    }    

    return;
}

static void write_os_size(int kernel_nbytes, FILE * img, int kernel_choice)
{
    long os_addr;
    if(kernel_choice==1){
        os_addr = 0x1fc;
    }else if(kernel_choice==2){
        os_addr = 0x1fa;
    }
    
    int kernel_sector=kernel_nbytes/512;
    fseek(img,os_addr,SEEK_SET);
    fwrite(&kernel_sector,2,1,img);
    if(options.extended){
        printf("os%d_size: %d sectors\n",kernel_choice,kernel_sector);
    }
    return;
}

/* print an error message and exit */
static void error(char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    if (errno != 0) {
        perror(NULL);
    }
    exit(EXIT_FAILURE);
}
