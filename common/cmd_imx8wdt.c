#include <common.h>
#include <command.h>
#include "../board/digi/ccimx8x/ccimx8x.h"

#ifdef __U_BOOT__
static int do_wdt(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
    printf("hello imx8 watchdog triger:%s\n", argv[1]);
    if(strcmp (argv[1], "start") == 0)
    {
        mca_wd_start();
    }
    else if(strcmp (argv[1], "stop") == 0)
    {
        mca_wd_stop();
    }
    else
    {
        printf("wrong input:%s\n", argv[1]);
    }
    
}

U_BOOT_CMD(wdt, 2, 1, do_wdt, "wdt start/stop\n", "arg1: start/stop\n");
#endif