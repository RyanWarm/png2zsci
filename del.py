#!/usr/share/env python

import glob
import os
import sys

def delete(dir,f):
        cwd = os.getcwd()
        os.chdir(dir)
        os.system('rm -f %s'%f)
        os.chdir(cwd)

if __name__ == '__main__':
	dir = sys.argv[1]
	for root, dirs, files in os.walk(dir):
		for f in files:
			filename, ext = os.path.splitext(f)
			if ext != '.db':
				continue
			
			print f
			delete(root,f)
				
				
