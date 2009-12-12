#!/usr/bin/perl 
#
# A Perl script to convert the data from SQL to a Matlab readable format
#
# The database should be the results from running an experiment with the
# Orbit Traffic Generator
#
# The database should have at least the following schema:
#
# _senders (name, id)
# otr2_udp_in (oml_ts_server, oml_ts_client, seq_no_max, seq_no_avg, oml_server_id)
# otg2_udp_out (oml_ts_server, oml_ts_client, seq_no_max, seq_no_avg, oml_server_id)
#

use warnings;
use strict;

use DBI;
use DB;

my $USAGE = <<USAGE;
Usage: $0 <dbfile>
USAGE

die("Invalid args.\n$USAGE") if( @ARGV != 1 );

my $DBNAME = shift @ARGV;

# Connect to the database
my $dbconn = DBI->connect("dbi:SQLite:dbname=$DBNAME", "", "")
    or die("Could not connect to database: $!\n");

# Grab the identifer for the each sender

my $statement = "SELECT name, id FROM _senders";
my $query = $dbconn->prepare($statement)
    or die("Could not prepare SQL statement: $statement\n");
$query->execute();

my %senderToName = ();
while( defined(my $ref = $query->fetchrow_hashref()) ) {
    $senderToName{$ref->{id}} = $ref->{name};
}

foreach my $id ( keys %senderToName ) {
    if ( $senderToName{$id} =~ /sink/ ) {
	$statement = "SELECT oml_ts_server, oml_ts_client, seq_no_max, seq_no_avg from otr2_udp_in where oml_sender_id=$id";
    }else{
	$statement = "SELECT oml_ts_server, oml_ts_client, seq_no_max, seq_no_avg from otg2_udp_out where oml_sender_id=$id";
    }
    $query = $dbconn->prepare($statement) or die("Could not prepare SQL statement: $statement\n");

    $query->execute();

    my $output = "" . $senderToName{$id} . " = [";

    while( defined(my $ref = $query->fetchrow_arrayref()) ) {
	foreach my $col ( @{ $ref } ) {
	    $output .= " " . $col;
	}
	$output .= ";";
    }
    $output .= " ];\n";

    print $output;
}

$dbconn->disconnect();
