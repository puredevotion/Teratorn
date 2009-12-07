#
# = listPubSubNodes.rb
#
# Connects to a XMPP server and lists all the available Pub/Sub nodes
#

require 'omf-common/omfPubSubService'
require 'xmpp4r/pubsub/helper/nodebrowser'

USAGE = <<USAGE
USAGE: #{$0} xmppIP
USAGE

begin
    if (ARGV.size == 0) 
	puts USAGE
	exit 1
    end

    xmppIP = ARGV[0]
    userJID = "discoverd@#{xmppIP}"
    password = "123"

    clientHelper = Jabber::Client.new(userJID)
    clientHelper.connect(xmppIP)
    clientHelper.auth(password)
    clientHelper.send(Jabber::Presence.new)

    nb = Jabber::PubSub::NodeBrowser.new(clientHelper)

    domainjid = Jabber::JID.new(nil, "pubsub.#{xmppIP}")

    nodes = nb.nodes(domainjid)

    for name in nodes do
	puts "Node: #{name}"
    end
end
