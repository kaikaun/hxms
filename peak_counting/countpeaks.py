#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# countpeaks.py

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
	files = []
	for g in sys.argv[1:]:
		files += glob.glob(g)
	files = [f for f in files if re.match('^\d{6}\.clust$', basename(f))]
	if not files:
		print 'Usage: ' + basename(__file__) + ' <cluster> ...'
		return -1
	lre = re.compile('(\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)')
	ms = MeanShift(bandwidth=bw, bin_seeding=True)
	for file in files:
		pts = []
		f = open(file, 'r')
		for line in f:
			res = lre.search(line)
			if res:
				pts.append([float(res.group(2))/sc, float(res.group(3))])
		f.close()
		ms.fit(np.array(pts))
		labels_unique = np.unique(ms.labels_)
		n_clusters_ = len(labels_unique)
		print file + ': %d' % n_clusters_
	return 0

if __name__ == '__main__':
	main()
