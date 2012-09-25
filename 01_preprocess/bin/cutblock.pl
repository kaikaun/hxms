#! /usr/bin/perl -w

$preprocess = "./preprocess";

defined $ARGV[3] ||
	die "Usage: cutblock.pl <mzXML> <min_mz> <max_mz> <blocks>\n"; 

$blocks = int($ARGV[3]);
$mz[0] = $ARGV[1];
$mz[$blocks] = $ARGV[2];

$min_t = sqrt($ARGV[1]);
$max_t = sqrt($ARGV[2]);
$inc_t = ($max_t-$min_t)/$blocks;

foreach $a (1 .. ($blocks-1)) {
	$mz[$a] = ($min_t + $inc_t * $a)**2;
}

foreach $a (0 .. ($blocks-1)) {
	$cmdline = $preprocess . " -m " . $mz[$a] .  " -M " . $mz[$a+1] . " ";
	$cmdline = $cmdline . $ARGV[0] . " " . $ARGV[0] . "." . sprintf('%04u',$a);
	print $cmdline . "\n";
	system($cmdline);
}
