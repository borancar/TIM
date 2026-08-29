#include <stdio.h>
#include <string.h>
#include "dgroup.h"
#include "io.h"
#include "tim.h"
int main(void) {
    io_reset();
    if (!io_load_program("out/TIM.img", "out/TIM.unpacked.exe")) { puts("load failed"); return 1; }
    strcpy((char *)(dgroup + 0x7000), "RESOURCE.MAP");
    strcpy((char *)(dgroup + 0x7020), "rb");
    printf("io_dos_open('RESOURCE.MAP') = %d\n", io_dos_open("RESOURCE.MAP"));
    printf("stdio_fopen('RESOURCE.MAP') = %#x\n", stdio_fopen(0x7000, 0x7020));
    return 0;
}
