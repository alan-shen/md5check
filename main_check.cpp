#include <errno.h>      
#include <stdio.h>            
#include <stdlib.h>     
#include <string.h>
#include <getopt.h> 
#include "check.h"      

int main(int argc, char** argv){
    char *part   = NULL;
    int opt;
    int option_index = 0;
    const char *optstring = ":hc:";
    static struct option long_options[] = { 
        {  "check", required_argument, NULL, 'c'},
        {   "help", no_argument,       NULL, 'h'},
        {0, 0, 0, 0}
    };  

    while( (opt = getopt_long(argc, argv, optstring, long_options, &option_index)) != -1 ){
        switch( opt ){
            case 'c':
                part   = (char*)malloc(512);
                strcpy(  part, optarg);
                printf("Check partition [%s]\n", part);
                break;
            case 'h':
                exit(0);
                break;
            case '?':
            default:
                printf("\nERROR: Unknown options!\n");
                exit(1);
                break;
        }   
    }

	//int ret = CHECK_PASS;
	//char *part = "/system";
	if(part == NULL){
		part = (char*)malloc(512);
		strcpy(part, "/system");
	}

	bool ret = main_check(part);
	printf("\n\n>>>>> RESULT <<<<< %s!!\n", ret==true?"Success":"Fail");
	
	if(part != NULL){
		free(part);
	}
#if 0
	switch(ret){
    	case(CHECK_PASS):					printf("[success]\n");							break;
		case(CHECK_FAIL):					printf("[error] CHECK_FAIL\n");					break;
		case(CHECK_NO_KEY):					printf("[error] CHECK_NO_KEY\n");				break;
		case(CHECK_OPEN_FILE_ERR):			printf("[error] CHECK_OPEN_FILE_ERR\n");		break;
		case(CHECK_MOUNT_ERR):				printf("[error] CHECK_MOUNT_ERR\n");			break;
		case(CHECK_SYSTEM_FILE_NUM_ERR):	printf("[error] CHECK_SYSTEM_FILE_NUM_ERR\n");	break;
		case(CHECK_FILE_NOT_MATCH):			printf("[error] CHECK_FILE_NOT_MATCH\n");		break;
		case(CHECK_LOST_FILE):				printf("[error] CHECK_LOST_FILE\n");			break;
		case(CHECK_ADD_NEW_FILE):			printf("[error] CHECK_ADD_NEW_FILE\n");			break;
		case(CHECK_IMAGE_ERR):				printf("[error] CHECK_IMAGE_ERR\n");			break;
		default:							printf("[error] unknow error\n");				break;
	}
#endif
	printf("\n");
}
