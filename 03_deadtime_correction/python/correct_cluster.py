#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# optimize_dt.py

import argparse
import os
import re
from TOFMS_deadtime import correct_deadtime

def main():
	parser = argparse.ArgumentParser(description='Correct clusters for deadtime.')
	parser.add_argument('clusters', 
						help='file containing cluster filenames')
	parser.add_argument('outputdir', 
						help='directory to write corrected clusters to')
	parser.add_argument('-d','--ext', dest='ext', type=float, default=5., 
						help='extending deadtime (ns)')
	parser.add_argument('-D','--non', dest='non', type=float, default=0., 
						help='non-extending deadtime (ns)')
	parser.add_argument('-s','--scantime', dest='ST', type=float, default=0.23, 
						help='scan time (s)')
	parser.add_argument('-c','--cycletime', dest='CT', type=float, default=45., 
						help='cycletime (μs)')
	parser.add_argument('-l','--length', dest='L', type=float, default=1078., 
						help='effective TOF path length (mm)')
	parser.add_argument('-v','--voltage', dest='V', type=float, default=5630., 
						help='effective accelerating voltage (V)')
	args = parser.parse_args()

	dt = [args.ext,args.non]
	pulses = args.ST / (args.CT * 1E-6)
	prop = (args.L * 1E-3) / (2 * 96485333.7 * args.V)**0.5

	try:
		inf =  open(args.clusters, 'r')
		files = [line.split()[0] for line in inf]
	except IOError:
		print "error opening ", args.clusters
		return -1

	try:
		os.mkdir(args.outputdir)
	except OSError:
		print args.outputdir, " already exists"
		return -1

	lre = re.compile('(\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)\s+(\d+\.\d+)')
	for file in files:
		spectrum={}
		scannums={}
		try:
			f = open(file, 'r') 
			for line in f:
				res = lre.search(line)
				if res:
					RT = float(res.group(2))
					if RT not in spectrum:
						spectrum[RT] = {}
					spectrum[RT][float(res.group(3))] = float(res.group(4))
					scannums[RT] = res.group(1)
		except IOError:
			next
		spectrum = correct_deadtime(dt,pulses,prop,spectrum)
		outputfilename = os.path.join(args.outputdir, os.path.basename(file))
		f =  open(outputfilename, 'w')
		for RT, scan in sorted(spectrum.iteritems()):
			for mz, I in sorted(scan.iteritems()):
				f.write("%s %.3f %.3f %.3f\n" % (scannums[RT],RT,mz,I))

	return 0

if __name__ == '__main__':
	main()
