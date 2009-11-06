#
# Copyright (c) 2006-2009 National ICT Australia (NICTA), Australia
#
# Copyright (c) 2004-2009 WINLAB, Rutgers University, USA
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
#
# = tellnode.rb
#
# == Description
#
# This file provides a sotfware tool to manually reboot or power ON/OFF 
# one or many node(s). It uses the existing Experiment Controller classes of OMF
#

require 'set'
require 'omf-common/arrayMD'
require 'net/http'
require 'omf-common/mobject'
require 'omf-expctl/nodeHandler.rb'

# Stub Class for the NodeHandler
# The current OMF classe/module are so tighly coupled that nothing can be
# done without a NodeHandler object.
# TODO: "clean" the entire OMF design to remove this non-relevant dependencies
# 

#
# Return a Topology object from a given topology declaration
#
# - topo = a topology declaration (either the name of an existing topology
#          file, or the sequence of nodes within this topology)
#
def getTopo(topo)
  if topo.include?(":")
    filename = topo.delete("[]")
    t = Topology["#{filename}"]
  else
    begin
      t = Topology.create("mytopo", eval(topo))
    rescue Exception => e
      filename = topo.delete("[]")
      t = Topology["#{filename}"]
    end
  end
  return t
end

#
# Send a power ON/OFF or reboot command to a given topology in a 
# given domain.
# Results will be displayed directly on STDOUT
#
# - topo = the Topology to send this command to
# - domain =  the domain to send this command to
#
def tellNode(cmd, topo, domain)
  puts "---------------------------------------------------"
  d = (domain == nil) ?  OConfig.domain : domain
  command = nil
  if (cmd == "on" || cmd == "-on" || cmd == "--turn-on")
    command = "on"
  elsif (cmd == "offs" || cmd == "-offs" || cmd == "--turn-off-soft")
    command = "offSoft"
  elsif (cmd == "offh" || cmd == "-offh" || cmd == "--turn-off-hard")
    command = "offHard"
  end
  if command == nil
    puts "ERROR - Unknown command : '#{cmd}'" 
    puts "Use 'help' to see usage information" 
    puts ""
    exit 1
  end
  puts " Testbed : #{d} - Command: #{command}"
  topo.eachNode { |n|
    url = "#{OConfig[:tb_config][:default][:cmc_url]}/#{command}?x=#{n[0]}&y=#{n[1]}&domain=#{d}"
    response = NodeHandler.service_call(url, "Can't send command to CMC")
    if (response.kind_of? Net::HTTPOK)
      puts " Node n_#{n[0]}_#{n[1]} - Ok"
    else
      puts " Node n_#{n[0]}_#{n[1]} - Error (node state: 'Not Available')"
    end
  }
  puts "---------------------------------------------------"
end

#
# Main Execution loop of this software tool
#
begin
  puts " "  
  cmd = ARGV[0] 
  topo = ARGV[1] 
  domain = ARGV[2] 
  NodeHandler.instance.loadControllerConfiguration()
  NodeHandler.instance.startLogger()
  OConfig.loadTestbedConfiguration()
  Topology.useNodeClass = false
  TraceState.init()
  theTopo = getTopo(topo)
  tellNode(cmd, theTopo, domain)
end
