#
# = discoverCommunicator.rb
#
# A Publish/Subscribe Communicator to discover Control IPs for their
# nodes and their corresponding coordinates.
#
# Connects to the XMPP server and waits for nodes to report their control IPs
#
# This class is more or less modeled after the XmppCommunicator class in omf-expctl
# (the definition of that class will provide more implementation information)
#

require 'omf-common/omfPubSubService'

class DiscoverPubSubCommunicator < MObject
    DOMAIN = "Domain"
    SYSTEM = "System"
    INV_DOMAIN = "teratorn"

    include Singleton
    @@instantiated = false
    @@user = nil
    @@pass = nil
    @@pubsub_node = nil
    @@db_host = nil
    @@db_user = nil
    @@db_pass = nil
    @@db = nil
    @@inventory = nil

    #
    # Return the Instantiation
    def DiscoverPubSubCommunicator.instantiated?
	return @@instantiated
    end

    def initialize ()
	@@service = nil
	@@instantiated = true

	@queue = Queue.new
	Thread.new {
	    while event = @queue.pop
		execute_command(event)
	    end
	}
    end

    def start(params)
	@@xmppIP = params['xmpp_server']
	@@user = params['discover_user']
	@@pass = params['discover_pass']
	@@pubsub_node = params['discover_pubsub_node']
	@@db_host = params['host']
	@@db_user = params['user']
	@@db_pass = params['password']
	@@db = params['database']

	# Create the Service Helper to interact with the PubSub Server
	begin
	    @@service = OmfPubSubService.new(@@user, @@pass, @@xmppIP)

	    @@service.add_event_callback { |event|
		@queue << event
	    }

	    @@service.join_pubsub_node("/#{DOMAIN}/#{SYSTEM}/#{@@user}")
	rescue Exception => ex
	    error "Failed to create ServiceHelper for PubSub Server '#{@@xmppIP}' - Error: '#{ex}'"
	end

	# keep connection to the PubSub server alive 
	Thread.new do
	    while true do
		sleep 60*60
		debug("Sending a ping to the XMPP server (keepalive)")
		@@service.ping
	    end
	    @@service.quit()
	end
    end

    #
    # Process DISCOVER commands
    # 
    # After receiving a DISCOVER command:
    # 1) Update the MySQL database with the new association between the control
    # IP and the node coordinates 
    #
    # 2) Reply to the node, as it should wait for a response before continuing
    #
    # The format of the DISCOVER messages is:
    #
    # DISCOVER <nodeIdentifier> <nodeIP>
    #
    # e.g. DISCOVER 00:0c:29:60:09:d8 192.168.3.128
    #
    # And the response:
    #
    # OK <DISCOVER message item id> <nodeIdentifier> <nodeIP>
    #
    # e.g. OK 1234 00:0c:29:60:09:d8 192.168.3.128
    #
    # - event:: [Jabber::PubSub::Event]
    #
    def execute_command (event)
	# Extract the Message from the PubSub Event
	begin
	    message = event.first_element("items")\
		           .first_element("item")\
			   .first_element("message")\
			   .first_element("body").text
	    if message.nil?
		return
	    end
	rescue Exception => ex
	    return
	end

	# Parse the Message to extract the Command
	argArray = message.split(' ')
	if (argArray.length < 1)
	  error "Message too short! '#{message}'"
	  return
	end
	 
	# Process the command, if it is valid   
	incomingPubSubNode = event.first_element("items").attributes['node']
	if (argArray[0].upcase == "DISCOVER") 
	    debug "Received on '#{incomingPubSubNode}' - msg: '#{message}'"
	    begin
		# Update the database
		if (@@inventory == nil)
		    @@inventory = MySQLInventory.new(@@db_host, @@db_user, @@db_pass, @@db);
		end

		# First, get the old IP of the node
		ips = []
		@@inventory.runQuery("SELECT ip FROM nodeToIP WHERE identifier='#{argArray[1]}';") {|result|
		    ips << result
		}

		if (ips.size != 0)
		    if ( ips[0] != argArray[2] ) 
			# Update the nodeToIP table with the new IP address
			if (ips.size > 1) 
			    # Delete all entries and insert single entry
			    @@inventory.runQuery("DELETE FROM nodeToIP WHERE identifier='#{argArray[1]};") {}

			    @@inventory.runQuery("INSERT INTO nodeToIP SET ip='#{argArray[2]}', identifier='#{argArray[1]}';"){}
			else
			    # Do a update
			    @@inventory.runQuery("UPDATE nodeToIP SET ip='#{argArray[2]}' WHERE identifier='#{argArray[1]}';"){}
			end

			# Update the nodes table to reflect the change of IP
			ips.each {|ip|
			    @@inventory.runQuery("UPDATE nodes SET control_ip='#{argArray[2]}' WHERE control_ip='#{ip}';"){}
	    		    # Create the pubsub node for the new IP
	    		    @@service.create_pubsub_node("/#{DOMAIN}/#{SYSTEM}/#{ip}")
			}

			# Send the response to the node
			item_id = event.first_element("items")\
				       .first_element("item").attribute("id").to_s
			response = "OK #{item_id} #{argArray[1]} #{argArray[2]}"
			item = Jabber::PubSub::Item.new
			msg = Jabber::Message.new(nil, response);
			item.add(msg)

			@@service.publish_to_node("/#{DOMAIN}/#{SYSTEM}/#{@@user}", item)
		    end
		end
	    rescue Exception => ex
		error "Failed to process message: '#{message}' - Error: '#{ex}'"
		return
	    end
	end
    end
end #class

=begin DEBUGGING
begin
    DiscoverPubSubCommunicator.instance.start()

    if (ARGV.size >= 3) 
	# Send a test DISCOVER message #

	# First, connect to the PubSub Server
	service = OmfPubSubService.new(ARGV[0], ARGV[1], ARGV[2])

	queue = Queue.new

	service.add_event_callback { |event|
	    queue << event
	}

	service.join_pubsub_node("/Domain/System/discoverd")

	# Second, send the DISCOVER message
	message = "DISCOVER [1,1] 192.168.3.128"
	item = Jabber::PubSub::Item.new
	msg = Jabber::Message.new(nil, message)
	item.add(msg)
	service.publish_to_node("/Domain/System/discoverd", item)

	# Third, receive the response
	while event = queue.pop
	    response = event.first_element("items")\
			    .first_element("item")\
			    .first_element("message")\
			    .first_element("body").text
	    puts "Received response = #{response}"
	    if (response == "OK #{item.id} [1,1] 192.168.3.128") 
		puts "DISCOVER command successful"
		break
	    end
	end
    end
end
=end
