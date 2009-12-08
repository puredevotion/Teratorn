#
# Define the source group
#
defGroup('source', [1,1]) {|node|
    node.prototype("test:proto:udp_sender", {
	'destinationHost' => getControlIP(1,2),
	'destinationPort' => 3000,
	'localHost' => getControlIP(1,1),
	'localPort' => 3001,
	'packetSize' => 256,
	'rate' => 8192 # bits per second
    })
}

#
# Define the sink group
#
defGroup('sink', [1,2]) {|node|
    node.prototype("test:proto:udp_receiver", {
	'localHost' => getControlIP(1,2),
	'localPort' => 3000
    })
}

whenAllInstalled() {|node|
    wait 30

    allGroups.startApplications

    wait 20

    Experiment.done
}
