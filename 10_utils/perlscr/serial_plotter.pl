#! /usr/bin/perl -w
# to get the input:
#  wc -l * | sort -gk1 | tail -n 50 | tee input 
# --> note: in the above the column should be "1" (we are coutning from 0)

(@ARGV > 1) ||
    die "Usage:  $0  <input file name>  <output dir name> [<column>]\n";

$filename   = $ARGV[0];
$outdirname = $ARGV[1];
$column     = 0;
(@ARGV>2)  &&  ($column = $ARGV[2]);


#(-e $outdirname) && 
#    die "$outdirname already exists; please remove or rename.\n";

open (IF, "<$filename" ) 
    || die "Cno $filename: $!.\n";

($scan, $retT, $mz, $int)  = ();

while ( <IF> ) {

    chomp;
    @aux  = split;
    $name = $aux[$column];

    (-e $name) || next;
    ($name =~ /clust/ ) || next;

    @aux = split '/', $name;
    $clust_number = pop @aux;
    $clust_number =~ s/\.clust//;


    open (OF, ">blah.gscr") || 
	die "CNo blah.gscr.\n";
    print OF "unset key \n";
    print OF "set pm3d map \n";
    print OF 'set format x "%10.1f'."\n";
    print OF 'set format y "%10.3f'."\n";
    #print OF "set xlabel \"SCAN NUMBER\"\n";
    print OF "set xlabel \"Retention Time (s)\"\n";
    print OF "set ylabel \"M/Z\" \n";
    print OF "set title \"CLUSTER $clust_number\" \n";
    print OF 'set palette  defined (0 "white", 300 "blue", 1000 "red", 1200 "yellow")'."\n";
    print OF "set term post color\n";
    print OF "set out '$clust_number.post'\n";
    print OF "splot '$name' using 2:3:4 with points ps 1 pt 7 palette\n";
    close OF;
   
    $cmd = "gnuplot blah.gscr";
    if (system $cmd ) {
	warn "Error runnning $cmd.\n";
	(-e "$clust_number.post") && `rm $clust_number.post`;
	next;
    }

    (-e $outdirname) || `mkdir $outdirname`;

    $ret = `head -n1 $name`;
    chomp $ret;
    ($scan, $retT, $mz, $int) = split " ", $ret;


    $outfile =  sprintf ("m_%.2f_t_%d.pdf", $mz, $retT);

    $cmd     = "ps2pdf $clust_number.post $outdirname/$outfile";
    (system $cmd ) && die "Error runnning $cmd.\n";
    (-e "$clust_number.post") && `rm $clust_number.post`;


}


close IF;

(-e "blah.gscr") && `rm blah.gscr`;
