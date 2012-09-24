#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# fit_gaussians.py

import sys
import glob
import re
from os.path import basename
import numpy as np
from sklearn.cluster import MeanShift
import scipy.optimize as optimize
from scipy.constants import pi

# Tuning parameters
sc = 75           # Scaling parameter for RT
bw = 0.08         # Bandwidth for MeanShift

def gaussian(cx,cy,wx,wy,ht):
	return lambda pts: ht*np.exp(-(np.square((cx-pts[:,0])/wx)+np.square((cy-pts[:,1])/wy))/2.)

def multi_gaussian(ps):
	return lambda pts: np.sum([gaussian(*p)(pts) for p in np.reshape(ps,(-1,5))], axis=0)

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
				pts.append([float(x) for x in res.groups()[1:4]])
		f.close()
		pts = np.array(pts)
		ms.fit(pts[:,0:2]/[sc,1])
		errfn = lambda p: multi_gaussian(p)(pts[:,0:2])-pts[:,2]
		p0 = []
		for i in range(len(ms.cluster_centers_)):
			cluster_pts = pts[ms.labels_ == i]
			p0 += list(ms.cluster_centers_[i]*[sc,1]) # RT and mz centers
			p0.append(np.ptp(cluster_pts[:,0])/5.)    # RT width
			p0.append(np.ptp(cluster_pts[:,1])/5.)    # mz width
			p0.append(np.amax(cluster_pts[:,2]))      # I height
		p0 = np.array(p0)
		cons = [lambda ps: np.amin(ps)]
		p_opt = optimize.fmin_cobyla(lambda ps: np.sum(np.square(errfn(ps))),p0,cons,disp=0)

		print file + ' : ' + str(len(ms.cluster_centers_)) + ' peak'
		total_vol = 0
		for p in np.reshape(p_opt,(-1,5)):
			vol = np.sum(gaussian(*p)(pts[:,0:2]))
			total_vol += vol
			print 'mz: '+str(p[1])+' RT: '+str(p[0])
			print 'W_mz: '+str(p[3])+' W_RT: '+str(p[2])
			print 'Height: '+str(p[4])+' Volume: '+str(vol)
		print 'Total calculated intensity: ' + str(total_vol)
		print 'Total recorded intensity: ' + str(np.sum(pts[:,2]))
		print
	return 0

if __name__ == '__main__':
	main()
