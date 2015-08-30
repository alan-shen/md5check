#include "root_check.h"
extern "C" {
#include "cr32.h"
}

void main(){
	int check_result = 0;

	printf("Start Check:");
	check_result = md5_check();
	if(check_result == 0){
		printf("Check Ok!\n");
	}
	else{
		printf("Check Fail!\n");
	}
}
