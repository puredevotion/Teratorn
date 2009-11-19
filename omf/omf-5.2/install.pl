#!/usr/bin/perl
#
# A perl script used to ease development testing
#

use strict;
use warnings;

my $OMF_HOST_ROOT = "/usr/lib/ruby/1.8";
my $OMF_DEV_ROOT = "ruby";

my $AGG = "omf-aggmgr";
my $COMM = "omf-common";
my $EXP = "omf-expctl";

my $USAGE = <<USAGE;
Usage: $0 <$AGG|$COMM|$EXP> [test]
USAGE

die($USAGE) if( @ARGV < 1 || @ARGV > 2 );

my $PKG = $ARGV[0];

if( $PKG !~ /^($AGG|$COMM|$EXP)$/ ) {
    die($USAGE);
}

my $TEST = 0;
if( @ARGV == 2 ) {
    $TEST = 1;
    print "TESTING---NOT COPYING\n";
}

# Special case for expctl
my %files = ();
if( $PKG eq $EXP ) {
    my $hash_ref = &diffFiles("$OMF_HOST_ROOT/$EXP.rb",
	"$EXP/$OMF_DEV_ROOT/$EXP.rb");
    %files = %{ $hash_ref };
}

my $tmp_ref = &diffFiles("$OMF_HOST_ROOT/$PKG/",
 "$PKG/$OMF_DEV_ROOT/$PKG/");

foreach my $key (keys %{ $tmp_ref } ) {
    print "sudo cp -f $key $tmp_ref->{$key}\n";
    if( !$TEST ) {
	print `sudo cp -f $key $tmp_ref->{$key}`;
    }
}

print "!!! Remember to restart omf-aggmgr (/etc/init.d/omf-aggmgr restart)\n";

sub diffFiles {
    my $orig = $_[0];
    my $new = $_[1];

    my %ret = ();

    print "diff -qr $orig $new\n";
    my @lines = `diff -qr $orig $new`;
    foreach my $line (@lines) {
	chomp $line;
	next if( $line !~ /^Files/ );

	# line = "Files ... and ... differ"
	my @fields = split(/ /, $line);

	next if( @fields != 5 );

	my $dest = $fields[1];
	my $src = $fields[3];

	$ret{$src} = $dest;
    }

    return \%ret;
}
