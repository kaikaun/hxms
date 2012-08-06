#!/usr/bin/env python
# -*- coding: utf-8 -*-
#       checkclusters.py

import sys
import os
import fnmatch
import re

def main():
	r = re.compile('(\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)')
	for path in [p for p in sys.argv[1:] if os.path.exists(p)]:
		for subdir, dirs, files in os.walk(path):
			for file in fnmatch.filter(files,'*.clust'):
				f=open(os.path.join(subdir,file), 'r')
				lines=f.readlines()
				f.close()
				pts = []
				ok = []
				for line in lines:
					result=r.search(line)
					pts.append((int(result.group(1)), float(result.group(3))))
					ok.append(False)
				for i1,v1 in enumerate(pts):
					if ok[i1] == False:
						for i2,v2 in enumerate(pts):
							if i1 != i2:
								if abs(v1[0]-v2[0])<=3 and abs(v1[1]-v2[1])<=0.05:
									ok[i1]=True
									ok[i2]=True
				for i1,v1 in enumerate(pts):
					if ok[i1] == False:
						print file, 'has orphan point', v1
	return 0

if __name__ == '__main__':
	main()
