#!/usr/bin/perl
#
# A script to run top for all related processes
#

use strict;
use warnings;

my $VMWARE_VMX = "vmware-vmx";
my $VMCONTROL = "vmcontrol";
my $VMWARE_AUTHD = "vmware-authd";
my $VMWARE_HOSTD = "vmware-hostd";
my $PGREP = "pgrep -d' '";

# Get all the processes

my $authd_pid = &add_p(`$PGREP $VMWARE_AUTHD`);
chomp $authd_pid;
my $hostd_pid = &add_p(`$PGREP $VMWARE_HOSTD`);
chomp $hostd_pid;
my $vmx_pid = &add_p(`$PGREP $VMWARE_VMX`);
chomp $vmx_pid;
my $vmcontrol_pid = &add_p(`$PGREP $VMCONTROL`);
chomp $vmcontrol_pid;

my $cmd = "top " . join(" ", $authd_pid, $hostd_pid, $vmx_pid, $vmcontrol_pid);

system($cmd);

sub add_p {
    return "-p " . join(" -p ", split(/ /, $_[0]));
}
