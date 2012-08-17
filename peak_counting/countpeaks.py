#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#       countpeaks.py
#       

import sys
import os
import fnmatch
import re
import numpy as np
from sklearn.cluster import MeanShift

# Tuning parameters
bw = 0.15 # Bandwidth for MeanShift
sc = 50   # Scaling parameter for RT

def main():
	r = re.compile('(\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)')
	ms = MeanShift(bandwidth=bw, bin_seeding=True)
	paths = [p for p in sys.argv[1:] if os.path.exists(p)]
	if not paths:
		print "Usage: "+os.path.basename(__file__)+" <cluster directory> ..."
		return -1
	for path in paths:
		for subdir, dirs, files in os.walk(path):
			for file in sorted(fnmatch.filter(files,'*.clust')):
				f=open(os.path.join(subdir,file), 'r')
				lines=f.readlines()
				f.close()
				pts = []
				for line in lines:
					res=r.search(line)
					pts.append([float(res.group(2))/sc, float(res.group(3))])
				ms.fit(np.array(pts))
				labels_unique = np.unique(ms.labels_)
				n_clusters_ = len(labels_unique)
				print os.path.join(subdir,file) + ": %d" % n_clusters_
	return 0

if __name__ == '__main__':
	main()
