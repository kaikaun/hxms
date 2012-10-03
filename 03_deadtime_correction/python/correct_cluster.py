#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# optimize_dt.py

import argparse
import glob
from os.path import basename
import re
import math

def correct_deadtime(dt, pulses, prop, spectrum):
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
				return -2
			new_I = -math.log(1 - new_I) * pulses
			new_scan[mz] = new_I
		new_spectrum[RT] = new_scan
	return new_spectrum

def main():
	parser = argparse.ArgumentParser(description='Correct clusters for deadtime.')
	parser.add_argument('clusters', metavar='C', nargs='+',
						help='cluster filenames (including wildcards)')
	parser.add_argument('-d','--ext', dest='ext', type=float, default=5., 
						help='extending deadtime (ns)')
	parser.add_argument('-D','--non', dest='non', type=float, default=0., 
						help='non-extending deadtime (ns)')
	parser.add_argument('-s','--scantime', dest='ST', type=float, default=0.23, 
						help='scan time (s)')
	parser.add_argument('-c','--cycletime', dest='CT', type=float, default=45, 
						help='cycletime (μs)')
	parser.add_argument('-l','--length', dest='L', type=float, default=1078, 
						help='effective TOF path length (mm)')
	parser.add_argument('-v','--voltage', dest='V', type=float, default=5630, 
						help='effective accelerating voltage (V)')
	args = parser.parse_args()

	dt = [args.ext,args.non]
	pulses = args.ST / (args.CT * 1E-6)
	prop = (args.L * 1E-3) / math.sqrt(2 * 96485333.7 * args.V)

	files = [glob.glob(g) for g in args.clusters]
	files = [f for f in sum(files,[]) if re.match('\d{6}\.clust$', basename(f))]
	if not files:
		print 'No valid cluster files found!'
		return -1

	lre = re.compile('(\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)')
	for file in files:
		spectrum={}
		scannums={}
		f = open(file, 'r')
		for line in f:
			res = lre.search(line)
			if res:
				RT = float(res.group(2))
				if RT not in spectrum:
					spectrum[RT] = {}
				spectrum[RT][float(res.group(3))] = float(res.group(4))
				scannums[RT] = res.group(1)
		f.close()
		spectrum = correct_deadtime(dt,pulses,prop,spectrum)
		f = open(file + '.corr', 'w')
		for RT, scan in sorted(spectrum.iteritems()):
			for mz, I in sorted(scan.iteritems()):
				f.write("%s %.3f %.3f %.3f\n" % (scannums[RT],RT,mz,I))
		f.close()

	return 0

if __name__ == '__main__':
	main()
