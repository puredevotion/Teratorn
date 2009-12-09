#
# Define the source group
#
defGroup('source', [ [1,1], [1,2] ]) {|node|
puts YAML::dump(node.net.e0.ip)
    
    node.prototype("test:proto:udp_sender", {
	'destinationHost' => node.net.e0.ip,
	'destinationPort' => 3000,
	'localHost' => node.net.e0.ip,
	'localPort' => 3001,
	'packetSize' => 256,
	'rate' => 8192 # bits per second
    })
}

#
# Define the sink group
#
defGroup('sink', [[1,1],[1,2]]) {|node|

    node.prototype("test:proto:udp_receiver", {
	'localHost' => node.net.e0.ip
    })
}

whenAllInstalled() {|node|

    wait 30

    allGroups.startApplications

    wait 20

    Experiment.done
}
