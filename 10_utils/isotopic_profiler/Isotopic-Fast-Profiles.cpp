/*	Isotopic-Fast-Profiles.cpp
 * Program to find the isotopic percentages and normalized
 * weights given a list of sequences.  The program also finds the level of
 * fast exchanging side chain deuteration profile for a given percent 
 * deuteration level inputed by the user.  The program first 
 * check to see if the user has supplied a file to read the
 * abundances and masses of elements.  If there is no file supplied
 * then it uses a standard set of H1,C13,N15,O17,O18,S33,S34.  
 * It prints out to stderr the masses and abundances of these
 * types.  So the user can see if they are appropriate. 
 * If the user has supplied a file, it attempts to read the file
 * and store that atom types and their masses, abundances and offsets.
 * It then prints this out so the user can see that it was read in 
 * properely.  
 * The main program then reads in the file containing the list of
 * sequences.  For each sequence, the program will print out the
 * sequence to the newfile and determine the length, residue and 
 * elemental compostition of that sequence.  All peptides are assumed
 * to be M/Z=+1, so there is 1 Oxygen and 3 Nitrogens associated with
 * the N and C terminus and all side chains are assumed to be neutral.
 * The monoisotopic mass is found and all the element isotope structures
 * are given a value based on the elemental composition.  
 * 
 * The probability is then found for each M/Z value, assuming they are
 * integer values.  They are all recorded and the greatest one is found
 * and used for nomalization.  Once all the probabilities are found, 
 * they are printed to the newfile with an 1.index, 2.M/Z,3.% probability
 * and 4.% normalized probability. The probability is found for each M/Z
 * where % probabilility > .005 and one extra for each sequence in the file. 
 *
 * The user can choose to print out just the isotopic abundance profile, the 
 * fast exchanging side chain profile, or both combined together.  For MALDI-TOF
 * both are usually used, but ESI-TOF usually only has the isotopic abundance.  
 *
 * Please see reference in the journal Protein Science:
 * Hotchko, M., Anand, G. S., Komives, E.A. and Ten Eyck, T. F. 2005.  Automated extraction of
 * backbone deuteration levels from amide H/2H mass spectrometry experiments.
 *
 * Probability algorithm by Vineet Bafna at UC, San Diego
 * Created by Matthew Hotchko, 03/26/04
 * Last updated on 04/07/04
 * Complile by typing:
 * g++ Isotopic-Fast-Profiles.cpp -o isotopic-fast-profiles -Wno-deprecated
 */

#if 0
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <iostream.h>
#include <iomanip.h>
#include <fstream.h>

# else
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <iostream>
# endif

using namespace std;


    FILE     *mfile,*abund_file;
    char     residue[256],sequence[256],ctmp[2],buf[256],isofile_name[256];
    int      i,length,seq_num,offset_temp,store_ii,type_binomial,charge;
    int      A,C,D,E,F,G,H,I,K,L,M,N,P,Q,R,S,T,V,W,Y;
    int     Carbon,Nitrogen,Oxygen,Sulfur,Hydrogen,n,k,ii,jj,kk,isotope_count;
    int     side_deut;
    size_t   nch;
    double   combos,prob,probability;
    char     temp_char;
    float    mass,p_temp,mass_ele_temp,prob_out,prob_max,prob_store[30],mass_temp;
    float    sum_profile[30],percent_deut,side_percent[30],profile_max,side_max;
    long double fin_prob=0;

typedef struct isotope {
		char	element[16];	/*element name*/
		int	offset;		/*offset from main isotope*/
		float 	p;		/*abundance of this isotope*/
		float	mass_ele;	/*mass of main isotope*/
		long    num_type;
		} ISOTOPE, *ISOTOPE_PTR;

/******************************************************alloc_isotope***********/
ISOTOPE_PTR alloc_isotope(char * char_temp, int offset_temp, float p_temp, float mass_ele_temp)
{
/*
	* alloc_isotope subroutine will allocate by calloc the isotope struture
	* and initialize the values of element, char, probability and mass.  
	* It then passes to the main function a isotope pointer so the main
	* can grab ahold of the data.  
*/
    ISOTOPE_PTR   at_tpe;

    if ((at_tpe= (ISOTOPE_PTR)calloc(1,sizeof(ISOTOPE))) == (ISOTOPE_PTR)NULL) {
	fprintf(stderr,"Cannont allocate another ISOTOPE structure.\n");
	exit(2); }
    strncpy(at_tpe->element,char_temp,15);
    at_tpe->offset = offset_temp;
    at_tpe->p = p_temp;
    at_tpe->mass_ele = mass_ele_temp;
    return(at_tpe);
}
/******************************************************combination*************/
long double combination(long n, long k) {	//works just fine, need to comment
/*
	* combination will take in n, which is the number of that type of element
	* and k, which is how many to chose from that element and find the
	* number of ways the combination is possible.  It returns the number
	* of ways that are possible to the function that called it.  combination
	* is called from binomial.  
*/
    long double comb =1.0;
    for (int jj=0;jj<k;jj++) {
	comb= comb*(n-jj)/(k-jj);
	}
    //cerr<<"  after comb="<<comb<<endl;
    return((long double)comb);
}
/******************************************************binomial****************/
long double binomial(long n, long k, double p) { //works just fine, need to comment
/*
	* binomial will take in n, which is th number of that type of element and
	* k, which is how many to choose from that element and p, which is the 
	* probability of that element.  It calls combination to get the number
	* of occurances of that binomial expression and multiplies that by the
	* power of p^k and (1-p)^(n-k) and returns this value.  It is called from
	* compute_probability. 
*/
    if (k > n) {
	return 0.0; }
    else {
    prob= combination(n,k)* pow(p,k)*pow(1-p,n-k);
    }
    return((long double) prob);
}


/******************************************************binomial****************/
long double compute_probability (ISOTOPE_PTR *at_tpe, long isotope_count, long L, long k)
{
/*
	* compute_probability is the function that actually finds the probability of
	* of a specific M/Z value.  It is called in the main function for each M/Z 
	* and returns the probability.  It takes in the pointer to the atom_types
	* to access the data in them, the number of elements (isotope_count), the
	* number L which points to the specific atom_type present and k which is
	* the number of masses above the monoisotopic value.  
	*
	* It is a recursive program that goes through
	* each element in the list and finds its binomial probability and multiplies
	* that by the probability of the rest of the elements that have the right M/Z, 
	* using a recursive call to compute_probability.  When the list is empty, 
	* it will return either 1 if ther is no more isotopes to worry about or
	* 0 if that line of recursions will not equal M/Z specified.  compute_
	* probabilty calls binomial to get the expression of a specific element.
*/
    double prob=0; double bin_prob,recur_prob;
    if (L == isotope_count) {
	if ( k == 0) {
	  return 1.0; }
	else {
	  return 0; }
	}
    for (int i=0;i<=k;i++) {
      if ((i % at_tpe[L]->offset) == 0) {
	if (at_tpe[L]->offset > 1) {
	  recur_prob=compute_probability(at_tpe,isotope_count,L+1,k-i);
	  bin_prob= binomial(at_tpe[L]->num_type,i/2,at_tpe[L]->p);
	  }
	else {
	  recur_prob=compute_probability(at_tpe,isotope_count,L+1,k-i);
	  bin_prob= binomial(at_tpe[L]->num_type,i,at_tpe[L]->p);
	  }
	double tmp_prob= bin_prob*recur_prob; 
	prob += tmp_prob;
	/*if (L==0) {
	  fprintf(stderr," i=%d  L=%d  k=%d  \n",i,L,k);
	  cerr<<"tmp_prob="<<tmp_prob;
	  cerr << "  bin_prob="<<bin_prob;
	  cerr << "  recur_prob=" << recur_prob;
	  cerr << "  tot_prob=" << prob << "\n";
	}  */
	if (i >k) {
	  cerr<<"Need to break here i:"<< i << endl;
	  }
        }
      }
    return(prob);
}
/******************************************************main*program************/
int main(int argc, char *argv[])
/*
	* main program is where all the file I/O and checks occur.  It also is
	* isotopes are read in from the file or assigned to the standards.  
	* main also reads in the sequence file and find the amino acids present
	* and find the elemental composition of each of these values.  Finally,
	* main has a loop to find all the isotopic masses and their distributions
	* for each peptide.  main then prints to newfile the indes, mass, percent
	* of total for each M/Z and normalized percent total in 4 columns for
	* each peptide.  
*/
{
    ISOTOPE_PTR	atom_type[100];

    if (argc < 5) {
	fprintf(stdout,"\nUsage: %s   sequence-file   new-file   ",argv[0]);
	fprintf(stdout,"percent-deuteration (isotopic=1/fast=2/both=3) ");
        fprintf(stdout,"element/abundance-file(optional)\n\n");
	fprintf(stdout,"Where\n 1 = sequence file in CAPITALS of single letter ");
	fprintf(stdout,"amino acid sequences with m/z charge afterwards (optional)\n");
	fprintf(stdout," 2 = new file name\n");
	fprintf(stdout," 3 = percent deuteration for side chain exchange ");
	fprintf(stdout,"(any number if not choosing fast exchange or both)\n");
	fprintf(stdout," 4 = Choose \"1 2 3\" (isotopic distribution=1)  ");
	fprintf(stdout,"(fast exchange=2)  (both=3)\n");
	fprintf(stdout," 5 = abundance-file for isotopic abundances (optional)\n\n");

	return(2);
    	}
    if ((mfile = fopen(argv[1], "r")) == (FILE *)NULL) {
	fprintf(stderr,"Cannot open file %s for reading models\n", argv[1]);
	return(2);
    	}
    percent_deut=atof(argv[3]);
    if (percent_deut >99.9 || percent_deut < 0) {
	percent_deut = 5;
	fprintf(stdout,"\npercent_deut is assigned to 5 because unrealistic value was chosen");
	}
    fprintf(stdout,"\npercent_deut is assigned to = %5.2f\n",percent_deut);
    type_binomial=atoi(argv[4]);
    if (type_binomial==1) {
        fprintf(stdout,"Only isotopic profile will be calculated\n\n"); }
    if (type_binomial==2) {
        fprintf(stdout,"Only fast exchanging sidechain profile will be calculated\n\n"); }
    if (type_binomial==3) {
        fprintf(stdout,"Both isotopic and fast exchanging profiles will be calculated together\n\n"); }
    ii=0;	/*assign ii=0 so can count isotope_counts*/
    if ((abund_file = fopen(argv[5], "r")) == (FILE *)NULL) {
	fprintf(stdout,"No element/abundance-file was chosen:");
	fprintf(stdout,"  Will use standards elements.\n");
	fprintf(stdout,"1H	2H	12C	13C\n");
	fprintf(stdout,"14N	15N	16O	17O	18O\n");
	fprintf(stdout,"32S	33S	34S\n\n");
	/*assign standard elements here*/
	atom_type[ii] = alloc_isotope("H",1,.000150,1.007825); ii++;
	atom_type[ii] = alloc_isotope("C",1,.011070,12.00000); ii++;
	atom_type[ii] = alloc_isotope("N",1,.003663,14.00307); ii++;
	atom_type[ii] = alloc_isotope("O",1,.000375,15.99491); ii++;
	atom_type[ii] = alloc_isotope("O",2,.002035,15.99491); ii++;
	atom_type[ii] = alloc_isotope("S",1,.00760,31.97207); ii++;
	atom_type[ii] = alloc_isotope("S",2,.04290,31.97207); ii++;
	//atom_type[ii] = alloc_isotope("S",4,.00200,31.97207); ii++;
	isotope_count=ii;
	for (i=0;i<isotope_count;i++) {
	  fprintf(stdout,"atom_type[%2d]->element=%s  ",i,atom_type[i]->element);
	  fprintf(stdout,"offset=%d  ",atom_type[i]->offset);
	  fprintf(stdout,"p=%f  ",atom_type[i]->p);
	  fprintf(stdout,"mass_ele=%f\n",atom_type[i]->mass_ele);
	  }
	fprintf(stdout,"Isotope_count= %d isotope types in standard set.\n",isotope_count);
    	}
    else {		//Will read in the file for isotopes
        ii=0;	/*assign ii=0 so can count isotope_counts*/
	fprintf(stdout,"Using list from file %s\n",argv[5]);
	while ((fgets(buf,sizeof(buf),abund_file)) != (char *)NULL) {
	  //fprintf(stdout,"%s",buf);
	  if (sscanf(buf,"%s %d %f %f", &temp_char, &offset_temp, &p_temp,
	      &mass_ele_temp) == 4) {
	    if (offset_temp >=1) {
	      atom_type[ii] = alloc_isotope(&temp_char,offset_temp,p_temp,mass_ele_temp);
	      fprintf(stdout,"atom_type[%2d]->element=%s  ",ii,atom_type[ii]->element);
	      fprintf(stdout,"  offset=%d  ",atom_type[ii]->offset);
	      fprintf(stdout,"p=%f  ",atom_type[ii]->p);
	      fprintf(stdout,"mass_ele=%f\n",atom_type[ii]->mass_ele);
	      ii++;	/*increment ii so next list is given*/
	      if (ii>=99) {
		fprintf(stdout,"Problem:  Only 100 isotope types are allowed.\n");
		exit(2); }
	      }
	    }
	  }
	isotope_count=ii;
	fprintf(stdout,"Isotope_count= %d isotope types in file %s\n",isotope_count,argv[5]);
	}

    seq_num=0;
    fprintf(stdout,"\nNOTE:  Mass/Charge= +1 is assumed for all peptides unless ");
    fprintf(stdout,"the charge is specified after the sequence.\n");
    fprintf(stdout,"       All side chains are assumed to have neutral charge.\n\n");
    FILE *isofile = fopen(argv[2],"w");
    strcpy(isofile_name,argv[2]);
    while ((fgets(buf, sizeof(buf), mfile) != (char *)NULL)) {
      charge=1;			//assign the m/z= +1 for the standard value
      if ((sscanf(buf,"%s %d",sequence,&charge) >0)||(sscanf(buf,"%1s",sequence) >0)) {
	/*have one sequence to analyze*/
	A=C=D=E=F=G=H=I=K=L=M=N=P=Q=R=S=T=V=W=Y=0;
	Carbon=Nitrogen=Oxygen=Sulfur=Hydrogen=0;
	mass=0;
        nch = strcspn(sequence, "\n");
        sequence[nch] = '\0';
        nch = strcspn(sequence, " ");
        sequence[nch] = '\0';
	seq_num++;
	if (charge <0 || charge > 7) {
	  charge=1;
	  fprintf(stdout,"You have not chosen a reasonable charge value, m/z= +1 will be used.\n");
	  }
	fprintf(stdout,"%3d Sequence= %s  ",seq_num,sequence);
	fprintf(isofile, "%s  %d\n",sequence,charge);
	length=strlen(sequence);
	fprintf(stdout,"  length=%d  charge=%d  ",length,charge);
	Oxygen=1;	/*C-terminus Oxygen*/
	Hydrogen=3;	/*1 C-terminus Hydrogen and 2 N-terminus Hydrogens*/
	side_deut=4;	/*3 for N terminus and 1 for C terminus*/
	//side_deut=0;	//Choose a number to investigate
	for (ii=0;ii<length;ii++) {
	  //cout<<"residue "<<ii<<" is "<<sequence[ii]<<endl;
	  switch (sequence[ii]) {
		case 'A':
		    A++;
		    Carbon=Carbon+3; Hydrogen=Hydrogen+5;
		    Nitrogen++; Oxygen++;
		    break;
		case 'C':
		    C++;
		    Carbon=Carbon+3; Hydrogen=Hydrogen+5;
		    Nitrogen++; Oxygen++; Sulfur++;
		    side_deut++;
		    break;
		case 'D':
		    D++;
		    Carbon=Carbon+4; Hydrogen=Hydrogen+5;
		    Nitrogen++; Oxygen=Oxygen+3;
		    side_deut++;
		    break;
		case 'E':
		    E++;
		    Carbon=Carbon+5; Hydrogen=Hydrogen+7;
		    Nitrogen++; Oxygen=Oxygen+3;
		    side_deut++;
		    break;
		case 'F':
		    F++;
		    Carbon=Carbon+9; Hydrogen=Hydrogen+9;
		    Nitrogen++; Oxygen++;
		    break;
		case 'G':
		    G++;
		    Carbon=Carbon+2; Hydrogen=Hydrogen+3;
		    Nitrogen++; Oxygen++;
		    break;
		case 'H':
		    H++;
		    Carbon=Carbon+6; Hydrogen=Hydrogen+7;
		    Nitrogen=Nitrogen+3; Oxygen++;
		    side_deut++;
		    break;
		case 'I':
		    I++;
		    Carbon=Carbon+6; Hydrogen=Hydrogen+11;
		    Nitrogen++; Oxygen++;
		    break;
		case 'K':
		    K++;
		    Carbon=Carbon+6; Hydrogen=Hydrogen+12;
		    Nitrogen=Nitrogen+2; Oxygen++;
		    side_deut=side_deut +2;
		    break;
		case 'L':
		    L++;
		    Carbon=Carbon+6; Hydrogen=Hydrogen+11;
		    Nitrogen++; Oxygen++;
		    break;
		case 'M':
		    M++;
		    Carbon=Carbon+5; Hydrogen=Hydrogen+9;
		    Nitrogen++; Oxygen++; Sulfur++;
		    break;
		case 'N':
		    N++;
		    Carbon=Carbon+4; Hydrogen=Hydrogen+6;
		    Nitrogen=Nitrogen+2; Oxygen=Oxygen+2;
		    side_deut=side_deut +2;
		    break;
		case 'P':
		    P++;
		    Carbon=Carbon+5; Hydrogen=Hydrogen+7;
		    Nitrogen++; Oxygen++;
		    break;
		case 'Q':
		    Q++;
		    Carbon=Carbon+5; Hydrogen=Hydrogen+8;
		    Nitrogen=Nitrogen+2; Oxygen=Oxygen+2;
		    side_deut=side_deut +2;
		    break;
		case 'R':
		    R++;
		    Carbon=Carbon+6; Hydrogen=Hydrogen+12;
		    Nitrogen=Nitrogen+4; Oxygen++;
		    side_deut=side_deut +4;
		    break;
		case 'S':
		    S++;
		    Carbon=Carbon+3; Hydrogen=Hydrogen+5;
		    Nitrogen++; Oxygen=Oxygen+2;
		    side_deut++;
		    break;
		case 'T':
		    T++;
		    Carbon=Carbon+4; Hydrogen=Hydrogen+7;
		    Nitrogen++; Oxygen=Oxygen+2;
		    side_deut++;
		    break;
		case 'V':
		    V++;
		    Carbon=Carbon+5; Hydrogen=Hydrogen+9;
		    Nitrogen++; Oxygen++;
		    break;
		case 'W':
		    W++;
		    Carbon=Carbon+11; Hydrogen=Hydrogen+10;
		    Nitrogen=Nitrogen+2; Oxygen++;
		    side_deut++;
		    break;
		case 'Y':
		    Y++;
		    Carbon=Carbon+9; Hydrogen=Hydrogen+9;
		    Nitrogen++; Oxygen=Oxygen+2;
		    side_deut++;
		    break;
		default:
		    cout<<"Do not recognize residue "<<sequence[ii]<<endl;
		    fprintf(stdout, "Do not recognize residue %s\n",residue);
		    break;
	    }	/*end of switch loop for deterimining which residue current one is*/
	  }	/*end of for loop to go over each residue in each sequence*/
	fprintf(stdout,"C= %d  H=%d  ",Carbon,Hydrogen);
	fprintf(stdout,"N= %d  O=%d  S=%d  ",Nitrogen,Oxygen,Sulfur);
	mass=Carbon*12 + Hydrogen*1.007825 + Nitrogen*14.00307 + Oxygen*15.99491
        + Sulfur*31.9720707;
	fprintf(stdout,"monoisotopic_mass= %f\n",mass);
	/*assign number of atoms in peptide to elemental isotope types*/
	for (jj=0;jj<isotope_count;jj++) {
	    if (strcmp("H", atom_type[jj]->element) == 0 ) {
	      atom_type[jj]->num_type = Hydrogen;
	      }
	    if (strcmp("C", atom_type[jj]->element) == 0 ) {
	      atom_type[jj]->num_type = Carbon;
	      }
	    if (strcmp("N", atom_type[jj]->element) == 0 ) {
	      atom_type[jj]->num_type = Nitrogen;
	      }
	    if (strcmp("O", atom_type[jj]->element) == 0 ) {
	      atom_type[jj]->num_type = Oxygen;
	      }
	    if (strcmp("S", atom_type[jj]->element) == 0 ) {
	      atom_type[jj]->num_type = Sulfur;
	      }
	    /*add more if statments if more elements are desired*/
	    }		/*end of for jj loop to assign num_type to each isotope*/
	//fprintf(stdout,"mass1= %10.4f\n",mass);
	/*Here must compute the isotopic abundances*/
	prob_max =0; profile_max=side_max=0;
	for (ii=0;ii<30;ii++) {
	    probability = compute_probability(&atom_type[0],isotope_count,0,ii);
	    prob_out= (float) probability*100;
	    prob_store[ii]=prob_out;
	    if (prob_out < .005) {
		store_ii=ii;
		break; }
	    if (prob_out > prob_max) {
		prob_max=prob_out; }
	    //cerr<<"in main ii="<<ii<<" probability="<<probability*100<<endl;
	  }	/*end of jj loop over each mass weight to calculate*/
	fprintf(stdout,"store_ii=%d  prob_max=%5.2f\n",store_ii,prob_max);
	for (ii=0;ii<store_ii+1;ii++) {
	    mass_temp=mass+ ii*1.002782;	//addition of an extra Carbon atom
	    //fprintf(isofile,"%d %10.4f",ii,mass_temp);
	    //fprintf(isofile," %5.2f",prob_store[ii]);
	    //fprintf(isofile,"  %5.2f\n",prob_store[ii]/prob_max*100);
	  if (type_binomial==1) {	//if wish to print out the fast exchanging only
	    fprintf(stdout,"isotopic profile[%d] = %7.4f\n",ii,prob_store[ii]);
	    }
	  }
	//fprintf(isofile,"\n");

	//Add fast-exchanging profiles here
	fprintf(stdout,"side_deut= %d \n",side_deut);
	for (ii=0;ii<store_ii+1;ii++) {
	  side_percent[ii]= (float) combination(side_deut,ii);
	  side_percent[ii]=side_percent[ii]*pow((1-percent_deut/100),(side_deut-ii));
	  side_percent[ii]=side_percent[ii]*pow((percent_deut/100),ii)*100;
	  if (side_percent[ii] > side_max) {
	    side_max = side_percent[ii]; }
	  if (type_binomial==2) {	//if wish to print out the fast exchanging only
	    fprintf(stdout,"side_percent[%d] = %7.4f\n",ii,side_percent[ii]);
	    }
	  }

	//Calculate the sum of the isotopic and fast-exchanging profiles here
	for (ii=0;ii<store_ii+1;ii++) {
	  sum_profile[ii]=0;
	  if (type_binomial==3) {	//if wish to print out both combined
	    fprintf(stdout,"ii=%d  ",ii); }
	  for (jj=0;jj<=ii;jj++) {
	    for (kk=ii;kk>=0;kk--) {
	      if (jj + kk == ii) {
	  	sum_profile[ii]=sum_profile[ii] + prob_store[jj]*side_percent[kk]/100;
		}
	      }
	    }
	  if (type_binomial==3) {	//if wish to print out both combined
	    fprintf(stdout,"  sum_profile[%d]=%5.3f\n",ii,sum_profile[ii]);
	    }
	  if (sum_profile[ii] > profile_max) {
	    profile_max = sum_profile[ii]; }
	  }
	for (ii=0;ii<store_ii+1;ii++) {
	  //mass_temp=mass+ ii*1.002782;
	  mass_temp=mass+ ii*1.003355;		//This is the increased mass from 13C over 12C
	  if (charge != 1) {
	    mass_temp= (mass + ii*1.003355 + charge -1)/charge;
	    }
	  fprintf(isofile,"%d %10.4f",ii,mass_temp);
	  if (type_binomial==1) {	//if wish to print out the fast exchanging only
	    fprintf(isofile," %5.2f",prob_store[ii]);
	    fprintf(isofile,"  %5.2f\n",prob_store[ii]/prob_max*100);
	    }
	  else if (type_binomial==2) {	//if wish to print out the fast exchanging only
	    fprintf(isofile," %5.2f",side_percent[ii]);
	    fprintf(isofile,"  %5.2f\n",side_percent[ii]/side_max*100);
	    }
	  else {
	    fprintf(isofile," %5.2f",sum_profile[ii]);
	    fprintf(isofile,"  %5.2f\n",sum_profile[ii]/profile_max*100);
	    }
	  }
	fprintf(isofile,"\n");
	}	/*end of if not and empty line*/
      }		/*end of while fgets a line loop*/

    fprintf(stdout,"\nThe output distribution file is called:  %s\n",isofile_name);

    return 0;
}

