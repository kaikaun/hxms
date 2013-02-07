from math import sqrt, log, exp

def correct_deadtime(non_ext, extending,  pulses, prop, spectrum):

	flight_time = lambda mz: prop * sqrt(mz)
	new_spectrum = {}
	for RT, scan in spectrum.iteritems():

		new_scan = {}

		for mz in sorted(scan.iterkeys()):
			pulses_not_hit = pulses
			pulses_blocked = 0
			FT = flight_time(mz)

			for mz2 in sorted(new_scan.iterkeys(), reverse=True):

				FT_diff = FT - flight_time(mz2)
				if FT_diff > non_ext*1e-9:
					break
				if FT_diff > extending*1e-9:
					pulses_not_hit -= scan[mz2]
				else:
					pulses_blocked += new_scan[mz2]

			new_I = scan[mz] * exp(pulses_blocked/pulses) / pulses_not_hit
			if new_I > 1:
				#except_msg = "Log underflow due to invalid peak values"
				#print pulses_blocked
				#print pulses
				#print pulses_not_hit
				#print except_msg
				#raise ValueError, except_msg
				return spectrum # try something else
			new_I        = -log(1 - new_I) * pulses
			new_scan[mz] =  new_I
			#print "   %8.2f   %8.4f  %8.4f " % (mz, scan[mz], new_scan[mz])
		new_spectrum[RT] = new_scan





	return new_spectrum
