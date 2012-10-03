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
