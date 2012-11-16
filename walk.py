#!/usr/share/env python

import glob
import os
import sys

filenumber = 0
failed = 0
totalpngsize = 0
cansavepngsize = 0
totalzscisize = 0
numcansave = 0

def convert(dir,f):
	cwd = os.getcwd()
	os.chdir(dir)
	os.system('~/png2zsci/png2zsci/png2zsci -c 100 %s'%f)
	os.chdir(cwd)

if __name__ == '__main__':
	dir = sys.argv[1]
	for root, dirs, files in os.walk(dir):
		for f in files:
			filename, ext = os.path.splitext(f)
			if ext != '.png':
				continue
			try:
				convert(root,f)
				pngsize = os.path.getsize(os.path.join(root,f))
				#root_8 = root.replace('assetNew','png8')
				#pngsize = os.path.getsize(os.path.join(root_8,f))
				zscisize = os.path.getsize(os.path.join(root,filename+'.zsci'))
				print f, pngsize, zscisize
				totalpngsize += pngsize
				filenumber += 1
				if pngsize > zscisize:
					numcansave += 1
					totalzscisize += zscisize
					cansavepngsize += pngsize
			except:
				failed += 1
	print 'filenumber: %d'%filenumber
	print 'numcansave:%d'%numcansave
	print 'failed: %d'%failed
	print 'totalpngsize: %d'%totalpngsize
	print 'cansavepngsize: %d'%cansavepngsize
	print 'totalzscisize: %d'%totalzscisize 
				
			
