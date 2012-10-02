#! /usr/bin/perl -w
# to get the input:
#  wc -l * | sort -gk1 | tail -n 50 | tee input
# --> note: in the above the column should be "1" (we are coutning from 0)

(@ARGV > 1) ||
	die "Usage:  $0  <input file name>  <output dir name> [<column>]\n";

$filename   = $ARGV[0];
$outdirname = $ARGV[1];
$column	 = 1;
(@ARGV>2)  &&  ($column = $ARGV[2]);

(-e $outdirname) &&
	die "$outdirname already exists; please remove or rename.\n";

open (IF, "<$filename" )
	|| die "Cno $filename: $!.\n";

while ( <IF> ) {
	chomp;
	@aux  = split;
	$name = $aux[$column];

	(-e $name) || next;
	($name =~ /clust/ ) || next;

	@aux = split '/', $name;
	$clust_number = pop @aux;
	$clust_number =~ s/\.clust//;

	open (OF, ">","blah.gscr") ||
		die "No blah.gscr\n";
	print OF "unset key\n";
	print OF 'set format y "%10.3f'."\n";
	print OF "set xlabel \"Retention Time (s)\"\n";
	print OF "set ylabel \"Intensity\" \n";
	print OF "set title \"CLUSTER $clust_number\" \n";
	print OF "set term post color\n";
	print OF "set out '$clust_number.post'\n";
	print OF "plot '$name.sort' using 2:4:(column(-2)) with line lc variable\n";
	close OF;

	open (CF, "<", $name) ||
		die "No $name\n";
	my @clust = <CF>;
	close CF;
	chomp @clust;
	@clust = map {[$_,(split " ", $_)[2]]} @clust;
	@clust = sort {$a->[1] cmp $b->[1]} @clust;
	open (OF, ">", $name . ".sort") ||
		die "No $name.sort\n";
	$lastmz = $clust[0]->[1];
	foreach $point (@clust) {
		if ($lastmz != $point->[1]) {
			print OF "\n\n";
			$lastmz = $point->[1];
		}
		print OF $point->[0];
		print OF "\n";
	}
	close OF;

	$cmd = "gnuplot blah.gscr";
	if (system $cmd ) {
		warn "Error runnning $cmd.\n";
		(-e "$clust_number.post") && `rm $clust_number.post`;
		(-e "$file.sort") && `rm $file.sort`;
		next;
	}

	(-e $outdirname) || `mkdir $outdirname`;
	$cmd = "ps2pdf $clust_number.post $outdirname/$clust_number.pdf";
	(system $cmd ) && die "Error runnning $cmd.\n";
	(-e "$clust_number.post") && `rm $clust_number.post`;
	(-e "$name.sort") && `rm $name.sort`;

	print "wrote $outdirname/$clust_number.pdf\n";
}

close IF;

(-e "blah.gscr") && `rm blah.gscr`;
