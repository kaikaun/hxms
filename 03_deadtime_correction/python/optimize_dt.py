#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# optimize_dt.py

from os.path import basename
from glob import glob
import sys
import re
import math
import numpy as np
import scipy.optimize as opt

pulses = 0.23 / 45E-6
prop = 1.078 / math.sqrt(2 * 96485333.7 * 5630)

min_I = 0    # Max I on a scan must be at least this to be accepted
min_len = 3  # Scan must have at least this number of points to be accepted

dt0 = [5.0,2.0] # Initial guess for non-extending and extending DTs in ns

def residuals(p, mz, t):
	return mz - (p[0]  + p[1] * t + p[2] * t**2 + p[3] * t**3)

def curvature(p, t):
	mz1 = p[1] + 2 * p[2] * t + 3 * p[3] * t**2
	mz2 = 2 * p[2] + 6 * p[3] * t
	return mz2 / ((1 + mz1**2)**1.5)

def flight_time(mz):
	return prop * math.sqrt(mz)

def spectrum_curvature(spectrum):
	mzs = []
	ts = []
	for RT, scan in spectrum.iteritems():
		if len(scan)>=min_len and max(scan.values())>=min_I:
			scan_mzs = np.array(scan.keys())
			scan_Is = np.array(scan.values())
			mzs.append(np.average(scan_mzs,weights=scan_Is))
			ts.append(RT)
	mz = np.array(mzs)
	t = np.array(ts)
	p0 = [mz[0],mz[-1]-mz[0],0.01,0.01]
	params = opt.leastsq(residuals, p0, args=(mz, t))
	return curvature(params[0], t)

def correct_deadtime(dt, spectrum):
	new_spectrum = {}
	for RT, scan in spectrum.iteritems():
		new_scan = {}
		for mz in sorted(scan.iterkeys()):
			pulses_not_hit = pulses
			pulses_blocked = 0
			FT = flight_time(mz)
			for mz2 in sorted(new_scan.iterkeys(), reverse=True):
				FT_diff = FT - flight_time(mz2)
				if FT_diff > dt[0]*1e-9:
					break
				if FT_diff > dt[1]*1e-9:
					pulses_not_hit -= scan[mz2]
				else:
					pulses_blocked += new_scan[mz2]
			new_I = scan[mz]*math.exp(pulses_blocked / pulses) / pulses_not_hit
			new_I = -math.log(1 - new_I) * pulses
			new_scan[mz] = new_I
		new_spectrum[RT] = new_scan
	return new_spectrum

def total_curv(dt, spectra):
	curv = 0
	for spectrum in spectra:
		curv += abs(sum(spectrum_curvature(correct_deadtime(dt,spectrum))))
	return curv

def main():
	files = []
	for g in sys.argv[1:]:
		files += glob(g)
	files = [f for f in files if re.match('^\d{6}\.clust$', basename(f))]
	if not files:
		print 'Usage: ' + basename(__file__) + ' <cluster> ...'
		return -1

	spectra = []
	lre = re.compile('(\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)')
	for file in files:
		spectrum={}
		f = open(file, 'r')
		for line in f:
			res = lre.search(line)
			if res:
				RT = float(res.group(2))
				if RT not in spectrum:
					spectrum[RT] = {}
				spectrum[RT][float(res.group(3))] = float(res.group(4))
		f.close()
		spectra.append(spectrum)

	# Constrained COBYLA 
	#cons = [lambda dt,sp: dt[1], lambda dt,sp: dt[0]-dt[1]]
	#dt, c = opt.fmin_cobyla(total_curv, dt0, cons, rhobeg=4, args=(spectra,))

	# Brute force search
	#dt = opt.brute(total_curv, ((0,10),(0,10)), args=(spectra,), Ns=100)

	# Simulated annealing
	dt,c = opt.anneal(total_curv,dt0,args=(spectra,),lower=[0,0],upper=[10,4])

	print dt
	#print c
	print 'Non-extending DT: ' + str(dt[0]) + ' ns'
	print 'Extending DT: ' + str(dt[1]) + ' ns'
	#print 'Total curvature: ' + str(c)

	return 0

if __name__ == '__main__':
	main()
