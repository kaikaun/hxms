#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# countpeaks.py

import sys
import glob
import re
from os.path import basename
import numpy as np
import sklearn.cluster

# Tuning parameters
sc = 75           # Scaling parameter for RT vs 
bw = 0.08         # Bandwidth for MeanShift
met = 'chebyshev' # Distance metric for DBSCAN
eps = 0.015       # Max distance for DBSCAN
mnn = 3           # Min neighbours for DBSCAN
#damp = 0.5        # Damping factor for Affinity propagation

def main():
	files = []
	for g in sys.argv[1:]:
		files += glob.glob(g)
	files = [f for f in files if re.match('^\d{6}\.clust$', basename(f))]
	if not files:
		print 'Usage: ' + basename(__file__) + ' <cluster> ...'
		return -1
	lre = re.compile('(\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)')
	ms = sklearn.cluster.MeanShift(bandwidth=bw, bin_seeding=True)
	db = sklearn.cluster.DBSCAN(metric=met, eps=eps, min_samples=mnn)
	#af = sklearn.cluster.AffinityPropagation(damping=damp)
	for file in files:
		pts = []
		high_I = 0
		f = open(file, 'r')
		for line in f:
			res = lre.search(line)
			if res:
				pts.append([float(res.group(2))/sc, float(res.group(3))])
				I = float(res.group(4))
				if I > high_I:
					high_I = I
		f.close()
		pt = np.array(pts)

		# MeanShift
		ms.fit(pt)
		ms_clusters = len(np.unique(ms.labels_))

		# DBSCAN
		db.fit(pt)
		db_clusters = len(np.unique(db.labels_))-(1 if -1 in db.labels_ else 0)
		
		# Affinity propagation
		#pt_norms = np.sum(pt ** 2, axis=1)
		#S = -pt_norms[:,np.newaxis]-pt_norms[np.newaxis,:]+2*np.dot(pt,pt.T)
		#p = 10 * np.median(S)
		#af.fit(S,p)
		#af_clusters = len(af.cluster_centers_indices_)

		print file+': MS %d'%ms_clusters+' DB %d'%db_clusters+' max %.3f'%high_I
	return 0

if __name__ == '__main__':
	main()
