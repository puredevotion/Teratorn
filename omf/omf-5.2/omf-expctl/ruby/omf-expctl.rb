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

$NH_VERSION = "4.4.0"
require 'rubygems'
require 'omf-expctl/nodeHandler'

startTime = Time.now
cleanExit = false

# Initialize the state tracking, Parse the command line options, and Run the NH
begin
  TraceState.init()
  NodeHandler.instance.parseOptions(ARGV)
  NodeHandler.instance.run(self)
  cleanExit = true

# Process the various Exceptions...
rescue SystemExit
rescue Interrupt
  # ignore
rescue IOError => iex
  MObject.fatal('run', iex)
rescue ServiceException => sex
  begin
    MObject.fatal('run', "ServiceException: #{sex.message} : #{sex.response.body}")
  rescue Exception
  end
rescue Exception => ex
  begin
    bt = ex.backtrace.join("\n\t")
    MObject.fatal('run', "Exception: #{ex} (#{ex.class})\n\t#{bt}")
  rescue Exception
  end
end

# If NH is called in 'interactive' mode, then start a Ruby interpreter
#if NodeHandler.instance.interactive?
#  require 'omf-expctl/console'
#  
#  OMF::ExperimentController::Console.instance.run
##  require 'irb'
##  ARGV.clear
##  ARGV << "--simple-prompt"
##  ARGV << "--noinspect"
##  IRB.start()
#end

# End of the experimentation, Shutdown the NH
if (NodeHandler.instance.running?)
  NodeHandler.instance.shutdown
  duration = (Time.now - startTime).to_i
  MObject.info('run', "Experiment #{Experiment.ID} finished after #{duration / 60}:#{duration % 60}")
end

