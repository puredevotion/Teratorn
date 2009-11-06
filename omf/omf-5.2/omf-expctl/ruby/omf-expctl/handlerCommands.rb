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
# = handlerCommands.rb
#
# == Description
#
# This file contains the definition of all the commands that 
# the experimenters can use in their scripts. All these commands'
# are understood by the EC, which will interpret them in order to
# stage the experiments
#


# The base exception for all errors related 
class OEDLException < Exception; end
  
# Raised for all missing argument exception on OEDL commands.
#
class OEDLMissingArgumentException < OEDLException
  attr_reader :cmdName, :argName
  
  def initialize(cmdName, argName, message = nil)
    @cmdName = cmdName
    @argName = argName
    super(message || "Missing '#{argName}' argument for command '#{cmdName}'")
  end
end

class OEDLIllegalArgumentException < OEDLException
  attr_reader :cmdName, :argName
  
  def initialize(cmdName, argName, legalValues = [], message = nil)
    @cmdName = cmdName
    @argName = argName
    message ||= "Illegal argument values for '#{argName}' in '#{cmdName}'"
    super(message)
  end
end

class OEDLIllegalCommandException < OEDLException
  attr_reader :cmdName, :argName
  
  def initialize(cmdName, message = nil)
    @cmdName = cmdName
    message ||= "Illegal command '#{cmdName}' unsupported by OEDL"
    super(message)
  end   
end


#
# Define an experiment property which can be used to bind
# to application and other properties. Changing an experiment
# property should also change the bound properties, or trigger
# commands to change them.
#
# - name = name of property
# - defaultValue = default value for this property
# - description = short text description of this property
#
def defProperty(name, defaultValue, description)
  Experiment.defProperty(name, defaultValue, description)
end

#
# Return the context for setting experiment wide properties
#
# [Return] a Property Context
#
def prop
  return PropertyContext
end

alias :property :prop

#
# Define a new topology. The topology can
# be described by an optionally array declaration, or
# with a block with the newly created topology as
# single argument.
#
# - refName = the name for this new topology
# - nodeArray = optional, an array that defines the node to add in this topology
# - &block = optional, a code-block containing commands that define this topology
#
# [Return] the newly created Topology object
#
def defTopology(refName, nodeArray = nil, &block)
  if (nodeArray != nil && !nodeArray.kind_of?(Array))
    raise "Topology description. Expected array but got '#{nodeArray}'"
  end
  topo = Topology.create(refName, nodeArray)
  if (! block.nil?)
    block.call(topo)
  end
  topo
end


#
# Define a new prototype. The supplied block is
# executed with the new Prototype instance
# as a single argument.
#
# - refName = reference name for this property
# - name = optional, short/easy to remember name for this property
# - &block = a code-block to execute on the newly created property
#
def defPrototype(refName, name = nil, &block)
  p = Prototype.create(refName)
  p.name = name
  block.call(p)
end

#
# Deprecated, 'defGroup' should be used instead
#
def defNodes(groupName, selector = nil, &block)
  MObject.warn "'defNodes()' is deprecated - please use defGroup() instead"
  defGroup(groupName, selector, &block)
end

#
# Define a set of nodes to be used in the experiment.
# This can either be a specific declaration of nodes to
# use, or a set combining other sets.
#
# - groupName = name of this group of nodes
# - selector = optional, this can be: a String refering to the name of an existing Topology, 
#              or an Array with the name of existing Groups to add to this group, 
#              or an Array explicitly describing the nodes to include in this group 
# - &block = a code-block with commands, which will be executed on the nodes in this group
#
# [Return] a RootNodeSetPath object referring to this new group of nodes
#
def defGroup(groupName, selector = nil, &block)

  if (NodeSet[groupName] != nil)
    raise "Node set '#{groupName}' already defined. Choose different name."
  end

  if selector.kind_of?(ExperimentProperty)
    selector = selector.value
  end

  if (selector != nil)
    # Selector is a name of an existing Topology (e.g. "myTopo")
    if selector.kind_of?(String)
      if ((topo = Topology[selector]) == nil)
        raise "Unknown topology '#{selector}' in node set '#{groupName}'"
      end
      ns = BasicNodeSet.new(groupName, topo)
    # Selector is an Array of...
    elsif selector.kind_of?(Array)
      # Selector is an Array of names of existing groups (e.g. ["group1","group2"])
      if selector[0].kind_of?(String)
        ns = GroupNodeSet.new(groupName, selector)
      # Selector is an Array of node descriptions (e.g. [2, 9, 5])
      else
        tname = "-:topo:#{groupName}"
        topo = Topology.create(tname, selector)
        ns = BasicNodeSet.new(groupName, topo)
      end
    elsif
      raise "Unknown node set declaration '#{selector}: #{selector.class}'"
    end
  else
    ns = BasicNodeSet.new(groupName)
  end

  return RootNodeSetPath.new(ns, nil, nil, block)
end


#
# Evaluate a code-block in the context of a previously defined
# group of nodes.
#
# - groupName = the name of the group of nodes
# - &block = the code-block to evaluate/execute on the group of nodes
#
# [Return] a RootNodeSetPath object referring to the group of nodes
#
def group(groupName, &block)
  ns = NodeSet[groupName.to_s]
  if (ns == nil)
    raise "Undefined node set '#{groupName}'"
  end
  return RootNodeSetPath.new(ns, nil, nil, block)
end

#
# Deprecated, 'group()' should be used instead
#
def nodes(groupName, &block)
  MObject.warn "'nodes()' is deprecated - please use group() instead"
  group(groupName, &block)
end

#
# Evaluate a code-block over all nodes in all groups of the experiment.
#
# - &block = the code-block to evaluate/execute on all the groups of nodes
#
# [Return] a RootNodeSetPath object referring to all the groups of nodes
#
def allGroups(&block)
  NodeSet.freeze
  ns = DefinedGroupNodeSet.instance
  return RootNodeSetPath.new(ns, nil, nil, block)
end

#
# Deprecated, 'AllGroups()' should be used instead
#
def allNodes(&block)
  MObject.warn "'allNodes()' is deprecated - please use allGroups() instead"
  allGroups(&block)
end

#
# Evalute block over all nodes in an the experiment, even those
# that do not belong to any groups
#
# - &block = the code-block to evaluate/execute on all the nodes
#
# [Return] a RootNodeSetPath object referring to all the nodes
#
def allNodes!(&block)
  NodeSet.freeze
  ns = RootGroupNodeSet.instance
  return RootNodeSetPath.new(ns, nil, nil, block)
end

#
# Periodically perform a given test on all nodes in 'nodesSelector'
# and execute block ONCE if all tests on all nodes evaluate to true.
# The interval between checks is given by 'interval' in seconds.
# During any experiment each node maintains a trace of its state in 
# an XML tree. The test to perform is made on the that XML state tree
# for each node. 
#
# This method has 2 behaviours:
# 
# * if 'triggerValue' is nil, then 'nodeTest' should contain an XPath to 
#   match in the XML state tree of the nodes. The test for a given node 
#   will then return 'true' when a match is found for that node.
#
# * if 'triggerValue' is set, then 'nodeTest' should contain an XPath to 
#   match in the XML state tree of the nodes. The test for a given node 
#   will then return 'true' if no match is found OR if a match is found
#   and its value is equal to 'triggerValue'. Thus the test will return
#   'false' if a match is found but its value is different from 
#   'triggerValue'.
#
# - nodesSelector = the name of the group of nodes to test
# - nodeTest = the test to perform on the nodes (i.e. an XPath to match against the node's state)
# - interval = the interval at which to perform the test (in sec, default=5)
# - triggerValue = a value to compare any 'nodeTest' match with, see above description (default=nil)
# - &block = the code-block to execute/evaluate against the nodes when the test returns 'true'
#
def whenAll(nodesSelector, nodeTest, interval = 5, triggerValue = nil, &block)
  ns = NodeSet[nodesSelector]
  if ns == nil
    raise "WhenAll: Unknown node set '#{nodesSelector}"
  end
  Thread.new(ns) { |ns|
    while true
      begin
        res = false
        isUp = ns.up?
        #MObject.debug("whenAll::internal", "Checking ", ns, " up?: ", isUp)
        if isUp
          res = ns.inject(true) { |flag, node|
            if flag
              match = node.match(nodeTest)
              #match.each{|e| e.write($stdout, 2)}
	      if triggerValue == nil
                flag = (match != nil && match.length > 0)
              else
                 match.each{ |e|
                   if (e.to_s != triggerValue.to_s) 
		     flag = false
		   end
	         }
	      end
              MObject.debug("whenAll::internal", "Not true for ", node) if !flag
              #p "FLAG: #{flag}"
            end
            flag
          }
        end
        if res
          MObject.info("whenAll", nodesSelector, ": '", nodeTest, "' fires")
          begin
            RootNodeSetPath.new(ns, nil, nil, block)
          rescue ServiceException => sex
            begin
              if (sex.response)
                MObject.error('run', "ServiceException: #{sex.message}\n\t#{sex.response.body}")
              else
                MObject.error('run', "ServiceException: #{sex.message}")
              end
            rescue Exception
            end
          rescue Exception => ex
            bt = ex.backtrace.join("\n\t")
            MObject.error("whenAll", "Exception: #{ex} (#{ex.class})\n\t#{bt}")
          end
          # done
          break
        end
        Kernel.sleep(interval)
      rescue Exception => ex
        bt = ex.backtrace.join("\n\t")
        puts "Exception: #{ex} (#{ex.class})\n\t#{bt}"
      end
    end
  }
end

#
# This method is a 'syntactic sugar' around 'whenAll' when it is used if a
# 'triggerValue' different of nil. See 'whenAll' method desrciption above.
#
# This method periodically perform a given test on all nodes in 'nodesSelector'
# and execute block ONCE if all tests on all nodes evaluate to true.
# The interval between checks is given by 'interval' in seconds.
# During any experiment each node maintains a trace of its state in 
# an XML tree. The test to perform is made on the that XML state tree
# for each node. 
#
# The 'nodeTest' argument should contain an XPath to match in the XML state 
# tree of the nodes. The test for a given node will then return 'true' if no 
# match is found OR if a match is found and its value is equal to 'triggerValue'. 
# Thus the test will return 'false' if a match is found but its value is different 
# from 'triggerValue'.
#
# - nodesSelector = the name of the group of nodes to test
# - nodeTest = the test to perform on the nodes (i.e. an XPath to match against the node's state)
# - triggerValue = a value to compare any 'nodeTest' match with, see above description 
# - interval = the interval at which to perform the test (in sec, default=5)
# - &block = the code-block to execute/evaluate against the nodes when the test returns 'true'
#
def whenAllEqual(nodesSelector, nodeTest, triggerValue, interval = 5, &block)
  whenAll(nodesSelector, nodeTest, interval, triggerValue, &block)
end

#
# Execute 'block' when all nodes report to be up.
# When the Node Handler (aka Experiment Controller) is invoked for an experiement that 
# supports some temporary disconnection, it ignores any 'whenAllUp' command. Instead
# the 'whenAllUp' commands will be interpreted by the 'slave' Node Handler, which will
# be executed directly on the potentially disconnected node/resource.
#
# - &block = the code-block to execute/evaluate against the nodes in 'up' state 
#
def whenAllUp(&block)
  # Check if this EC instance is set to run in Disconnection Mode
  # If yes, then returned now because whatever is asked from this whenAll should be
  # executed by the whenAll of the slave EC on the disconnected mode
  if NodeHandler.disconnectionMode?
    return
  end
  whenAll("*", "status[@value='UP']", &block)
end

#
# Execute 'block' when all nodes report all applications installed.
# When the Node Handler (aka Experiment Controller) is invoked for an experiement that 
# supports some temporary disconnection, it ignores any 'whenAllInstalled' command. Instead
# the 'whenAllInstalled' commands will be interpreted by the 'slave' Node Handler, which will
# be executed directly on the potentially disconnected node/resource.
#
# - &block = the code-block to execute/evaluate against the 'installed' nodes 
#
def whenAllInstalled(&block)
  # Check if this EC instance is set to run in Disconnection Mode
  # If yes, then returned now because whatever is asked from this whenAll should be
  # executed by the whenAll of the slave EC on the disconnected mode
  if NodeHandler.disconnectionMode?
    return
  end
  whenAllEqual("*", "apps/app/status/@value", "INSTALLED.OK", &block)
end

#
# Periodically execute 'block' every 'interval' seconds until block
# returns nil.
#
# - name = the name for this periodic action
# - interval = interval at which to execute the action (in sec, default=60) 
# - initial = optional, any initial conditions that will be passed to the Thread running this code-block 
# - &block = the code-block to periodically execute/evaluate. This periodic task is stopped when block returns 'nil'
#
def every(name, interval = 60, initial = nil, &block)
  Thread.new(initial) { |context|
    while true
      Kernel.sleep(interval)
      MObject.debug("every(#{name}): fires - #{context}")
      begin
        if ((context = block.call(context)) == nil)
          break
        end
      rescue Exception => ex
        bt = ex.backtrace.join("\n\t")
        MObject.error("every(#{name})", "Exception: #{ex} (#{ex.class})\n\t#{bt}")
      end
    end
    MObject.debug("every(#{name}): finishes")
  }
end

#
# Periodically execute 'block' against a group of nodes every 'interval' seconds 
#
# - nodesSelector = the name of the group of nodes 
# - interval = interval at which to execute the action (in sec, default=60) 
# - &block = the code-block to periodically execute/evaluate
#
def everyNS(nodesSelector, interval = 60, &block)
  ns = NodeSet[nodesSelector]
  if ns == nil
    raise "Every: Unknown node set '#{nodesSelector}"
  end
  path = RootNodeSetPath.new(ns)
  Thread.new(path) { |path|
    while true
      Kernel.sleep(interval)
      MObject.debug("every", nodesSelector, ": fires")
      begin
        if ! (path.call &block)
          break
        end
      rescue Exception => ex
        bt = ex.backtrace.join("\n\t")
        MObject.error("whenAll", "Exception: #{ex} (#{ex.class})\n\t#{bt}")
      end
    end
    MObject.debug("every", nodesSelector, ": finishes")
  }
end

#
# Return the appropriate antenna (set)
#
# - x = x coordinate of the antenna 
# - y = y coordinate of the antenna 
# - precision = optional, how close to (x,y) does the antenna really have to be (default=nil)
#
# [Return] an Antenna object
#
def antenna(x, y, precision = nil)
  a = Antenna[x, y, precision = nil]
  if (a == nil)
    raise "Undefined antenna within #{x}@#{y}"
  end
  return a
end

def defGraph(uri = nil, &block)
  require 'omf-expctl/graph/graph'
  OMF::ExperimentController::Graph::Graph.new(uri, &block)
end

def mstream(uri = nil)
  require 'omf-expctl/oml/mstream'
  if uri
    OMF::ExperimentController::OML::MStream[uri]
  else
    OMF::ExperimentController::OML::MStream
  end
end
alias :mStream :mstream

#
# Wait for some time before issuing more commands
#
# - time = Time to wait in seconds (can be
#
def wait(time)
  if time.kind_of?(ExperimentProperty)
    duration = time.value
  else
    duration = time
  end
  info "Request from Experiment Script: Wait for #{duration}s...."
  Kernel.sleep duration
end

#
# Debugging support:
# print an information message to the 'stdout' & the logfile of EC
#
# - *msg = message to print
#
def info(*msg)
  MObject.info('exp', *msg)
end

#
# Debugging support:
# print an warning message to the 'stdout' & the logfile of EC
#
# - *msg = message to print
#
def warn(*msg)
  MObject.warn('exp', *msg)
end

#
# Debugging support:
# print an error message to the 'stdout' & the logfile of EC
#
# - *msg = message to print
#
def error(*msg)
  MObject.error('exp', *msg)
end

#
# Reporting/Debugging support:
# print the XML tree of states/attributs of EC
#
def ls(xpath = nil)
  root = NodeHandler::ROOT_EL
  if xpath.nil?
    root.write($stdout, 2)
  else
    res = REXML::XPath.match(root, xpath)
    res.inject(true) {|isFirst, el|
      puts "\n--------------------------" unless isFirst
      el.write($stdout, 2)
      false
    }
  end
  '' # supress additional output from IRB
end

#
# Reporting/Debugging support:
# print the XML tree of states/attributs of EC
#
def ls2(xpath = nil)
  root = NodeHandler::ROOT_EL
  if xpath.nil?
    res = NodeHandler::ROOT_EL.children
  else
    res = REXML::XPath.match(root, xpath)
  end

  res.each do |e|
    attrs = e.attributes
    as = ""
    if attrs.size > 0
      res = []
      attrs.each_attribute do |a|	
        res << "#{a.name}=#{a.value}"
      end
      as = " (#{res.join(' ')}) "
    end
    puts "#{e.name}#{as} #{e.text}" 	
  end
  '' # supress additional output from IRB
end

#def _load(file)
#  eval "require('#{file}')", self.binding
#end
#    end # Commands
#  end # ExperimentController
#end # OMF

