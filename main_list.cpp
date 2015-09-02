#include <errno.h>      
#include <stdio.h>            
#include <stdlib.h>     
#include <string.h>      
#include <getopt.h>
#include "check.h"      

const char *gCmdName = "mi_md5list";

void help(void){
	printf("%s:\n\n", gCmdName);
	printf("%s -p system/bin/adb\n", gCmdName);
	printf("%s -h\n", gCmdName);
}

int main(int argc, char **argv){
	int ret = 0;

	char *fliter = NULL;
	char *part   = NULL;
	int type;
    int opt;
    int option_index = 0;
    const char *optstring = ":hpc:";
    static struct option long_options[] = {
        {  "check", required_argument, NULL, 'c'},
        {   "path", required_argument, NULL, 'p'},
        {   "help", no_argument,       NULL, 'h'},
        {0, 0, 0, 0}
    };

	while( (opt = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1 ){
        switch( opt ){
            case 'p':
				fliter = (char*)malloc(512);
                strcpy(fliter, optarg);
                printf("Fliter is [%s]\n", fliter);
                break;
            case 'c':
				part   = (char*)malloc(512);
                strcpy(  part, optarg);
                printf("Check partition [%s]\n", part);
                break;
            case 'h':
                help();
                exit(0);
                break;
            case '?':
            default:
                printf("\nERROR: Unknown options!\n");
				help();
                exit(1);
                break;
        }
    }

	if(part==NULL){
		part   = (char*)malloc(512);
		strcpy(part, "/system");
	}

	printf("List:\n");
	if(fliter == NULL)
		main_list(NULL, part);
	else{
		main_list(fliter, part);
		free(fliter);
	}

	free(part);

	return ret;
}
