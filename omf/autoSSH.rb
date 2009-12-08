require 'net/ssh'

begin
    Net::SSH.start("wireless-gw.cs.wisc.edu", "user", "password", :auth_methods => [ "password" ] ) do |ssh|
	puts "Connection opened"
	channel = ssh.open_channel do |ch|
	    ch.on_data do |c, data|
		$STDOUT.print data
	    end

	    ch.on_close { puts "done!" }
	end

	if( channel == nil ) 
	    puts "Failed to open channel"
	    exit 1
	end

	puts "Connection looped"
	ssh.loop { true }
    end
rescue Net::SSH::AuthenticationFailed => ex
    puts "Authentication Failed: #{ex}"
end
