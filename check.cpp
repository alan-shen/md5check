#define LOG_TAG "UpdateCheck"

/********************************************************************************************
 * Header Files Normal
 ********************************************************************************************/
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

#include "cutils/properties.h"
#include "cutils/android_reboot.h"
#include "minzip/DirUtil.h"
#include "minzip/SysUtil.h"
#include "minzip/Zip.h"

/********************************************************************************************
 * Header Files For Debug Log
 ********************************************************************************************/
#ifdef USE_LOGCAT_LOG
#include "log/log.h"
#else
	#undef  SLOGI
	#define SLOGI printf
	#undef  SLOGE
	#define SLOGE printf
#endif

/********************************************************************************************
 * Header Files Local
 ********************************************************************************************/
#include "check.h"
extern "C" {
	#include "cr32.h"
	#include "md5.h"
}

/********************************************************************************************
 * MACRO
 ********************************************************************************************/
#undef  FILENAME_MAX
#define FILENAME_MAX 200

/*
 * ---------
 * GENERATE:
 * ---------
 *           crc32
 *           +md5sum
 *           +encrypt               crc32                    zip
 * /system/ ----------> file_count -------> doub_check --(+)-----> /system/system_checksum
 *                          |                             ^
 *                          |                             |
 *                          \_____________________________/
 *
 * ------
 * PARSE:
 * ------
 *
 *                          unzip                    uncrypt
 * /system/system_checksum -------> /tmp/file_count ---------> /tmp/system_dencrypt
 *                                  /tmp/doub_check
 */
static const char   *FILE_COUNT_ZIP="file_count";
static const char *DOUBLE_DYW_CHECK="doub_check";
static const char *TEMP_FILE_IN_RAM="/tmp/system_dencrypt";
static const char   *FILE_COUNT_TMP="/tmp/file_count";
static const char     *DYW_DOUB_TMP="/tmp/doub_check";
static const char     *FILE_NEW_TMP="/tmp/list_new_file";

/********************************************************************************************
 * Structure Define
 ********************************************************************************************/
struct last_check_file{
	int          n_newfile;           // num of new file, and record name to FILE_NEW_TMP
	int          n_lostfile;          // num of lost file, name can find in check_map[]
	int          n_modifyfile;        // num of modify file, name can find in check_modify[]
	int          n_rootfile;          // num of root apk, idx is in root_to_check, name can
                                      // find in file_to_check[]
	int          file_number_to_check;// num of files have been checked
	int          expect_file_number;  // num of files record in "system_checksum"
	unsigned int file_count_check;    // crc32 code of 'file_count' record in 'system_checksum'
	unsigned int crc_count_check;     // not use now
};
static struct last_check_file *check_file_result;

/********************************************************************************************
 * Global Variable
 ********************************************************************************************/
static const char* file_to_check[]={
	"supersu.apk",
	"superroot.apk",
	"superuser.apk",
	"busybox.apk"
};

static const char* file_to_pass[]={
	"recovery-from-boot.p",
	"install-recovery.sh",
	"system_checksum",
	//"build.prop",
	//"S_ANDRO_SFL.ini",
	"recovery.sig",
};

unsigned int key=15;                               // key for encrypt the checksum
bool         checkResult = true;
int          root_to_check[MAX_ROOT_TO_CHECK]={0}; // record which root apk had been finded
int check_map[MAX_FILES_IN_SYSTEM/INT_BY_BIT+1];   // bit maps for list of checked files
int check_modify[MAX_FILES_IN_SYSTEM/INT_BY_BIT+1];// bit maps for record which file is modified

/********************************************************************************************
 * Interface For mark bit map
 ********************************************************************************************/
static void set_bit(int x){
	check_map[x>>SHIFT]|= 1<<(x&MASK);
}
static void clear_bit(int x){
	check_map[x>>SHIFT]&= ~(1<<(x&MASK));
}
static int test_bit(int x){
	return check_map[x>>SHIFT]&(1<<(x&MASK));
}

static void set_bit_m(int x){
	check_modify[x>>SHIFT]|= 1<<(x&MASK);
}
static void clear_bit_m(int x){
	check_modify[x>>SHIFT]&= ~(1<<(x&MASK));
}
static int test_bit_m(int x){
	return check_modify[x>>SHIFT]&(1<<(x&MASK));
}

/*
 *
 */
static int clear_selinux_file(char* path)
{
	int found=0;
	int ret=0;
	FILE *fp_info;
	FILE *fp_new;
	char buf[512];
	char p_name[256];
	unsigned char p_md[MD5_LENGTH*2];
	char *p_cmp_name;
	unsigned int p_size;
	int p_number;
	struct stat statbuf;

	fp_info = fopen(TEMP_FILE_IN_RAM, "r");
	if(fp_info){
		if(fgets(buf, sizeof(buf), fp_info) != NULL){
			while(fgets(buf, sizeof(buf), fp_info)){
				if (sscanf(buf, "%d    %s    %u    %s", &p_number,p_name,&p_size,p_md) == 4){
					//TODO:path[0] will be '\0' sometimes, and it should be '/'
					path[0] = '/';
					if(strstr(p_name,path)!=NULL){
						p_cmp_name=strstr(p_name,path);
						if(strcmp(p_cmp_name,path)==0){
							found=1;
							clear_bit(p_number);
						}
					}
				}
			}

			if(found==0){
				SLOGI("-found a new file,filename is %s\n",path);
				check_file_result->n_newfile+=1;

				if(access(FILE_NEW_TMP,0)==-1){
					int fd_new=creat(FILE_NEW_TMP,0755);
				}

				fp_new=fopen(FILE_NEW_TMP, "a");
				if(fp_new){
					fprintf(fp_new,"%s\n",path);
				}
				else{
					SLOGI("open %s error,error reason is %s\n",FILE_NEW_TMP,strerror(errno));
					return CHECK_ADD_NEW_FILE;
				}

				checkResult=false;
				fclose(fp_info);
				fclose(fp_new);
				return CHECK_ADD_NEW_FILE;
			}
		}
	}
	else{
		SLOGI("open %s error,error reason is %s\n",TEMP_FILE_IN_RAM,strerror(errno));
		return CHECK_NO_KEY;
	}//end of "if(fp_info)"

	fclose(fp_info);
	return CHECK_PASS;
}

static int clear_selinux_dir(char const* path)
{
	int ret=0;
	FILE *fp_info;
	FILE *fp_new;
	char buf[512];
	char p_name[256];
	unsigned char p_md[MD5_LENGTH*2];
	char *p_cmp_name;
	unsigned int p_size;
	int p_number;
	struct stat statbuf;

	fp_info = fopen(TEMP_FILE_IN_RAM, "r");
	if(fp_info){
		if(fgets(buf, sizeof(buf), fp_info) != NULL){
			while(fgets(buf, sizeof(buf), fp_info)){
				if (sscanf(buf, "%d    %s    %u    %s", &p_number,p_name,&p_size,p_md) == 4){
					//TODO:path[0] will be '\0' sometimes, and it should be '/'
					//path[0] = '/';
					if(strstr(p_name,path)!=NULL) {
						p_cmp_name=strstr(p_name,path);
						SLOGI("%s file is selinux protected,just pass\n",p_cmp_name);
						clear_bit(p_number);
					}
					else {
						//SLOGI("not found %s in orignal file,please check!",path);
						continue;
					}
				}
			}
		}
	}
	else{
		SLOGI("open %s error,error reason is %s\n",TEMP_FILE_IN_RAM,strerror(errno));
		return CHECK_NO_KEY;
	}

	fclose(fp_info);
	return CHECK_PASS;
}

static int crc_compare_really(char* path_local, int nCS, char* nMd5)
{
	int found=0;
	int ret=0;
	FILE *fp_info;
	FILE *fp_new;
	char buf[512];
	char p_name[256];
	unsigned char p_md[MD5_LENGTH*2];
	char *p_cmp_name;
	int p_size;
	int p_number;
	struct stat statbuf;

	char *path;
	path = strstr(path_local, "/system/");
	if(path == NULL){
		path = path_local;
	}
	//SLOGI(">> path: %s, path_loacl: %s\n", path, path_local);

	fp_info = fopen(TEMP_FILE_IN_RAM, "r");
	if(fp_info){
		if(fgets(buf, sizeof(buf), fp_info) != NULL){
			//while(fgets(buf, sizeof(buf), fp_info)&&!found)
			while(fgets(buf, sizeof(buf), fp_info)){
				memset(p_md, '0', sizeof(p_md));
				if (sscanf(buf, "%d    %s    %u    %s", &p_number,p_name,&p_size,p_md) == 4){
					//TODO: can not get correct p_name from sscanf(), so use below instead
					char *p1 = strchr(buf, '\t');
					char *p2 = strchr(p1+1, '\t');
					if(p1&&p2)
						memcpy(p_name, p1+1, p2-p1-1);

					//TODO:path[0] will be '\0' sometimes, and it should be '/'
					path[0] = '/';
					if(strstr(p_name,path)!=NULL){
						p_cmp_name=strstr(p_name,path);
						if(strcmp(p_cmp_name,path)==0){
							found=1;
							int rettmp = 0;
							clear_bit(p_number);
							if(p_size==nCS){
								//SLOGI("%s crc check pass\n",path);
							}
							else{
								SLOGI("expected crc is %u\n",p_size);
								SLOGI("computed crc is %u\n",nCS);
								SLOGI("%s is modifyed\n",path);
								//fclose(fp_info);
								rettmp = CHECK_FILE_NOT_MATCH;
							}

							if(p_md[1]=='\0')
								p_md[1]='0';
							hextoi_md5(p_md);
							if(memcmp(nMd5, p_md, MD5_LENGTH)==0){
								//SLOGI("%s md5 check pass\n",path);
							}
							else{
								int i;
								for(i=0;i<16;i++){
									SLOGI("%02x",nMd5[i]);
								}
								for(i=0;i<16;i++){
									SLOGI("%02x", p_md[i]);
								}
								SLOGI("<<ERROR>>\n");
								//check_file_result->n_modifyfile+=1;
								SLOGI("Error:%s has been modified md5",path);
								ret=stat(path,&statbuf);
								if(ret != 0){
									SLOGI("Error:%s is not exist\n",path);
								}
								time_t modify=statbuf.st_mtime;
								SLOGI("on %s\n", ctime(&modify));
								//fclose(fp_info);
								//return CHECK_FILE_NOT_MATCH;
								rettmp = CHECK_FILE_NOT_MATCH;
							}
							if(rettmp){
								check_file_result->n_modifyfile+=1;
								clear_bit_m(p_number);
								fclose(fp_info);
								return rettmp;
							}
						}
					}
				}
			}

			if(found==0){
				SLOGI("found a new file,filename is %s\n",path);
				check_file_result->n_newfile+=1;

				if(access(FILE_NEW_TMP,0)==-1){
					int fd_new=creat(FILE_NEW_TMP,0755);
				}
				fp_new=fopen(FILE_NEW_TMP, "a");
				if(fp_new){
					fprintf(fp_new,"%s\n",path);
				}
				else{
					SLOGI("open %s error,error reason is %s\n",FILE_NEW_TMP,strerror(errno));
					return CHECK_ADD_NEW_FILE;
				}
				checkResult=false;
				fclose(fp_info);
				fclose(fp_new);
				return CHECK_ADD_NEW_FILE;
			}
		}
	}
	else {
		SLOGI("open %s error,error reason is %s\n",TEMP_FILE_IN_RAM,strerror(errno));
		return CHECK_NO_KEY;
	}

	fclose(fp_info);
	return CHECK_PASS;
}

static int crc_compute_for_local( const char* path, unsigned int* nCS, unsigned char *nMd5)
{
	char buf[4*1024];
	FILE *fp = 0;
	int rRead_count = 0;
	int i = 0;
	*nCS = 0;
	MD5_CTX md5;

	MD5Init(&md5);
	memset(nMd5, 0, MD5_LENGTH);

	struct stat st;
	memset(&st,0,sizeof(st));

	if ( path == NULL ){
		SLOGI("crc_compute_for_local-> %s is null", path);
		return -1;
	}
	if(lstat(path,&st)<0){
		SLOGI("\n %s does not exist,lsta fail", path);
		return 1;
	}
	if(S_ISLNK(st.st_mode)){
		SLOGI("%s is a link file,just pass\n", path);
		return 0;
	}

	fp = fopen(path, "r");
	if( fp == NULL ){
		SLOGI("\ncrc_compute_for_local->path:%s ,fp is null", path);
		SLOGI("\nfopen fail reason is %s",strerror(errno));
		return -1;
	}
	while(!feof(fp)){
		memset(buf, 0x0, sizeof(buf));
		rRead_count = fread(buf, 1, sizeof(buf), fp);
		if( rRead_count <=0 )
			break;
		*nCS += crc32(*nCS, buf, rRead_count);
		MD5Update(&md5,(unsigned char*)buf, rRead_count);
	}

	MD5Final(&md5, nMd5);
	fclose(fp);
	return 0;
}

static bool crc_compare( char const*dir)
{
	struct dirent *dp;
	DIR *d_fd;
	unsigned int nCS = 0;
	unsigned char nMd5[MD5_LENGTH];
	char newdir[FILENAME_MAX];
	int find_pass=0;

	if ((d_fd = opendir(dir)) == NULL) {
		if(strcmp(dir,"/system/")==0) {
			SLOGI("open system dir fail,please check!\n");
			return false;
		}
		else {
			SLOGI("%s is selinux protected,this dir just pass!\n",dir);
			if(clear_selinux_dir(dir) != 0) {
				SLOGI("clear selinux dir fail\n");
			}
			return false;
		}
	}
	while ((dp = readdir(d_fd)) != NULL) {
		find_pass = 0;
		if (strcmp(dp->d_name, ".") == 0
				|| strcmp(dp->d_name, "..") == 0
				|| strcmp(dp->d_name,"lost+found")==0){
			continue;
		}
		if (dp->d_type == DT_DIR){
			memset(newdir, 0, FILENAME_MAX);
			strcpy(newdir, dir);
			strcat(newdir, dp->d_name);
			strcat(newdir, "/");
			if(crc_compare(newdir) == false){
				//closedir(d_fd);
				continue;
				//return false;
			}
		}
		else{
			unsigned int idx = 0;
			unsigned int idy = 0;
			memset(newdir, 0, FILENAME_MAX);
			strcpy(newdir, dir);
			strcat(newdir, dp->d_name);
			for(; idx < sizeof(file_to_check)/sizeof(char*); idx++){
				if(strcmp(dp->d_name, file_to_check[idx]) == 0){
					root_to_check[idx]=1;
					SLOGI("Dir_check---found a root File:  %s\n",dp->d_name);
				}
			}
			for(; idy < sizeof(file_to_pass)/sizeof(char*); idy++){
				if(strcmp(dp->d_name, file_to_pass[idy]) == 0){
					SLOGI("Dir_check---found a file to pass:  %s\n",dp->d_name);
					find_pass=1;
					break;
				}
			}
			if(find_pass==0){
				SLOGI("scanning **** %s ****\n",dp->d_name);
				if(0 == crc_compute_for_local(newdir, &nCS, nMd5)){
					if (crc_compare_really(newdir, nCS, (char*)nMd5)!=0){
						SLOGI("Error:%s check fail\n",newdir);
						checkResult = false;
					}
					check_file_result->file_number_to_check++;
				}
				else if(1 == crc_compute_for_local(newdir, &nCS, nMd5)) {
					SLOGI("%s could be selinux protected\n",newdir);
					//closedir(d_fd)
					if (clear_selinux_file(newdir)!=0) {
						SLOGI("Error:%s is a selinux file,clear bit fail\n",newdir);
						checkResult = false;
					}
					check_file_result->file_number_to_check++;
					continue;
				}
				else {
					SLOGI("check %s error\n",newdir);
					closedir(d_fd);
					return false;
				}
			}
		}
	}

	closedir(d_fd);
	return true;
}

/*
* For List ROOT/LOST/NEW/MODIFY Files
*/
static int list_root_file()
{
	int idx=0;

	for(;idx<MAX_ROOT_TO_CHECK;idx++){
		if(root_to_check[idx]==1){
			check_file_result->n_rootfile+=1;
			SLOGI("found a root file,%s\n",file_to_check[idx]);
		}
	}

	return 0;
}

static int list_lost_file(int number)
{
	FILE *fp_info;
	char buf[512];
	char p_name[256];
	char p_md[256];
	int found=0;
	char *p_cmp_name=NULL;
	unsigned int p_size;
	int p_number;
	unsigned int idy=0;

	fp_info = fopen(TEMP_FILE_IN_RAM, "r");
	if(fp_info){
		if (fgets(buf, sizeof(buf), fp_info) != NULL) {
			while (fgets(buf, sizeof(buf), fp_info)) {
				if (sscanf(buf, "%d    %s %u    %s", &p_number,p_name,&p_size,p_md) == 4) {
					if(p_number==number){
						p_cmp_name=strstr(p_name,"/system");
						for(; idy < sizeof(file_to_pass)/sizeof(char*); idy++){
							if(strstr(p_cmp_name, file_to_pass[idy]) != NULL){
								SLOGI("\tname : %-50s --found a file to pass\n",p_cmp_name);
								//checkResult=true;
								//break;
								return 0;
							}
						}
						if(p_cmp_name != NULL){
							check_file_result->n_lostfile+=1;
							SLOGI("\tname : %-50s is lost\n",p_cmp_name);
							checkResult=false;
						}
						else{
							check_file_result->n_lostfile+=1;
							SLOGI("\tname : %-50s is lost\n",p_name);
							checkResult=false;
						}
						found=1;
						break;
					}
				}
			}
			if(!found){
				SLOGI("\tError:not found a lost file: %d\n", number);
				fclose(fp_info);
				return -1;
			}
		}
	}
	else{
		SLOGI("fopen error,error reason is %s\n",strerror(errno));
		return -1;
	}

	fclose(fp_info);
	return 0;
}

static int list_new_file()
{
	FILE *fp_new;
	struct stat statbuf;
	char buf[256];

	if(access(FILE_NEW_TMP,0)==-1){
		SLOGI("%s is not exist\n",FILE_NEW_TMP);
		return 0;
	}
	fp_new= fopen(FILE_NEW_TMP, "r");
	SLOGI("\n>> New File List <<\n");
	if(fp_new){
		while (fgets(buf, sizeof(buf), fp_new)) {
			char *token = strtok(buf, "\n");
			int ret=stat(buf,&statbuf);
			time_t modify=statbuf.st_mtime;
			SLOGI("\tname : %-50s --Created on  : %s", token, ctime(&modify));
		}
	}
	else{
		SLOGI("open %s error,error reason is %s\n",FILE_NEW_TMP,strerror(errno));
		return -1;
	}

	fclose(fp_new);
	return 0;
}

static int list_modify_file(int number)
{
	FILE *fp_info;
	struct stat statbuf;
	char buf[512];
	char p_name[256];
	char p_md[256];
	int found=0;
	char *p_cmp_name=NULL;
	unsigned int p_size;
	int p_number;
	fp_info = fopen(TEMP_FILE_IN_RAM, "r");
	if(fp_info){
		if (fgets(buf, sizeof(buf), fp_info) != NULL) {
			while (fgets(buf, sizeof(buf), fp_info)) {
				if (sscanf(buf,"%d    %s    %u    %s", &p_number,p_name,&p_size,p_md) == 4) {
					if(p_number==number){
						p_cmp_name=strstr(p_name,"/system");
						if(p_cmp_name != NULL){
							int ret=stat(p_cmp_name,&statbuf);
							if(ret != 0){
								SLOGI("Error:%s is not exist\n",p_cmp_name);
							}
							time_t modify=statbuf.st_mtime;
								SLOGI("\tname : %-50s --Modified on : %s",p_cmp_name, ctime(&modify));
						}
						else{
							SLOGI("Error:%s is modifyed\n",p_name);
						}
						found=1;
						break;
					}
				}
			}
			if(!found){
				SLOGI("\tError:not found a modify file %d\n", number);
				fclose(fp_info);
				return -1;
			}
		}
	}
	else{
		SLOGI("fopen error,error reason is %s\n",strerror(errno));
		return -1;
	}

	fclose(fp_info);
	return 0;
}

/*
 * For release some resources
 */
static bool remove_check_file(const char *file_name)
{
	int  ret = 0;
	ret = unlink(file_name);
	if (ret == 0)
		return true;

	if (ret < 0 && errno == ENOENT)
		return true;

	return false;
}

static bool remove_check_dir(const char *dir_name)
{
	struct dirent *dp;
	DIR *d_fd;
	if ((d_fd = opendir(dir_name)) == NULL) {
		SLOGI("dir_check-<<<< %s not dir\n",dir_name);
		return false;
	}
	while ((dp = readdir(d_fd)) != NULL) {
		if (strcmp(dp->d_name, ".") == 0
				|| strcmp(dp->d_name, "..") == 0
				|| strcmp(dp->d_name,"lost+found")==0){
			continue;
		}
		if (dp->d_type == DT_DIR){
			char newdir[FILENAME_MAX]={0};
			memset(newdir, 0, FILENAME_MAX);
			strcpy(newdir, dir_name);
			strcat(newdir, dp->d_name);
			strcat(newdir, "/");
			if(remove_check_dir(newdir) == false){
				closedir(d_fd);
				return false;
			}
		}
		else{
			char newdir[FILENAME_MAX];
			int idx = 0;
			int idy = 0;
			memset(newdir, 0, FILENAME_MAX);
			strcpy(newdir, dir_name);
			strcat(newdir, dp->d_name);
			const char* to_remove_file;
			to_remove_file=newdir;
			if(!remove_check_file(to_remove_file)){
				SLOGI("Error:unlink %s fail\n",to_remove_file);
			}
		}
	}

	return true;
}

static void remove_unneed_file()
{
	if(!remove_check_file(TEMP_FILE_IN_RAM)){
		SLOGI("unlink temp system crc file error\n");
	}

	if(!remove_check_file(FILE_NEW_TMP)){
		SLOGI("unlink temp new file error\n");
	}

	if(!remove_check_file(FILE_COUNT_TMP)){
		SLOGI("unlink temp new file error\n");
	}

	if(!remove_check_file(DYW_DOUB_TMP)){
		SLOGI("unlink temp new file error\n");
	}
}

/*
* Load files with decrypt
*/
static char* decrypt_str(char *source,unsigned int key)
{
	char buf[FILENAME_MAX]={0};
	memset(buf, 0, FILENAME_MAX);
	int i;
	int j=0;
	int len=strlen(source);
	if(len%2 != 0){
		SLOGI("Error,sourcr encrypt filename length is odd");
		return NULL;
	}
	int len2=len/2;
	for(i=0;i<len2;i++){
		char c1=source[j];
		char c2=source[j+1];
		j=j+2;
		c1=c1-65;
		c2=c2-65;
		char b2=c2*16+c1;
		char b1=b2^key;
		buf[i]=b1;
	}
	buf[len2]='\0';
	memset(source,0,len);
	strcpy(source,buf);

	return source;
}

static int check_file_number_insystem(int file_number)
{
	FILE *fp_info;
	char buf[512];
	char p_name[128];
	int p_number;
	int p_pnumber;
	fp_info = fopen(TEMP_FILE_IN_RAM, "r");
	SLOGI(">> check file num in system <<\n");
	if(fp_info){
		if (fgets(buf, sizeof(buf), fp_info) != NULL) {
			if (sscanf(buf, "%d    %s %d", &p_pnumber,p_name, &p_number)== 3) {
				if (!strcmp(p_name, "file_number_in_system_dayu")) {
					check_file_result->expect_file_number=p_number;
					SLOGI("\tp_number is : %d\n\tfile_number is : %d\n\tmodfy num is : %d,\n\tnewfile num is : %d\n\n",
						p_number,file_number,
						check_file_result->n_modifyfile,
						check_file_result->n_newfile);
				}
			}
		}
	}
	else{
		SLOGI("fopen error,error reason is %s\n",strerror(errno));
		return 1;
	}

	fclose(fp_info);
	return 0;
}

static int encrypt_file_doub_check()
{
	char buf[512];
	FILE *fp_info;
	unsigned int file_count_crc;
	unsigned int nCS = 0;
	unsigned char nMd5[MD5_LENGTH];
	fp_info = fopen(DYW_DOUB_TMP, "r");
	if(fp_info){
		if(fgets(buf, sizeof(buf), fp_info) != NULL){
			if (sscanf(buf,"%u", &file_count_crc) == 1){
				check_file_result->file_count_check=file_count_crc;
			}
			else {
				SLOGI("double check file is error\n");
				return CHECK_NO_KEY;
			}
		}
		else {
			SLOGI("double check file is null\n");
			return CHECK_NO_KEY;
		}
	}
	else{
		SLOGI("open %s error,error reason is %s\n",DYW_DOUB_TMP,strerror(errno));
		return CHECK_NO_KEY;
	}

	if(0 == crc_compute_for_local(FILE_COUNT_TMP, &nCS, nMd5)){
		if(nCS!=check_file_result->file_count_check){
			SLOGI("file count double check fail\n");
			return CHECK_NO_KEY;
		}
	}

	return 0;
}

static int load_zip_file(char *part)
{
	//TODO: shenpengru: need support double system partition!!!
	//const char *ZIP_FILE_ROOT=strcat(part, "/system_checksum");
	char *ZIP_FILE_ROOT;
	char *checkfile = "/system_checksum";
	char base[512];
	strcpy(base, part);
	ZIP_FILE_ROOT = strcat(base, checkfile);

	ZipArchive zip;
	//struct stat statbuf;
	int ret;
	int err=1;
	MemMapping map;
	if (sysMapFile(ZIP_FILE_ROOT, &map) != 0) {
		SLOGI("failed to map file %s\n", ZIP_FILE_ROOT);
		return CHECK_MOUNT_ERR;
	}

	SLOGI("load zip file from %s\n",ZIP_FILE_ROOT);
	err = mzOpenZipArchive(map.addr, map.length, &zip);
	if (err != 0) {
		SLOGI("Can't open %s\n(%s)\n", ZIP_FILE_ROOT, err != -1 ? strerror(err) : "bad");
		sysReleaseMap(&map);
		return CHECK_NO_KEY;
	}
	const ZipEntry* file_count = mzFindZipEntry(&zip,FILE_COUNT_ZIP);
	if (file_count== NULL) {
		mzCloseZipArchive(&zip);
		sysReleaseMap(&map);
		return CHECK_NO_KEY;
	}
	unlink(FILE_COUNT_TMP);

	int fd_file = creat(FILE_COUNT_TMP, 0755);
	if (fd_file< 0) {
		mzCloseZipArchive(&zip);
		SLOGI("Can't make %s:%s\n", FILE_COUNT_TMP, strerror(errno));
		sysReleaseMap(&map);
		return CHECK_NO_KEY;
	}
	bool ok_file= mzExtractZipEntryToFile(&zip, file_count, fd_file);
	close(fd_file);

	if (!ok_file) {
		SLOGI("Can't copy %s\n", FILE_COUNT_ZIP);
		sysReleaseMap(&map);
		return CHECK_NO_KEY;
	}
	else{
		SLOGI("%s is ok\n", FILE_COUNT_TMP);
	}

	const ZipEntry* dcheck_crc =
	mzFindZipEntry(&zip,DOUBLE_DYW_CHECK);
	if (dcheck_crc== NULL) {
		mzCloseZipArchive(&zip);
		sysReleaseMap(&map);
		return CHECK_NO_KEY;
	}

	unlink(DYW_DOUB_TMP);
	int fd_d = creat(DYW_DOUB_TMP, 0755);
	if (fd_d< 0) {
		mzCloseZipArchive(&zip);
		SLOGI("Can't make %s\n", DYW_DOUB_TMP);
		sysReleaseMap(&map);
		return CHECK_NO_KEY;
	}

	bool ok_d = mzExtractZipEntryToFile(&zip, dcheck_crc, fd_d);
	close(fd_d);
	if (!ok_d) {
		SLOGI("Can't copy %s\n", DOUBLE_DYW_CHECK);
		sysReleaseMap(&map);
		return CHECK_NO_KEY;
	}
	else{
		SLOGI("%s is ok\n", DYW_DOUB_TMP);
	}

	mzCloseZipArchive(&zip);
	sysReleaseMap(&map);
	return 0;
}

static int load_system_encrypt_file() 
{
	FILE *fp_info;
	FILE *fp_tmp;
	char buf[512];
	char p_name[512];
	char *p_cmp_name=NULL;
	char p_crc[256];
	char p_md5[256];
	int p_number;
	int p_file_number;
	fp_info = fopen(FILE_COUNT_TMP, "r");
	fp_tmp = fopen(TEMP_FILE_IN_RAM,"w+");
	if(fp_tmp){
		if(fp_info) {
			if (fgets(buf, sizeof(buf), fp_info) != NULL) {
				if (sscanf(buf, "%d    %s %d", &p_number,p_name,&p_file_number) == 3) {
					fprintf(fp_tmp,"%d\t%s\t%d\n",p_number,p_name,p_file_number);
				}
				while (fgets(buf, sizeof(buf), fp_info)) {
					memset(p_md5, 0, sizeof(p_md5));
					if (sscanf(buf, "%d    %s %s %s", &p_number,p_name,p_crc,p_md5) == 4) {
						char *p_pname=decrypt_str(p_name,key);
						char *p_pcrc=decrypt_str(p_crc,key);
						char *p_pmd5=decrypt_str(p_md5,key);
						unsigned long crc;
						crc = strtoul(p_pcrc, (char **)NULL, 10);
						//SLOGI("p_pname:%s, crc32:%s %lu %d %d %d\n", p_pname, p_pcrc, crc, sizeof(long), sizeof(long long), sizeof(unsigned int));
						fprintf(fp_tmp,"%d\t%s\t%lu\t%s\n",p_number,p_pname,crc, p_pmd5);
					}
				}
			}
		}
		else {
			SLOGI("fopen error,error reason is %s\n",strerror(errno));
			return -1;
		}
	}
	else{
		SLOGI("fopen error,error reason is %s\n",strerror(errno));
		return CHECK_NO_KEY;
	}

	fclose(fp_info);
	fclose(fp_tmp);
	return 0;
}

/*
 * Unzip the check file and decrypt them.
 */
int load_all(char *part){
	check_file_result=(struct last_check_file*)malloc(sizeof(last_check_file));

	memset(check_file_result,0,sizeof(last_check_file));
	memset(check_map,0xff,sizeof(check_map));
	memset(check_modify,0xff,sizeof(check_modify));

	if(load_zip_file(part)){
		SLOGI("load source zip file fail\n");
		return CHECK_NO_KEY;
	}
	if(load_system_encrypt_file()){
		SLOGI("load system encrypt file fail\n");
		return CHECK_NO_KEY;
	}
	if(encrypt_file_doub_check()){
		SLOGI("encrypt file double check fail\n");
		return CHECK_NO_KEY;
	}

	return CHECK_PASS;
}

/*
 * Just for unzip+decrypt the check file and list the path+crc32+md5sum.
 */
int main_list(char *fliter, char *part){
	int ret=0;
	FILE *fp_info;
	char buf[512];
	char p_name[256];
	unsigned char p_md[MD5_LENGTH*2];
	char *p_cmp_name;
	unsigned int p_size;
	int p_number;

	ret = load_all(part);
	if(ret != CHECK_PASS){
		SLOGI("Load check file fail!\n");
		return CHECK_NO_KEY;
	}

	fp_info = fopen(TEMP_FILE_IN_RAM, "r");
	if(fp_info){
		SLOGI("-------------------------------------------------------------\n");
		if(fgets(buf, sizeof(buf), fp_info) != NULL){
			while(fgets(buf, sizeof(buf), fp_info)){
				if(fliter != NULL){
					if(strstr(buf,fliter)!=NULL){
						SLOGI("%s", buf);
					}
				}
				else{
					SLOGI("%s", buf);
				}
			}
		}
		SLOGI("-------------------------------------------------------------\n");
	}
	else{
		SLOGI("Open fail\n");
	}

	return 0;
}

/*
* For check the file which is new/lost/modified.
*/
int main_check(char *part){
	int per,cper;
	int ret = 0;
	int realresult = 0;

	SLOGI("Now check begins, please wait.....%s\n", part);
	ret = load_all(part);
	if(ret != CHECK_PASS){
		SLOGI("Load check file fail!\n");
		return -1;
	}

	SLOGI("\n\n========== ========== START SYSTEM MD5 CHECK ========== ==========\n");
	//TODO: shenpengru: need support double system partition!!!
	char base[512];
	strcpy(base, part);
	strcat(base, "/");
	if(false == crc_compare(base)){
		checkResult = -1;
	}

	check_file_result->file_number_to_check+=1;

	SLOGI("\n\n========== ========== SHOW THE RESULT ====== ========== ==========\n");
	if (check_file_number_insystem(check_file_result->file_number_to_check)!=0){
		checkResult=-1;
	}

	//new
	if(list_new_file()){
		SLOGI("list new file error\n");
	}
	//root
	if(list_root_file()){
		SLOGI("list root file error\n");
	}
	//lost
	SLOGI("\n>> Lost File List <<\n");
	//for(cper=0;cper<check_file_result->file_number_to_check-1;cper++){
	for(cper=0;cper<check_file_result->expect_file_number-1;cper++){
		if(test_bit(cper)){
			//checkResult=false;
			list_lost_file(cper);
		}
	}
	//modify
	SLOGI("\n>> Modified File List <<\n");
	//for(cper=0;cper<check_file_result->file_number_to_check-1;cper++){
	for(cper=0;cper<check_file_result->expect_file_number-1;cper++){
		if(!test_bit_m(cper)){
			checkResult=-1;
			list_modify_file(cper);
		}
	}
	SLOGI("\n\n");
	if(check_file_result->n_newfile){
		SLOGI("[Report] found %d new files\n",check_file_result->n_newfile);
	}
	if(check_file_result->n_lostfile){
		realresult = -1;
		SLOGI("[Report] found %d lost files\n",check_file_result->n_lostfile);
	}
	if(check_file_result->n_modifyfile){
		realresult = -1;
		SLOGI("[Report] found %d modified files\n",check_file_result->n_modifyfile);
	}
	if(check_file_result->n_rootfile){
		SLOGI("[Report] found %d root files\n",check_file_result->n_rootfile);
	}

	int m_root_check=0;
	for(;m_root_check<MAX_ROOT_TO_CHECK;m_root_check++){
		root_to_check[m_root_check]=0;
	}
	remove_unneed_file();
	free(check_file_result);

	return realresult;
}
