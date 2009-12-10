#!/usr/bin/perl
#
# A perl script to provide a kernel-compiling workload
#
use strict;
use warnings;

my $KERNEL_DIR = "/usr/src/linux-source-2.6.30";
my $MAKE_CMD = "make-kpkg --rootcmd=fakeroot kernel_image";
my $CLEAN_CMD = "make-kpkg clean";

my $USAGE = <<USAGE;
Usage: $0 <number of iterations>
USAGE

if( scalar(@ARGV) < 1 ) {
    print STDERR $USAGE;
    exit 1;
}

chdir $KERNEL_DIR;

my $i;
for($i = 0; $i < $ARGV[0]; ++$i) {
    print "!!! Starting iteration $i\n";
    my $start = time;

    &make();
    &clean();

    my $end = time;

    print "!!! Iteration $i: " . ($end - $start) . " seconds\n";

    if( $i != ($ARGV[0] - 1) ) {
	print "\n\n";
    }
}

sub make {
    open(CMD, "-|") or exec($MAKE_CMD . " 2>&1") 
	or die("Failed to execute make command: $!\n");

    while(my $line = <CMD>) {
	print "\t\tMAKE: $line";
    }

    close(CMD);
}

sub clean {
    open(CMD, "-|") or exec($CLEAN_CMD . " 2>&1")
	or die("Failed to execute clean command: $!\n");

    while(my $line = <CMD>) {
	print "\t\tCLEAN: $line";
    }

    close(CMD);
}
