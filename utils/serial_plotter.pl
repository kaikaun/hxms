#! /usr/bin/perl -w
# to get the input:
#  wc -l * | sort -gk1 | tail -n 50 | tee input
@ARGV ||
    die "Usage:  $0  <input file name> \n";

$filename = $ARGV[0];
open (IF, "<$filename" ) 
    || die "Cno $filename: $!.\n";

($size, $name) = ();
while ( <IF> ) {
    chomp;
    ($size, $name) = split;
    (-e $name) || next;
    ($name =~ /clust/ ) || next;

    $clust_number = $name;
    $clust_number =~ s/\.clust//;
    open (OF, ">blah.gscr") || 
	die "CNo blah.gscr.\n";
    print OF "unset key \n";
    print OF "set pm3d map \n";
    print OF 'set format y "%10.3f'."\n";
    print OF "set xlabel \"SCAN NUMBER\"\n";
    print OF "set ylabel \"M/Z\" \n";
    print OF "set title \"CLUSTER $clust_number\" \n";
    print OF 'set palette  defined (0 "white", 300 "blue", 1000 "red", 1200 "yellow")'."\n";
    print OF "set term post color\n";
    print OF "set out '$clust_number.post'\n";
    print OF "splot '$name' using 1:3:4 with points ps 1 pt 7 palette\n";
    close OF;
   
    $cmd = "gnuplot blah.gscr";
    if (system $cmd ) {
	warn "Error runnning $cmd.\n";
	(-e "$clust_number.post") && `rm $clust_number.post`;
	next;
    }

    (-e "pdfs") || `mkdir pdfs`;
    $cmd = "ps2pdf $clust_number.post pdfs/$clust_number.pdf";
    (system $cmd ) && die "Error runnning $cmd.\n";
    (-e "$clust_number.post") && `rm $clust_number.post`;

    print "wrote pdfs/$clust_number.pdf\n";

}


close IF;

(-e "blah.gscr") || `rm blah.gscr`;
