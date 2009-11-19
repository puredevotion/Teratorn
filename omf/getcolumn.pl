#!/usr/bin/perl
#
# A script to print whole columns as Matlab data
#

use strict;
use warnings;

use DBI;
use DB;

my $USAGE = <<USAGE;
Usage: $0 <dbname> <tablename> <column1> [ [column2] [column3] ... ]
USAGE

die("Invalid args.\n$USAGE") if( @ARGV < 3 );

my $DBNAME = shift @ARGV;
my $TABLE = shift @ARGV;

# Connect to the database
my $dbconn = DBI->connect("dbi:SQLite:dbname=$DBNAME", "", "")
    or die("Could not connect to database: $!\n");

# Create the select columns string
my $columns = join(",", @ARGV);
$columns =~ s/,\s*$//g;

my $statement = "SELECT $columns FROM $TABLE";
my $query = $dbconn->prepare($statement)
    or die("Could not prepare SQL statement: $statement\n");
$query->execute();

my %results = ();
while( defined(my $ref = $query->fetchrow_hashref()) ) {
    foreach my $col (keys %{ $ref }) {
	push @{ $results{$col} }, $ref->{$col};
    }
}

foreach my $col ( keys %results ) {
    my $str = "$TABLE" . "_$col = [ ";
    foreach my $val ( @{ $results{$col} } ) {
	$str .= "$val ";
    }
    $str .= "];";
    print "$str\n";
}

$dbconn->disconnect();
