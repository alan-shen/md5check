#include <ctype.h>     
#include <errno.h>      
#include <fcntl.h>      
#include <limits.h>     
#include <linux/input.h>     
#include <stdio.h>            
#include <stdlib.h>     
#include <string.h>      
#include <sys/stat.h>     
#include <sys/statfs.h>     
#include <sys/types.h>       
#include <time.h>           
#include <unistd.h>     
#include <dirent.h>
#include "common.h"     
#include "cutils/properties.h"     
#include "cutils/android_reboot.h"     
#include "install.h"                    
#include "minui/minui.h"     
#include "minzip/DirUtil.h"     
#include "minzip/SysUtil.h"      
#include "roots.h"               
#include "ui.h"         
#include "screen_ui.h"     
#include "device.h"
#include "minzip/Zip.h"     
#include "root_check.h"      
extern "C" {                 
    #include "md5.h"       
}

#define MD5SUM_OF_SYSTEM "/system/md5_system"

int md5_check(){

}
