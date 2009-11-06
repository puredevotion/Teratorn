#
# Define the source group
#
defGroup('source', [1,1]) {|node|

    node.prototype("test:proto:udp_sender", {
	'destinationHost' => '192.168.3.128',
	'destinationPort' => 3000,
	'localHost' => '192.168.3.128',
	'localPort' => 3001,
	'packetSize' => 256,
	'rate' => 8192
    })
}

#
# Define the sink group
#
defGroup('sink', [1,1]) {|node|

    node.prototype("test:proto:udp_receiver", {
	'localHost' => '192.168.3.128'
    })
}

whenAllInstalled() {|node|

    wait 30

    allGroups.startApplications

    wait 20

    Experiment.done
}
