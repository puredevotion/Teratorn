/*  Copyright 2009 Dan McNulty, Tom Baars
 *
 *  This file is part of Teratorn.
 *
 *  Teratorn is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Standard includes
#include <iostream>
#include <cerrno>
#include <cstdlib>
#include <cassert>
#include <map>
#include <set>
using std::cout;
using std::cerr;
using std::endl;
using std::string;
using std::map;
using std::set;

// C header files
extern "C" {
#include <unistd.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// libdaemon includes
#include <libdaemon/daemon.h>
}

// Project includes
#include "cpulimit.h"
#include "vixint.h"
#include "util.h"

/* 
 * The commands received via a network interface take on the following format:
 * 
 * USER username PASS password CMD vm_cmd \r\n
 *
 * Take note of the use of a single space as a delimiter between all tokens.
 */

/*
 * The format of the config file is:
 * option1=value1
 * option2=value2
 * option3=value3
 * ...
 *
 * valid options are (identifer = type of value)
 * hostname = string
 * port = string
 * remote = integer (non-zero means true, zero means false)
 * user = string
 * password = string
 * limit = integer (between 1 and 100)
 * vmpath = string
 */

typedef char vm_cmd;
enum {
    VM_UP = 'U',
    VM_DOWN = 'D'
};

// Fields
static const string USER = "USER";
static const string PASS = "PASS";
static const string CMD = "CMD";

// Responses
static const string BAD = "BAD";
static const string OK = "OK";

// Delimiter
static const string END = "\r\n";
static const string CONFIG_DELIM = "=";

// TODO SSL support for the command socket

static const char *USAGE = 
" [-n] [-c full_path_config] [-H hostname] [-p port] [-r] -U user -l limit <-u|-d> <vmpath>";
static const char *OPT_STR = "H:udU:l:nrp:c:";
static const char *DEFAULT_HOST = "https://127.0.0.1:8333/sdk";
static const char *DEFAULT_PORT = "8400";
static const int CHECK_VM_PERIOD = 2*60; // seconds
static const int LOWEST_PRIO = 19;
static const char *DEFAULT_CONFIG = "/etc/vmware/vmcontrol/config";

// Prototypes
int die(int, int, char *, int, int, set<int>&);
int launchCPULimit(pid_t, int, int *, int *);
int controlLoop(pid_t, int, int, char *, bool, char *, char *, char *, char *);
pid_t daemonize();
int recvCommand(int, string &);
int parseCommandToken(string &, const string &, string &, size_t);
int parseCommand(string &, string &, string &, vm_cmd &);
int parseResponse(string &, string &);
int parseConfigLine(FILE *, string &, string &);

int main(int argc, char *argv[]) {
    bool powerOn = false;
    bool powerOff = false;
    bool daemon = true;
    bool local = true;
    char *port = const_cast<char *>(DEFAULT_PORT);
    char *user = NULL;
    char *password = NULL;
    char *path = NULL;
    char *hostname = const_cast<char *>(DEFAULT_HOST);
    char *config = const_cast<char *>(DEFAULT_CONFIG);
    int limit = 0;

    // Initial daemon logging
    daemon_pid_file_ident = daemon_ident_from_argv0(argv[0]);
    daemon_log_ident = daemon_ident_from_argv0(argv[0]);

    int arg;
    while( (arg = getopt(argc, argv, OPT_STR)) != -1 ) {
	switch((char)arg) {
	    case 'u':
		powerOn = true;
		break;
	    case 'd':
		powerOff = true;
		break;
	    case 'U':
		user = optarg;
		break;
	    case 'H':
		hostname = optarg;
		break;
	    case 'p':
		port = optarg;
		break;
	    case 'l':
		limit = strtol(optarg, NULL, 10);
		break;
	    case 'n':
		daemon = false;
		break;
	    case 'r':
		local = false;
		break;
	    case 'c':
		config = optarg;
		break;
	    default:
		cerr << "Invalid Arg: " << (char)arg << endl 
		     << "Usage: " << argv[0] << USAGE << endl;
		return 1;
	}
    }

    if( !access(config, R_OK) ) {
	FILE *configFile = fopen(config, "r");
	if( configFile == NULL ) {
	    // This is not a critical error, try to proceed
	    cerr << "Could not open config file '" << config << "': " <<
		strerror(errno) << endl;
	}else{
	    // Parse the config file
	    int line = 1;
	    bool readMore = true;
	    while( readMore ) {
		string option;
		string value;
		int parseRet = parseConfigLine(configFile, option, value);

		if( parseRet < 0 ) {
		    cerr << "encountered error when reading config file at line " << line
			 << endl;
		    readMore = false;
		}else if( parseRet == 0 ) {
		    fclose(configFile);
		    readMore = false;
		}else{
		    char *valueStr = new char[value.length()];
		    strncpy(valueStr, value.c_str(), value.length());

		    /*
		     * valid options are (identifer = type of value)
		     * hostname = string
		     * port = string
		     * remote = integer (non-zero means true, zero means false)
		     * user = string
		     * password = string
		     * limit = integer (between 1 and 100)
		     * vmpath = string
		     */
		    if( option == "hostname" ) {
			hostname = valueStr;
		    }else if( option == "port" ) {
			port = valueStr;
		    }else if( option == "remote" ) {
			int tmpVal = strtol(valueStr, NULL, 10);
			if( tmpVal == 0 ) local = true;
			else local = false;
			delete valueStr;
		    }else if( option == "user" ) {
			user = valueStr;
		    }else if( option == "password" ) {
			password = valueStr;
		    }else if( option == "limit" ) {
			limit = strtol(valueStr, NULL, 10);
			delete valueStr;
		    }else if( option == "vmpath" ) {
			path = valueStr;
		    }else{
			cerr << "invalid option in config file '" << option << "', line "
			    << line << endl;
		    }
		}
		line++;
	    }
	}
    }

    if( (!powerOn && !powerOff) || !user ) {
	cerr << "Must specify -u or -d and a user" << endl;
	cerr << "Usage: " << argv[0] << USAGE << endl;
	return 1;
    }
  
    if( powerOn && powerOff ) {
	cerr << "Ambiguous option: power on or power off?" << endl;
	cerr << "Usage: " << argv[0] << USAGE << endl;
	return 1;
    }

    // Ask for the password
    if( password == NULL ) {
	cout << user << "'s password: ";
	password = get_password();
	cout << endl;
	if( password == NULL ) {
	    return 1;
	}
    }

    // If the daemon is already running, send it the respective command
    // Otherwise, start the daemon

    pid_t daemonPid;
    if( (daemonPid = daemon_pid_file_is_running()) >= 0 ) {
	daemon_log(LOG_INFO, "Daemon running...sending command to daemon");
	// Build the command
	string sendCmd = USER + " " + user + " " +
	                 PASS + " " + password + " " + 
	                 CMD + " ";
	if( powerOff ) {
	    sendCmd.append(1, static_cast<char>(VM_DOWN));
	}else if( powerOn ) {
	    sendCmd.append(1, static_cast<char>(VM_UP));
	}else{
	    // Unreachable
	    assert(0);
	}

	sendCmd += " " + END;

	// Connect to the daemon
	struct addrinfo opts;
	struct addrinfo *addr_list;
	int status;

	bzero(&opts, sizeof(struct addrinfo));
	opts.ai_family = AF_UNSPEC;
	opts.ai_socktype = SOCK_STREAM;
	opts.ai_flags = AI_PASSIVE;

	char *bindHost = const_cast<char *>("localhost");

	if( (status = getaddrinfo(bindHost, port, &opts, &addr_list)) != 0 ) {
	    daemon_log(LOG_ERR, "Error getting address info: %s\n",
		    gai_strerror(status));
	    zero_string(password);
	    return 1;
	}

	int daemon;
	if( (daemon = get_socket(addr_list, false) ) == -1 ) {
	    // get_socket does the logging internally
	    zero_string(password);
	    return 1;
	}

	struct addrinfo *addr_iter;
	bool connected = false;
	for(addr_iter = addr_list; addr_iter != NULL; 
	    addr_iter = addr_iter->ai_next) 
	{
	    if( connect(daemon, addr_iter->ai_addr, addr_iter->ai_addrlen) ) {
		daemon_log(LOG_ERR, L_LOG_STR("connect"));
	    }else{
		// done if connected successfully
		connected = true;
		break;
	    }
	}
	freeaddrinfo(addr_list);

	if( !connected ) {
	    zero_string(password);
	    close(daemon);
	    return 1;
	}

	// send command
	if( !send_all(daemon, const_cast<char *>(sendCmd.c_str()), 
		    sendCmd.length()) ) {
	    zero_string(password);
	    close(daemon);
	    return 1;
	}

	// Set up info for select
	fd_set read_fds, saved;
	FD_ZERO(&saved);
	FD_SET(daemon, &saved);

	string response;
	string value;

	bool noResponse = true;
	while(noResponse) {
	    read_fds = saved;
	    int select_ret = select(daemon + 1, &read_fds, NULL, NULL, NULL);

	    if( select_ret == -1 ) {
		if( errno == EINTR ) {
		    continue;
		}else{
		    daemon_log(LOG_ERR, L_LOG_STR("select"));
		    zero_string(password);
		    close(daemon);
		    return 1;
		}
	    }

	    if( FD_ISSET(daemon, &read_fds) ) {
		int recvRet = recvCommand(daemon, response);

		// It is an error if the server closes the connection before
		// sending the response
		if( recvRet < 0 ) {
		    daemon_log(LOG_ERR, 
			    "Command failed: did not receive server response");
		    zero_string(password);
		    close(daemon);
		    return 1;
		}

		int sizeCmd = parseResponse(response, value);

		if( sizeCmd < 0 ) {
		    daemon_log(LOG_ERR,
			    "Command failed: received invalid response");
		    zero_string(password);
		    close(daemon);
		    return 1;
		}

		// Incomplete command, wait for more data
		if( sizeCmd == 0 ) {
		    if( recvRet == 0 ) {
			daemon_log(LOG_ERR, 
				"Command failed: did not receive server response");
			zero_string(password);
			close(daemon);
			return 1;
		    }else{
			continue;
		    }
		}

		noResponse = false;
	    }
	}

	// Verify the response
	if( value == BAD ) {
	    daemon_log(LOG_ERR,
		    "Command failed: server reports invalid command");
	}else if( value == OK ) {
	    daemon_log(LOG_INFO,
		    "Command successful");
	}else{
	    daemon_log(LOG_ERR,
		    "Command failed: received invalid response");
	}

	zero_string(password);
	close(daemon);
    }else{
	// This program is really only useful with root privileges. 
	// Also, this program should not be setuid root
	if( geteuid() == 0 && getuid() != 0 ) {
	    cerr << "Don't run this program setuid...it is a bad idea." << endl;
	    return 1;
	}

	if( getuid() != 0 ) {
	    cerr << "You are not root." 
		 << " This program is useless without root privileges." << endl;
	    return 1;
	}

	// Process options for the daemon
	if(path == NULL) {
	    if(optind >= argc) {
		cerr << "Virtual machine not specified" << endl;
		cerr << "Usage: " << argv[0] << USAGE << endl;
		return 1;
	    }
	    path = argv[optind];
	}


	if( limit < 1 || limit > 100 ) {
	    cerr << "Limit must be in range 1-100%" << endl;
	    cerr << "Usage: " << argv[0] << USAGE << endl;
	    return 1;
	}

	if( powerOff ) {
	    if( !powerOffVM(path, user, password, hostname) ) return 1;

	    // Zero the password
	    zero_string(password);
	}else if( powerOn ) {
	    if( daemon ) {
		// Daemon isn't running, so start it
		daemonPid = daemonize();

		if( daemonPid < 0 ) {
		    return 1;
		}else if ( daemonPid > 0 ) {
		    // Daemon successfully started, end parent
		    return 0;
		}
	    }

	    // Regardless, create a pid file
	    if( daemon_pid_file_create() < 0 ) {
		daemon_log(LOG_ERR, L_LOG_STR("daemon_pid_file_create"));
		return 1;
	    }

	    if( !powerOnVM(path, user, password, hostname) ) return 1;

	    pid_t vmPid = getVMPid(path);

	    if( vmPid == -1 ) {
		daemon_log(LOG_ERR, "Could not determine PID of newly"
			" launched virtual machine...failing.");
		return 1;
	    }

	    daemon_log(LOG_INFO, "VM_PID = %d", vmPid);

	    // renice virtual machine to lowest priority
	    // not a failure if it cannot be niced
	    if( setpriority(PRIO_PROCESS, vmPid, LOWEST_PRIO) ) {
		daemon_log(LOG_WARNING, L_LOG_STR("setpriority"));
	    }

	    return !controlLoop(vmPid, limit, CHECK_VM_PERIOD, port, local,
		    path, user, password, hostname);
	}
    }

    return 0;
}

/**
 * Cleanup function called through the program
 *
 * cmdSocket 		The command socket
 * main_pipe		The pipe for the main process
 * password		The password used for the VIX API
 * cpulimit_pid		The PID for the cpulimit process
 * signal_fd		The fd used to read signals
 * sockets		A set of sockets opened in the program
 *
 * Always returns 0
 */
int die(int cmdSocket, int main_pipe, char *password, int cpulimit_pid,
	int signal_fd, set<int> &sockets) 
{
    // Zero the password
    if( password != NULL ) {
	zero_string(password);
    }

    // Close the listening socket
    if( cmdSocket > 0 ) {
	// Errors don't matter, the file is being deleted
	if( close(cmdSocket) ) {
	    daemon_log(LOG_ERR, L_LOG_STR("close"));
	}
    }

    // Close all command sockets
    set<int>::iterator sock_it;
    for(sock_it = sockets.begin(); sock_it != sockets.end(); ++sock_it) {
	if( close(*sock_it) ) {
	    daemon_log(LOG_ERR, L_LOG_STR("close"));
	}
    }

    // Alert the cpulimit process about termination
    if( main_pipe > 0 ) {
	// Errors don't matter, the process is terminating
	close(main_pipe);
    }

    // Tell cpulimit process to stop
    if( cpulimit_pid > 0 ) {
	kill(cpulimit_pid, SIGTERM);
    }

    if( signal_fd > 0 ) {
	close(signal_fd);
    }

    daemon_signal_done();
    daemon_pid_file_remove();

    return 0;
}

/**
 * Launches a separate process for cpulimit functionality
 *
 * pid		The pid of the VM
 * limit	The CPU limit percentage
 * cpulimitfd	The read end of the bi-dir pipe between the cpulimit and the main process
 * mainfd	The write end of the bi-dir pipe
 *
 * -1, on failure, the cpulimit PID otherwise
 */
int launchCPULimit(pid_t pid, int limit, int *cpulimitfd, int *mainfd) {
    int cpulimit_pipe[2];
    int main_pipe[2];
    pid_t cpulimit_pid;

    if( pipe(cpulimit_pipe) == -1 ) {
	daemon_log(LOG_ERR, L_LOG_STR("pipe"));
	return -1;
    }

    if( pipe(main_pipe) == -1 ) {
	daemon_log(LOG_ERR, L_LOG_STR("pipe"));
	return -1;
    }

    if( (cpulimit_pid = fork()) < 0 ) {
	daemon_log(LOG_ERR, L_LOG_STR("fork"));
	return -1;
    }

    if( cpulimit_pid == 0 ) {
	// In cpulimit process
	if( daemon_reset_sigs(-1) < 0 ) {
	    daemon_log(LOG_ERR, L_LOG_STR("daemon_reset_sigs"));
	    daemon_log(LOG_ERR, "cpulimit process failed. Bailing!");
	    exit(1);
	}

	if( daemon_unblock_sigs(-1) < 0 ) {
	    daemon_log(LOG_ERR, L_LOG_STR("daemon_unblock_sigs"));
	    daemon_log(LOG_ERR, "cpulimit process failed. Bailing!");
	    exit(1);
	}

	int cpulimit, main;
	
	// writes to its pipe
	if( close(cpulimit_pipe[0]) ) {
	    daemon_log(LOG_ERR, L_LOG_STR("close"));

	    // Try to let main process know
	    close(cpulimit_pipe[1]);
	    daemon_log(LOG_ERR, "cpulimit process failed. Bailing!");
	    exit(1);
	}

	cpulimit = cpulimit_pipe[1];

	// reads from the main process' pipe
	if( close(main_pipe[1]) ) {
	    daemon_log(LOG_ERR, L_LOG_STR("close"));

	    // Try to let main process know
	    close(cpulimit);
	    daemon_log(LOG_ERR, "cpulimit process failed. Bailing!");
	    exit(1);
	}

	main = main_pipe[0];

	// Wow, VIX opens quite a few file descriptors
	if( daemon_close_all(main, cpulimit, -1) < 0 ) {
	    daemon_log(LOG_ERR, L_LOG_STR("daemon_close_all"));

	    // Try to let main process know
	    close(cpulimit);
	    daemon_log(LOG_ERR, "cpulimit process failed. Bailing!");
	    exit(1);
	}

	cpulimit_cmd msg = CPULIMIT_START;
	do{
	    if( msg == CPULIMIT_START ) 
		cpulimit_bypid(0, 1, limit, pid);

	    daemon_log(LOG_ERR, "cpulimit stopped.");

	    // If we get here, it means cpulimit died for some reason so alert
	    // the main process
	    
	    msg = CPULIMIT_STOPPED;
	    if( !write_all(cpulimit, &msg, sizeof(cpulimit_cmd)) ) {
		daemon_log(LOG_ERR, L_LOG_STR("write_all"));

		// Try to let main process know
		close(cpulimit);
		daemon_log(LOG_ERR, "cpulimit process failed. Bailing!");
		exit(1);
	    }

	    // Wait for command from main process
	    int read_result = read_all(main, &msg, sizeof(cpulimit_cmd));
	    if( read_result == 0 ) {
		msg = CPULIMIT_END;
	    }else if( read_result == -1 ) {
		daemon_log(LOG_ERR, L_LOG_STR("read_all"));

		// Try to let main process know
		close(cpulimit);
		daemon_log(LOG_ERR, "cpulimit process failed. Bailing!");
		exit(1);
	    }
	}while(msg != CPULIMIT_END);

	daemon_log(LOG_INFO, "cpulimit received end command. Stopping!");
	exit(1);
    }else{
	// In main process
	
	// writes to its pipe
	if( close(main_pipe[0]) ) {
	    daemon_log(LOG_ERR, L_LOG_STR("close"));

	    // Try to let cpulimit process know
	    close(main_pipe[1]);
	    return -1;
	}

	// reads from cpulimit's pipe
	if( close(cpulimit_pipe[1]) ) {
	    daemon_log(LOG_ERR, L_LOG_STR("close"));

	    // Try to let cpulimit process know
	    close(main_pipe[1]);
	    return -1;
	}

	*mainfd = main_pipe[1];
	*cpulimitfd = cpulimit_pipe[0];

	return cpulimit_pid;
    }

    return -1;
}

/**
 * The main loop of the daemon
 *
 * vmPid		The PID for the VM process
 * limit		The CPU usage limit
 * checkVMPeriod	How often to check if the VM is still running
 * port			The port to listen on
 * local		Whether the command socket should be local or remote
 * path			The path to the VM (relative to the datastore)
 * user			The user for VMWare
 * password		The password for VMWare
 * hostname		The hostname for VMWare
 */
int controlLoop(pid_t vmPid, int limit, int checkVMPeriod, char *port, 
	bool local, char *path, char *user, char *password, char *hostname) 
{
    int cpulimitfd, mainfd, cpulimit_pid;
    set<int> sockets;

    cpulimit_pid = launchCPULimit(vmPid, limit, &cpulimitfd, &mainfd);
    if( cpulimit_pid == -1 ) {
	daemon_log(LOG_ERR, "Failed to launch cpulimit process.");
	return die(-1, -1, password, -1, -1, sockets);
    }

    // Set up the command socket
    struct addrinfo opts;
    struct addrinfo *addr_list;
    int status;

    bzero(&opts, sizeof(struct addrinfo));
    opts.ai_family = AF_UNSPEC;
    opts.ai_socktype = SOCK_STREAM;
    opts.ai_flags = AI_PASSIVE;

    char *bindHost = NULL;
    if( local ) {
	// Only listen on local address
	bindHost = const_cast<char *>("localhost");
    }

    if( (status = getaddrinfo(bindHost, port, &opts, &addr_list)) != 0 ) {
	daemon_log(LOG_ERR, "Error getting address info: %s\n",
		gai_strerror(status));
	return die(-1, mainfd, password, cpulimit_pid, -1, sockets);
    }

    int cmdSocket;
    if( (cmdSocket = get_socket(addr_list, true) ) == -1 ) {
	// get_socket does the logging internally
	return die(-1, mainfd, password, cpulimit_pid, -1, sockets);
    }
    freeaddrinfo(addr_list);

    // Listen for commands on this socket
    if( listen(cmdSocket, 10) ) {
	daemon_log(LOG_ERR, L_LOG_STR("listen"));
	return die(cmdSocket, mainfd, password, cpulimit_pid, -1, sockets);
    }

    // set up fd used to read signals
    if( daemon_signal_init(SIGINT, SIGTERM, SIGQUIT, SIGHUP, 0) < 0 ) {
	daemon_log(LOG_ERR, L_LOG_STR("daemon_signal_init"));
	return die(cmdSocket, mainfd, password, cpulimit_pid, -1, sockets);
    }

    int signal_fd = daemon_signal_fd();
    if( signal_fd < 0 ) {
	daemon_log(LOG_ERR, L_LOG_STR("daemon_signal_fd"));
	return die(cmdSocket, mainfd, password, cpulimit_pid, signal_fd, 
		sockets);
    }

    // set up state for select
    fd_set saved, read_fds;
    int maxFd = ( cpulimitfd > cmdSocket ) ? cpulimitfd : cmdSocket;
    maxFd = ( maxFd > signal_fd ) ? maxFd : signal_fd;
    FD_ZERO(&saved);
    FD_SET(cmdSocket, &saved);
    FD_SET(cpulimitfd, &saved);
    FD_SET(signal_fd, &saved);

    // Wait for commands from network, cpulimit pipe or a signal
    // Periodically, check that VM is still running
    bool stop = false;

    // holds incomplete commands
    map<int, string> incCmds;

    while(!stop) {
	int select_ret;

	read_fds = saved;

	struct timeval vmcheck_to;
	vmcheck_to.tv_sec = checkVMPeriod;
	vmcheck_to.tv_usec = 0;

	select_ret = select(maxFd + 1, &read_fds, NULL, NULL, &vmcheck_to);

	if( select_ret == -1 ) {
	    if( errno == EINTR ) {
		continue;
	    }else{
		daemon_log(LOG_ERR, L_LOG_STR("select"));
		return die(cmdSocket, mainfd, password, cpulimit_pid, signal_fd,
			sockets);
	    }
	}

	for(int i = 0; i <= maxFd; ++i) {
	    if( FD_ISSET(i, &read_fds) ) {
		// Signals should be handled first
		if( i == signal_fd ) {
		    int sig;

		    if( (sig = daemon_signal_next()) <= 0 ) {
			daemon_log(LOG_ERR, L_LOG_STR("daemon_signal_next"));
			return die(cmdSocket, mainfd, password, cpulimit_pid,
				signal_fd, sockets);
		    }

		    switch(sig) {
			case SIGINT:
			case SIGTERM:
			case SIGHUP:
			case SIGQUIT:
			    stop = true;
			    break;
			default:
			    break;
		    }
		// Handle cpulimit event
		}else if( !stop && i == cpulimitfd ) {
		    cpulimit_cmd msg;
		    if( !read_all(cpulimitfd, &msg, sizeof(cpulimit_cmd)) ) {
			return die(cmdSocket, mainfd, password, cpulimit_pid,
				signal_fd, sockets);
		    }
		    daemon_log(LOG_INFO, 
			    "Received command from cpulimit: %d", msg);

		    if( msg == CPULIMIT_STOPPED ) {
			if( !isVMRunning(path, user, password, hostname) ) {
			    if( !powerOnVM(path, user, password, hostname) ) {
				return die(cmdSocket, mainfd, password, 
					cpulimit_pid, signal_fd, sockets);
			    }

			    msg = CPULIMIT_START;
			    if( !write_all(mainfd, &msg, 
					sizeof(cpulimit_cmd)) ) 
			    {
				return die(cmdSocket, mainfd, password, 
					cpulimit_pid, signal_fd, sockets);
			    }
			}else{
			    // cpulimit stopped for some other reason
			    stop = true;
			    msg = CPULIMIT_END;
			    if( !write_all(mainfd, &msg, sizeof(cpulimit_cmd)) )
			    {
				return die(cmdSocket, mainfd, password, 
					cpulimit_pid, signal_fd, sockets);
			    }
			}
		    }
		// Handle new connection
		}else if( !stop && i == cmdSocket ) {
		    struct sockaddr_storage remote_addr;
		    socklen_t addr_size = sizeof(struct sockaddr_storage);

		    int newConn = accept(cmdSocket,
			    (struct sockaddr *)&remote_addr, &addr_size);

		    if( newConn == -1 ) {
			daemon_log(LOG_ERR, L_LOG_STR("accept"));
		    }else{
			FD_SET(newConn, &saved);
			if( newConn > maxFd ) maxFd = newConn;
			sockets.insert(newConn);

			struct sockaddr_in *addr =
			    (struct sockaddr_in *)&remote_addr;
			daemon_log(LOG_INFO, "new conn from %s, socket=%d",
				inet_ntoa(addr->sin_addr), newConn);
		    }
		// Any other socket will send commands to be processed
		}else{
		    daemon_log(LOG_INFO, "activity on socket=%d", i);
		    // Yes, this should be a reference to get the desired effect
		    string &in_cmd = incCmds[i];
		    int recvRet = recvCommand(i, in_cmd);

		    if (recvRet < 0) {
			return die(cmdSocket, mainfd, password, cpulimit_pid, 
				signal_fd, sockets);
		    }
		    
		    if (recvRet == 0) {
			// Connection closed
			FD_CLR(i, &saved);
			sockets.erase(i);

			if (close(i)) {
			    daemon_log(LOG_ERR, L_LOG_STR("close"));
			}
			daemon_log(LOG_INFO, "socket=%d closed", i);
			continue;
		    }

		    // Process all received commands
		    int sizeCmd = -1;
		    do {
			string cmd_user, cmd_password;
			vm_cmd cmd;

			sizeCmd = parseCommand(in_cmd, cmd_user, 
				cmd_password, cmd);

			if( sizeCmd < 0 ) {
			    // Invalid command received
			    daemon_log(LOG_INFO, "Received invalid command");

			    // Remove the invalid command
			    size_t endPos = in_cmd.find(END);

			    endPos += END.length();
			    if( endPos >= in_cmd.length()-1 ) {
				in_cmd.clear();
			    }else{
				in_cmd = in_cmd.substr(endPos);
			    }

			    string response = BAD + END;
			    if( !send_all(i, 
					const_cast<char *>(response.c_str()), 
					response.length()))
			    {
				return die(cmdSocket, mainfd, password, 
					cpulimit_pid, signal_fd, sockets);
			    }
			}else if( sizeCmd > 0 ) {
			    // Valid command found

			    // super basic authentication => must match the 
			    // credentials used to talk to the virtual machine
			    if (cmd_user != string(user) || 
				cmd_password != string(password) ) 
			    {
				daemon_log(LOG_WARNING, 
					"invalid credentials on socket=%d", i);
				string response = BAD + END;
				if( !send_all(i, 
					    const_cast<char *>(response.c_str()), 
					    response.length()))
				{
				    return die(cmdSocket, mainfd, password, 
					    cpulimit_pid, signal_fd, sockets);
				}

				if( sizeCmd  >= static_cast<int>(in_cmd.length()-1) ) {
				    in_cmd.clear();
				}else{
				    in_cmd = in_cmd.substr(sizeCmd);
				}

				continue;
			    }

			    daemon_log(LOG_INFO, "Received command: %c", cmd);

			    cpulimit_cmd cpulimit_msg;

			    string response_field;

			    switch (cmd) {
			    case VM_UP:
				if (!powerOnVM(path, user, password, hostname)) {
				    return die(cmdSocket, mainfd, password, 
					    cpulimit_pid, signal_fd, sockets);
				}

				// Queue a start event for cpulimit 
				cpulimit_msg = CPULIMIT_START;
				if (!write_all(mainfd, &cpulimit_msg, 
					    sizeof(cpulimit_cmd))) 
				{
				    return die(cmdSocket, mainfd, password, 
					    cpulimit_pid, signal_fd, sockets);
				}

				response_field = OK;

				break;
			    case VM_DOWN:
				if (!powerOffVM(path, user, password, hostname)) {
				    return die(cmdSocket, mainfd, password, 
					    cpulimit_pid, signal_fd, sockets);
				}

				// Queue a stop event for cpulimit
				cpulimit_msg = CPULIMIT_END;
				if (!write_all(mainfd, &cpulimit_msg, 
					    sizeof(cpulimit_cmd))) 
				{
				    return die(cmdSocket, mainfd, password, 
					    cpulimit_pid, signal_fd, sockets);
				}

				stop = true;

				response_field = OK;

				break;
			    default:
				// Ignore invalid commands
				response_field = BAD;
				break;
			    }

			    // Send a response
			    string response = response_field + END;
			    if( !send_all(i, 
					const_cast<char *>(response.c_str()), 
					response.length()))
			    {
				return die(cmdSocket, mainfd, password, 
					cpulimit_pid, signal_fd, sockets);
			    }

			    // Move to the next command
			    if( sizeCmd  >= static_cast<int>(in_cmd.length()-1) ) {
				in_cmd.clear();
			    }else{
				in_cmd = in_cmd.substr(sizeCmd);
			    }
			}else{
			    daemon_log(LOG_INFO, "incomplete conversation");
			}
		    } while (sizeCmd != 0 && in_cmd.length() > 0);
		}
	    }

	    if( stop ) break;
	}

	// Check that the VM is still running
	if(!stop && !isVMRunning(path, user, password, hostname) ) {
	    daemon_log(LOG_INFO, "VM not running, attempting to restart.");
	    // Restart the VM
	    if( !powerOnVM(path, user, password, hostname) ) {
		return die(cmdSocket, mainfd, password, cpulimit_pid,
			signal_fd, sockets);
	    }

	    // Tell cpulimit to reconnect to the VM's process
	    cpulimit_cmd msg = CPULIMIT_START;
	    if( !write_all(mainfd, &msg, sizeof(cpulimit_cmd)) ) {
		return die(cmdSocket, mainfd, password, cpulimit_pid,
			signal_fd, sockets);
	    }
	}
    }

    daemon_log(LOG_ERR, "vmcontrol stopped.");

    // This is valid shutdown
    return !die(cmdSocket, mainfd, password, cpulimit_pid,
	    signal_fd, sockets);
}

/**
 * Receive a plain-text command
 *
 * socket	The socket to listen on
 * recvBuf	The received command
 *
 * -1, on failure; 0, on closed or reset; size of receive otherwise
 */
int recvCommand(int socket, string &recvBuf) 
{
    const int BLOCK_SIZE = 1024; // 1 KB

    // leave room for null-terminator
    char *recvStr = new char[BLOCK_SIZE+1];

    // Buffer all available data
    int total_recv = 0, num_recv;
    bool done = false;
    while( !done ) {
	num_recv = recv(socket, recvStr, BLOCK_SIZE, MSG_DONTWAIT);

	if( num_recv == -1 ) {
	    if( errno == EAGAIN || errno == EINTR ) {
		done = true;
	    }else if( errno == ECONNRESET ) {
		daemon_log(LOG_INFO, L_LOG_STR("recv"));
		recvBuf.clear();
		delete recvStr;
		return 0;
	    }else{
		daemon_log(LOG_ERR, L_LOG_STR("recv"));
		delete recvStr;
		return -1;
	    }
	}else if( num_recv == 0 ) {
	    // Inform caller of socket shutdown on client-side
	    delete recvStr;
	    return 0;
	}else{
	    recvStr[num_recv] = '\0';
	    recvBuf += recvStr;
	    total_recv += num_recv;
	}
    }

    delete recvStr;
    return total_recv;
}

/**
 * Parse a single token and its value (see format at beginning of file)
 *
 * in_cmd	The command to parse
 * cmd_token	The token to look for
 * cmd_value	The value of the token (when found)
 * start_pos	The starting position in the command
 * 
 * the position of the value end; -1, on failure; 0, when the command is incomplete
 */
int parseCommandToken(string &in_cmd, const string &cmd_token, 
	string &cmd_value, size_t start_pos) 
{
    size_t token_end = in_cmd.find_first_of(" ", start_pos);
    if( token_end == string::npos ) return 0;
    if( in_cmd.substr(start_pos, token_end-start_pos) != cmd_token ) return -1;

    size_t token_val_end = in_cmd.find_first_of(" ", token_end+1);
    if( token_val_end == string::npos ) return 0;

    cmd_value = in_cmd.substr(token_end+1, token_val_end-(token_end+1));

    return token_val_end;
}

/**
 * @return  < 0, if the command is invalid; 0, if the command is incomplete;
 * 	    the length of the command, if the command was parsed correctly
 */
int parseCommand(string &in_cmd, string &user, string &password, vm_cmd &cmd) {
    // Find the end of the command
    int endPos = in_cmd.find(END);
    if( endPos == static_cast<int>(string::npos) ) return 0;

    // The value of the USER field
    int retUser = parseCommandToken(in_cmd, USER, user, 0);
    if( retUser <= 0 || retUser >= endPos ) return -1;

    // The value of the PASS field
    int retPass = parseCommandToken(in_cmd, PASS, password, retUser+1);
    if( retPass <= 0 || retPass >= endPos ) return -1;

    // The value of the CMD field
    std::string cmd_str;
    int retCmd = parseCommandToken(in_cmd, CMD, cmd_str, retPass+1);
    if( retCmd <= 0 || retCmd >= endPos ) return -1;
    cmd = cmd_str[0];

    return endPos + END.length();
}

/**
 * @return  < 0, if the response is invalid; 0, if the response is incomplete;
 * 	    the length of the response, if the response was parsed correctly
 */
int parseResponse(string &response, string &value) {
    // Find the end of the response
    int endPos = response.find(END);
    if( endPos == static_cast<int>(string::npos) ) return 0;

    value = response.substr(0, endPos);

    if( value != OK && value != BAD ) return -1;
    
    return endPos + END.length();
}

/*
 * @return < 0, if there was an error parsing the command; 0, if end of file was
 *         reached; > 0, if successful
 */
int parseConfigLine(FILE *stream, string &option, string &value) {
    string line;

    int c = fgetc(stream);
    while( c != EOF && ((char)c) != '\n' && ((char)c) != '\r') {
	line.append(1, (char)c);
	c = fgetc(stream);
    }
    if( feof(stream) ) return 0;
    else if( ferror(stream) ) return -1;

    int delimPos = line.find(CONFIG_DELIM);
    if( delimPos == static_cast<int>(string::npos) ) return -1;

    option = line.substr(0, delimPos);
    value = line.substr(delimPos+1, line.length() - delimPos);

    return 1;
}

/**
 * Forks the current process and establishes the new process as a daemon
 */
pid_t daemonize() {
    pid_t retPid;
    if( daemon_reset_sigs(-1) < 0 ) {
	daemon_log(LOG_ERR, L_LOG_STR("daemon_reset_sigs"));
	return -1;
    }

    if( daemon_unblock_sigs(-1) < 0 ) {
	daemon_log(LOG_ERR, L_LOG_STR("daemon_unblock_sigs"));
	return -1;
    }

    if( (retPid = daemon_fork()) < 0 ) {
	daemon_log(LOG_ERR, L_LOG_STR("daemon_fork"));
	return -1;
    }else if( !retPid ) {
	// Parent just exits
	
	// Now in daemon
	if( daemon_close_all(-1) < 0 ) {
	    daemon_log(LOG_ERR, L_LOG_STR("daemon_close_all"));
	    return -1;
	}

    }
    return retPid;
}
