#!/usr/bin/python

from scipy.optimize import leastsq
import numpy as np
import sys
from Bio.PDB.PDBParser import PDBParser
import detector


def set_init_param(x,y,z):
    p0=[x[0], x[-1]-x[0], 0, 0, y[0], y[-1]-y[0], 0, 0, z[0], z[-1]-z[0], 0, 0]
    return p0

def der1(x, t):
    return x[1]+2*x[2]*t+3*x[3]*t**2

def der2(x,t):
    return 2*x[2]+6*x[3]*t
    
def der3(x,t):
    return 6*x[3]

# calculation of:
#   x'''(y'z''-y''z')
#
# x(a,t) = a[0] + a[1]*t +a[2]*t**2 + a[3]*t**3
# y(b,t) = b[0] + b[1]*t +b[2]*t**2 + b[3]*t**3
# z(b,t) = c[0] + c[1]*t +c[2]*t**2 + c[3]*t**3

def expr1(a,b,c,t):
    return der3(a,t)*expr2(b,c,t)

# calculation of:
#   (x'y''-x''y')
#
# x(a,t) = a[0] + a[1]*t +a[2]*t**2 + a[3]*t**3
# y(b,t) = b[0] + b[1]*t +b[2]*t**2 + b[3]*t**3

def expr2(a,b,t):
    return der1(a,t)*der2(b,t)-der2(a,t)*der1(b,t)

# The calculation of torsion of a space curve
# T = (x'''(y'z''-y''z')+y'''(x''z-x'z'')+z'''(x'y''-x''y'))/((y'z''-y''z')+(x''z-x'z'')+(x'y''-x''y'))

def torsion(a,b,c,t):
    num = expr1(a,b,c,t)+expr1(b,c,a,t)+expr1(c,a,b,t)
    denum = expr2(b,c,t)**2+expr2(c,a,t)**2+expr2(a,b,t)**2
    return num/denum

# The calculation of curvature of a space curve
# K=(((z''y'-y''z')**2 + (x''z'-z''x')**2 + (y''x'-x''y')**2)**0.5)/(x'**2+y'**2+z'**2)**1.5
# 
    
def curvature(a,b,c,t):
    num = (expr2(b,c,t)**2+expr2(c,a,t)**2+expr2(a,b,t)**2)**0.5
    denum = (der1(a,t)**2+der1(b,t)**2+der1(c,t)**2)**1.5
    return num/denum

def residuals(p, X, Y, Z, t):
    a0, a1, a2, a3, b0, b1, b2, b3, c0, c1, c2, c3 = p
    f1 = X - (a0  + a1 * t + a2 * t**2 + a3 * t**3)
    f2 = Y - (b0  + b1 * t + b2 * t**2 + b3 * t**3)
    f3 = Z - (c0  + c1 * t + c2 * t**2 + c3 * t**3)
    err = np.concatenate((f1,f2,f3))
    return err
    
def optimize (x,y,z):
    np.set_printoptions(precision=3)
    np.set_printoptions(suppress=True)

    N = len(x)
    t = np.linspace(0,1,N)
    
#    x = [i-10 for i in x]
    
    p0 = set_init_param(x, y, z)
    param = leastsq(residuals, p0, args=(x, y, z, t))
    a = param[0][0:4]
    b = param[0][4:8]
    c = param[0][8:12]
    
#    print p0,
#    print a, 
#    print b, 
#    print c
#    print param,
#    print "\n"
#    
#    for item in a:
#        print "%8.4f" % item,
#    print ""
#    for item in b:
#        print "%8.4f" % item,
#    print ""
#    for item in c:
#        print "%8.4f" % item,
#    print ""
    
    curv = curvature(a,b,c,t)
    tors = torsion(a,b,c,t)
    
    
#    print curv
#    print tors
#    print "*****************************"
#    
    return curv, tors
    
def main():
	if len(sys.argv) < 2:
	    sys.exit('Usage: %s input_pdb_file' % sys.argv[0])
	pdb_name = sys.argv[1]
	parser=PDBParser(PERMISSIVE=1)
	structure_id = "temp"
	structure = parser.get_structure(structure_id, pdb_name)
	model = structure[0]

	for chain in model.get_list():
	    coord_list = detector.determine_ca_coordinates(chain)
        x= [0 for i in range(5)]
        y= [0 for i in range(5)]
        z= [0 for i in range(5)]
        for i in range(len(coord_list)-2):
            res_id = coord_list[i].get_full_id()[3][1]
            print res_id + 2
            if (i + 4 > len(coord_list) -1): # last element in list
                continue
            for j in range(5):
                x[j] = coord_list[i+j].get_coord()[0]
                y[j] = coord_list[i+j].get_coord()[1]
                z[j] = coord_list[i+j].get_coord()[2]

            curv, tors = optimize (x,y,z)

if __name__ == "__main__":
    main()

