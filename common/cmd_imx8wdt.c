#include <common.h>
#include <command.h>


int do_wdt(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    printf("hello world\n");
    for(int i; i < argc; i++)
    {
        printf("%s \n", argv[i]);
    }
    printf("\n");
}

U_BOOT_CMD(wdt, 2, 1, do_wdt, "wdt open/close", "arg1: open/close");