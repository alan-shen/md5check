#include <errno.h>      
#include <stdio.h>            
#include <stdlib.h>     
#include <string.h>      
#include "check.h"      

int main(){
	int ret = 0;

	printf("List:\n");
	ret = list();

	return ret;
}
