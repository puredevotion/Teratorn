#!/usr/bin/ruby

# $Revision: 1.2 $
# $Id: ifconfig_wrapper.rb,v 1.2 2004/02/13 06:23:17 daniel Exp $

# = Ruby/Ifconfig - ruby interface to the information presented by ifconfig
#
# == Prerequisites
#
# You must have a working ifconfig binary in your path.
# The output format of you particular version of ifconfig must be supported.
# Currently this has only been tested with net-tool 1.6 on Linux
#
# == Example
# require 'ifconfig'
#
# cfg = IfconfigWrapper.new.parse        # parses the output of ifconfig
#
# cfg.interfaces()           # returns list of interfaces
#
# cfg[eth0].addresses('inet')   # returns ipv4 address of eth0
#
# cfg.addrresses('inet')        # returns list of all ipv4 addresses
#
# cfg[eth0].status   # returns true if interface is up
#
# cfg.each { block } # run a block on each interface object


#
# inspired by Net::Ifconfig::Wrapper perl module
# by Daniel Podolsky, <tpaba@cpan.org>
#
# Ruby version by Daniel Hobe <daniel@packetspike.net>
# Licence::  GPL[http://www.gnu.org/copyleft/gpl.html]
#

class IfconfigWrapper
  NAME = 'Ruby/Ifconfig'
  include Enumerable

  #
  # Can manually specify the platform (should be output of the 'uname' command)
  # and the ifconfig input
  #
  def initialize(platform=nil,input=nil,netstat=nil)
    platform = self.get_os unless !platform.nil?
    require "omf-common/ifconfig/linux/ifconfig"
    @cfg = Ifconfig.new(input,netstat)
  end
  def parse
    return @cfg
  end
  # Find out what os we are on
  #
  def get_os
    plat = IO.popen('uname').gets.strip()
    case plat
      when 'Windows'
        return plat
      when 'Linux'
        return plat
      when 'SunOS'
        return plat
      when 'FreeBSD','OpenBSD','NetBSD'
        return 'BSD'
      else
        return 'Linux'
    end
  end
end
