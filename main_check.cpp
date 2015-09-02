#include <errno.h>      
#include <stdio.h>            
#include <stdlib.h>     
#include <string.h>      
#include "check.h"      

int main(){
	//int ret = CHECK_PASS;
	char *part = "/system";
	bool ret = main_check(part);
	printf("\n\n>>>>> RESULT <<<<< %s!!\n", ret==true?"Success":"Fail");
	
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
