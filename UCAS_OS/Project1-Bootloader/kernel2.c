#include <sbi.h>

#define VERSION_BUF 50

int version = 2; // version must between 0 and 9
char buf[VERSION_BUF];

int bss_check()
{
    for (int i = 0; i < VERSION_BUF; ++i) {
        if (buf[i] != 0) return 0;
    }
    return 1;
}

int getch()
{
	//Waits for a keyboard input and then returns.
    int c;
    while((c=sbi_console_getchar())==-1){
        ;
    }
    return c;
}

int main(void)
{
    int check = bss_check();
    char output_str[] = "bss check: _ version: _\n\r";
    char output_val[2] = {0};
    int i, output_val_pos = 0;

    output_val[0] = check ? 't' : 'f';
    output_val[1] = version + '0';
    for (i = 0; i < sizeof(output_str); ++i) {
        buf[i] = output_str[i];
	if (buf[i] == '_') {
	    buf[i] = output_val[output_val_pos++];
	}
    }

	//print "Hello OS!"
    char str1[] = "Hello OS!\n";
    sbi_console_putstr(str1);
	//print array buf which is expected to be "Version: 1"
  
    sbi_console_putstr(buf);
    //fill function getch, and call getch here to receive keyboard input.
	//print it out at the same time 
    char ver2[]="This is Version 2, you couldn't read or write\n" ;
    sbi_console_putstr(ver2);
    return 0;
}
