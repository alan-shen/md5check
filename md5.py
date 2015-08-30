#!/usr/bin/python

import os
import subprocess
import sys
import binascii
import string
import zipfile
import hashlib

def fileCountIn(dir):
	return sum([len(files) for root,dirs,files in os.walk(dir)])

def cal_system_file_crc_noencry(file_count,in_dir):
	value=-1
	number=0
	crc=0
	fo=open(file_count,"w")
	fo.write(str(value))
	fo.write("\t")	
	fo.write("file_number_in_system_dayu")
	fo.write("\t")
	fo.write(str(fileCountIn(in_dir)))
	fo.write("\n")
	for root, dirs, files in os.walk(in_dir):
		for file in files:
			if file=="file_count":
				break
			if os.path.isfile(os.path.join(root,file)) and not os.path.islink(os.path.join(root,file)):
				mymd5=hashlib.md5()
				frb=open(os.path.join(root,file),'rb')
				while True:
					tmp=frb.read(4*1024)
					if tmp=='':
						break
					mymd5.update(tmp)
			print ("\t%s\t%s" %(mymd5.hexdigest(),  os.path.join(root,file)))
			fo.write(str(number))
			fo.write("\t")
			if not os.path.islink(os.path.join(root,file)):
				filemd5=mymd5.hexdigest()
			else:
				filemd5=0
			fo.write(filemd5)
			fo.write("\t")
			filepath=os.path.join(root,file)	
			fo.write(filepath)
			fo.write("\n")
			number+=1
			frb.close()
	fo.close()

#
#./build/tools/releasetools/rootcheck.py \
#        $(TARGET_OUT) \					argv[0]
#        $(TARGET_OUT)/data					argv[1]
#
def main(argv):
	print ">> md5 generate start"
	out_dir = argv[1]
	in_dir = argv[0]
	print in_dir
	file_count=out_dir+"/md5_system"

	print "cal system file crc begin\n----------"	
	#cal_system_file_crc(file_count,in_dir)
	cal_system_file_crc_noencry(file_count,in_dir)
	print "----------\ncal system file crc end"
	
	print "<< md5 generate end"

if __name__ == '__main__':
	main(sys.argv[1:])
