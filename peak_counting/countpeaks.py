#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
#       countpeaks.py
#       

import sys
import glob
import re
import numpy as np
from os.path import basename
from sklearn.cluster import MeanShift

# Tuning parameters
bw = 0.15 # Bandwidth for MeanShift
sc = 50   # Scaling parameter for RT

def main():
	r = re.compile('(\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)')
	fr = re.compile('\d{6}\.clust')
	ms = MeanShift(bandwidth=bw, bin_seeding=True)
	files = []
	for g in sys.argv[1:]:
		files += glob.glob(g)
	files = [f for f in files if fr.search(basename(f))]
	if not files:
		print 'Usage: ' + basename(__file__) + ' <cluster> ...'
		return -1
	for file in files:
		f = open(file, 'r')
		lines = f.readlines()
		f.close()
		pts = []
		for line in lines:
			res = r.search(line)
			pts.append([float(res.group(2))/sc, float(res.group(3))])
		ms.fit(np.array(pts))
		labels_unique = np.unique(ms.labels_)
		n_clusters_ = len(labels_unique)
		print file + ': %d' % n_clusters_
	return 0

if __name__ == '__main__':
	main()
