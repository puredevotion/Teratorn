#
# Defines a simple experiment has two nodes generate to each other using the
# Orbit traffic generator programs. The topology is:
#
# [1,node1]:3000 -----> [1,node2]:3000
# [1,node2]:4000 -----> [1,node1]:4000
#

defProperty('node1', 1, "The y coordinate of the first node")
defProperty('node2', 2, "The y coordinate of the second node")

#
# Define the sink group for node1
#
defGroup('sink_node1', [1, property.node1.value ]) {|node|
    node.prototype("test:proto:udp_receiver", {
	'localHost' => getControlIP(1, property.node1.value ),
	'localPort' => 4000
    })
}

#
# Define the source group node1
#
defGroup('source_node1', [1, property.node1.value ]) {|node|
    node.prototype("test:proto:udp_sender", {
	'destinationHost' => getControlIP(1,property.node2.value ),
	'destinationPort' => 3000,
	'localHost' => getControlIP(1, property.node1.value ),
	'localPort' => 3000,
	'packetSize' => 256,
	'rate' => 250000 # bits per second
    })
}

#
# Define the sink group for node2
#
defGroup('sink_node2', [1, property.node2.value ]) {|node|
    node.prototype("test:proto:udp_receiver", {
	'localHost' => getControlIP(1, property.node2.value),
	'localPort' => 3000
    })
}

#
# Define the source group node1
#
defGroup('source_node2', [1, property.node2.value ]) {|node|
    node.prototype("test:proto:udp_sender", {
	'destinationHost' => getControlIP(1,property.node1.value),
	'destinationPort' => 4000,
	'localHost' => getControlIP(1,property.node2.value),
	'localPort' => 4000,
	'packetSize' => 256,
	'rate' => 250000 # bits per second
    })
}

#
# Experiment control flow
#
whenAllInstalled() {|node|
    # Wait for everything to get set
    wait 30

    allGroups.startApplications

    # Run for 2 minutes
    wait 120

    Experiment.done
}
