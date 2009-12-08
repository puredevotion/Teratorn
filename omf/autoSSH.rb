require 'net/ssh'

begin
    Net::SSH.start("wireless-gw.cs.wisc.edu", "mcnulty", "", :auth_methods => [ "password" ] ) do |ssh|
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

	channel.request_pty do |ch, success| 
	    if success
		puts "pty obtained"
	    else
		puts "failed to obtain pty"
		exit 1
	    end
	end

	puts "Connection looped"
	ssh.loop { true }
    end
rescue Net::SSH::AuthenticationFailed => ex
    puts "Authentication Failed: #{ex}"
end
