#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# optimize_dt.py

import argparse
import re
import numpy as np
import scipy.optimize as opt
from TOFMS_deadtime import correct_deadtime
from anneal_patch   import anneal

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

def total_curv(fudge_parameters, blah, spectra):
	non_ext, extending, pulses, prop = fudge_parameters
	corr_spectra = [correct_deadtime(non_ext, extending, pulses,prop,s) for s in spectra]
	curv = [np.sum(spectrum_curvature(s)) for s in corr_spectra]
	return np.sum(np.abs(curv))

def main():
	parser = argparse.ArgumentParser( \
				description='Find optimized deadtime for clusters.')
	parser.add_argument('clusters', 
						help='file containing cluster filenames')

	parser.add_argument('-d','--ext', dest='ext', type=float, default=1.0, 
						help='initial guess for extending deadtime (ns)')
	parser.add_argument('-D','--non', dest='non', type=float, default=3.0, 
						help='initial guess for non-extending deadtime (ns)')


	parser.add_argument('-s','--scantime', dest='ST', type=float, default=0.23, 
						help='scan time (s)')
	parser.add_argument('-c','--cycletime', dest='CT', type=float, default=45., 
						help='cycletime (μs)')
	parser.add_argument('-l','--length',  dest='L', type=float, default=1078., 
						help='effective TOF path length (mm)')
	parser.add_argument('-v','--voltage', dest='V', type=float, default=5630., 
						help='effective accelerating voltage (V)')
	parser.add_argument('--min_I',        type=float, default=1., 
						help='minimum scan peak intensity')
	parser.add_argument('--min_len',      type=int, default=4, 
						help='minimum scan length in points')
	parser.add_argument('--min_scans',    type=int, default=5, 
						help='minimum spectrum length in scans')
	args = parser.parse_args()

	dt0    = np.array([args.ext, args.non])
	pulses = args.ST / (args.CT * 1E-6)
	prop   = (args.L * 1E-3) / (2 * 96485333.7 * args.V)**0.5

	try:
		inf   = open(args.clusters, 'r')
		files = sum([line.rstrip().split(" ") for line in inf],[])
	except IOError:
		print "error opening ", args.clusters
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
		for RT, scan in spectrum.copy().iteritems():
			if len(scan) < args.min_len or max(scan.values()) < args.min_I:
				del spectrum[RT]
		if len(spectrum) >= args.min_scans:
			spectra.append(spectrum)

	# Constrained COBYLA 
	# cons = [(lambda dt,pu,pr,sp: dt[1]), (lambda dt,pu,pr,sp: dt[0]-dt[1])]
	# dt, c = opt.fmin_cobyla(total_curv, dt0, cons, args=(pulses, prop, spectra))
	# print "COBYLA "
	#print "Non-extending DT: %.3f ns" % dt
	#print "    Extending DT: %.3f ns" % c
	#print


	# Brute force search
	if 0:
		dt = opt.brute(total_curv, ((0,2*dt0[0]),(0,2*dt0[1])), Ns=100, 
			       args=(pulses, prop, spectra))

		print "brute force search "
		print "Non-extending DT: %.3f ns" % dt[0]
		print "    Extending DT: %.3f ns" % dt[1]
		print


	# Show total curvatures around initial guess
	#for ext in np.linspace(0, 3*args.ext, 21):
	#	for non in np.linspace(0, 2*args.non, 11):
	#		val = total_curv([ext,non], pulses, prop, spectra)
	#		print "%.1f %.1f %f" % (ext, non, val)
	#exit (1)

	# Simulated annealing
	blah = "dummy"
	parameter_names = [" non extending", "  extending", "  pulses", "  prop"]
	for repeat in range(10):

		parameters_initial = np.array([args.non, args.ext, pulses, prop])
		lower_bounds=np.array([3.0, 0.5, pulses*0.8, prop*0.9])
		upper_bounds=np.array([7.0, 3.0, pulses*1.1, prop*1.1])
		optimized_params, min_val, T, feval, iters, accept, retval = \
		    anneal (total_curv, parameters_initial, args= (blah, spectra), 
				full_output=True, lower=lower_bounds, upper=upper_bounds)


		print "repeat ", repeat
		print " names  ", parameter_names
		print " lower  ", lower_bounds
		print " upper  ", upper_bounds
		print " init   ", parameters_initial
		print " opt    ", optimized_params 
		print "    minimum:      %.5f "   % min_val

	#print "Command line: deadtime -d%.3f  -D%.3f  <mzXML>" % (dt[0], dt[1])
	#print


	return 0

if __name__ == '__main__':
	main()
