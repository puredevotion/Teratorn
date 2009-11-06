#--
# Copyright (c) 2006 Shawn Patrick Garbett
# Copyright (c) 2002,2004,2005 by Horst Duchene
# 
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
# 
#     * Redistributions of source code must retain the above copyright notice(s),
#       this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright notice,
#       this list of conditions and the following disclaimer in the documentation
#       and/or other materials provided with the distribution.
#     * Neither the name of the Shawn Garbett nor the names of its contributors
#       may be used to endorse or promote products derived from this software
#       without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#++
require 'omf-common/gratr/arc_number'

module GRATR

  # Arc includes classes for representing egdes of directed and
  # undirected graphs. There is no need for a Vertex class, because any ruby
  # object can be a vertex of a graph.
  #
  # Arc's base is a Struct with a :source, a :target and a :label
  Struct.new("ArcBase",:source, :target, :label)

  class Arc < Struct::ArcBase

    def initialize(p_source,p_target,p_label=nil)
      super(p_source, p_target, p_label)
    end

    # Ignore labels for equality
    def eql?(other) self.class == other.class and target==other.target and source==other.source; end

    # Alias for eql?
    alias == eql?

    # Returns (v,u) if self == (u,v).
    def reverse() self.class.new(target, source, label); end

    # Sort support
    def <=>(rhs) [source,target] <=> [rhs.source,rhs.target]; end

    # Arc.new[1,2].to_s => "(1-2 'label')"
    def to_s
      l = label ? " '#{label.to_s}'" : ''
      "(#{source}-#{target}#{l})"
    end
    
    # Hash is defined in such a way that label is not
    # part of the hash value
    def hash() source.hash ^ (target.hash+1); end

    # Shortcut constructor. Instead of Arc.new(1,2) one can use Arc[1,2]
    def self.[](p_source, p_target, p_label=nil)
      new(p_source, p_target, p_label)
    end
    
    def inspect() "#{self.class.to_s}[#{source.inspect},#{target.inspect},#{label.inspect}]"; end
    
  end
  
  class MultiArc < Arc
    include ArcNumber
  end
  
end
