#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# optimize_dt.py

import sys
import glob
from os.path import basename
import re
import math
import numpy as np
import scipy.optimize as opt

pulses = 0.23 / 45E-6
prop = 1.078 / math.sqrt(2 * 96485333.7 * 5630)

min_I = 0     # Max I on a scan must be at least this to be accepted
min_len = 4   # Scan must have at least this number of points
min_scans = 5 # Spectrum must have at least this number of scans

dt0 = [5.0,2.0] # Initial guess for non-extending and extending DTs in ns

def residuals(p, mz, t):
	return mz - (p[0]  + p[1] * t + p[2] * t**2 + p[3] * t**3)

def curvature(p, t):
	mz1 = p[1] + 2 * p[2] * t + 3 * p[3] * t**2
	mz2 = 2 * p[2] + 6 * p[3] * t
	return mz2 / ((1 + mz1**2)**1.5)

def spectrum_curvature(spectrum):
	mz = [np.average(s.keys(),weights=s.values()) for s in spectrum.values()]
	mz = np.array(mz)
	t = np.array(spectrum.keys())
	p0 = np.array([mz[0],mz[-1]-mz[0],0.01,0.01])
	params = opt.leastsq(residuals, p0, args=(mz, t))
	return curvature(params[0], t)

def total_curv(dt, spectra):
	curv = [np.sum(spectrum_curvature(correct_deadtime(dt,s))) for s in spectra]
	return np.sum(np.abs(curv))

def correct_deadtime(dt, spectrum):
	flight_time = lambda mz: prop * math.sqrt(mz)
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
			if new_I > 1:
				print "Invalid peak values for deadtime correction!"
				print "Check that peak values have not been previously corrected"
				return -2
			new_I = -math.log(1 - new_I) * pulses
			new_scan[mz] = new_I
		new_spectrum[RT] = new_scan
	return new_spectrum

def main():

	if ( len(sys.argv) <= 1 ):
		print 'Usage: ' + basename(__file__) + ' <file w. cluster names>'
		return -1	

	filenames = sys.argv[1]
	try:
		inf = open (filenames, "r")
	except:
		print "error opening ", filenames
		return -1

	files = sum([line.rstrip().split(" ") for line in inf],[])

	spectra = []
	lre = re.compile('(\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)')
	for file in files:
		spectrum={}
		with open(file, 'r') as f:
			for line in f:
				res = lre.search(line)
				if res:
					RT = float(res.group(2))
					if RT not in spectrum:
						spectrum[RT] = {}
					spectrum[RT][float(res.group(3))] = float(res.group(4))
		for RT, scan in spectrum.copy().iteritems():
			if len(scan) < min_len or max(scan.values()) < min_I:
				del spectrum[RT]
		if len(spectrum) >= min_scans:
			spectra.append(spectrum)



	return 0

if __name__ == '__main__':
	main()
