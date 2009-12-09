require 'net/ssh'

begin
    ssh = Net::SSH.start("wireless-gw.cs.wisc.edu", "user", { :password => "password" })

    ssh.open_channel do |ch|
	# Enable terminal emulation
	ch.request_pty do |ch, success|
	    if !success
		puts "Failed to get pseudo-tty"
		exit 1
	    end
	end

	# Get the wireless gateway to send the login message
	ch.send_channel_request "shell"

	ch.on_close { puts "Failure: channel closed" }
    end
    
    ssh.loop { true }
rescue Net::SSH::AuthenticationFailed => ex
    puts "Authentication Failed: #{ex}"
end
