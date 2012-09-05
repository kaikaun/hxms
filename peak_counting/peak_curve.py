#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# peak_curve.py

import sys
import glob
import re
from os.path import basename
import numpy as np
from scipy.optimize import leastsq

min_I = 0    # Max I on a scan must be at least this to be accepted
min_len = 3  # Scan must have at least this number of points to be accepted

def residuals(p, mz, t):
	return mz - (p[0]  + p[1] * t + p[2] * t**2 + p[3] * t**3)

def curvature(p, t):
	mz1 = p[1] + 2 * p[2] * t + 3 * p[3] * t**2
	mz2 = 2 * p[2] + 6 * p[3] * t
	return abs(mz2) / ((1 + mz1**2)**1.5)

def main():
	files = []
	for g in sys.argv[1:]:
		files += glob.glob(g)
	files = [f for f in files if re.match('^\d{6}\.clust$', basename(f))]
	if not files:
		print 'Usage: ' + basename(__file__) + ' <cluster> ...'
		return -1
	lre = re.compile('(\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)')
	for file in files:
		pts = []
		f = open(file, 'r')
		for line in f:
			res = lre.search(line)
			if res:
				pts.append((float(res.group(2)), float(res.group(3)), float(res.group(4))))
		f.close()
		pt = np.array(pts,dtype=[('RT',float),('mz',float),('I',float)])
		mzs = []
		ts = []
		for RT in np.unique(pt['RT']):
			sc = pt[pt['RT'] == RT]
			if len(sc)>=min_len and np.amax(sc['I'])>=min_I:
				ts.append(RT);
				mzs.append(np.average(sc['mz'],weights=sc['I']))
		mz = np.array(mzs)
		t = np.array(ts)
		p0 = [mz[0],mz[-1]-mz[0],0.01,0.01]
		params = leastsq(residuals, p0, args=(mz, t))
		curv = curvature(params[0], t)
		print file + ": " + str(np.amax(curv))
	return 0

if __name__ == '__main__':
	main()
