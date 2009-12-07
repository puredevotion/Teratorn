#
# Define the source group
#
defGroup('source', [1,1]) {|node|

    node.prototype("test:proto:udp_sender", {
	'destinationHost' => '192.168.3.100',
	'destinationPort' => 3000,
	'localHost' => '192.168.3.100',
	'localPort' => 3001,
	'packetSize' => 256,
	'rate' => 8192 # bits per second
    })
}

#
# Define the sink group
#
defGroup('sink', [1,1]) {|node|

    node.prototype("test:proto:udp_receiver", {
	'localHost' => '192.168.3.100'
    })
}

whenAllInstalled() {|node|

    wait 30

    allGroups.startApplications

    wait 20

    Experiment.done
}
