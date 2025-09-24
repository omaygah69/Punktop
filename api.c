#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void ReadCpuModel(){
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) {
        perror("fopen");
        return;
    }
    char line[1024];
    while(fgets(line, sizeof(line), fp))
    {
        if (strncmp(line, "model name", 10) == 0) {
            char *colon = strchr(line, ':');
            if (colon) {
                printf("CPU: %s", colon + 2); // skip ": "
            }
            break; 
        }
    }
    fclose(fp);
}

int main()
{
    ReadCpuModel();
    return 0;
}
