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
from sklearn.cluster import MeanShift, estimate_bandwidth

def main():
	r = re.compile('(\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)')
	
	for path in [p for p in sys.argv[1:] if os.path.exists(p)]:
		for subdir, dirs, files in os.walk(path):
			for file in fnmatch.filter(files,'*.clust'):
				f=open(os.path.join(subdir,file), 'r')
				lines=f.readlines()
				f.close()
				pts = []
				for line in lines:
					result=r.search(line)
					pts.append([int(result.group(1)), float(result.group(3))])
				p = np.array(pts)
				bw = estimate_bandwidth(p)
				ms = MeanShift(bandwidth=bw, bin_seeding=True)
				ms.fit(p)
				labels = ms.labels_
				cluster_centers = ms.cluster_centers_
				labels_unique = np.unique(labels)
				n_clusters_ = len(labels_unique)
				print os.path.join(subdir,file) + ": %d" % n_clusters_
	return 0

if __name__ == '__main__':
	main()
