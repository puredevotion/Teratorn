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
#include <cstdio>
#include <cerrno>
#include <cstdlib>
using std::cout;
using std::cerr;
using std::endl;
using std::string;

#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>

// Project includes
#include "cpulimit.h"
#include "vixint.h"
#include "util.h"

enum vm_cmd {
    VM_UP,
    VM_DOWN
};

// TODO daemonize, syslog, config file, client

static const char *USAGE = " [-H hostname] [-U user] [-l limit] <-u|-d> <vmpath>";
static const char *OPT_STR = "H:udU:l:";
static const char *DEFAULT_HOST = "http://127.0.0.1:8222/sdk";
static const int CHECK_VM_PERIOD = 60; // seconds
static const char *CMD_PATH = "/var/run/vmware/vmcontrol.cmd=";
static const char *CMD_PATH_REAL = "/var/run/vmware/vmcontrol.cmd";

static int sigint = 0;

static void sighandle(int signum) {
    switch(signum) {
	case SIGINT:
	    fprintf(stderr, "SIGINT received\n");
	    sigint = 1;
	    break;
	case SIGPIPE:
	    fprintf(stderr, "SIGPIPE received\n");
	    sigint = 1;
	    break;
	default:
	    fprintf(stderr, "%s received\n", strsignal(signum));
	    break;
    }
}

void die(int cmdSocket, int main_pipe, char *password, int cpulimit_pid) {
    // Zero the password
    int i = 0;
    while( password[i] != '\0' ) {
	password[i] = '\0';
	i++;
    }
    free(password);

    // Remove the socket file
    if( cmdSocket > 0 ) {
	// Errors don't matter, the file is being deleted
	if( close(cmdSocket) ) {
	    perror("close");
	}

	if( unlink(CMD_PATH_REAL) ) {
	    perror("unlink");
	}
    }

    // Alert the cpulimit process about termination
    if( main_pipe > 0 ) {
	// Errors don't matter, the process is terminating
	close(main_pipe);
    }

    // Tell cpulimit process to stop
    if( cpulimit_pid > 0 ) {
	kill(cpulimit_pid, SIGINT);
    }

    exit(1);
}

int launchCPULimit(pid_t pid, int limit, int *cpulimitfd, int *mainfd) {
    int cpulimit_pipe[2];
    int main_pipe[2];
    pid_t cpulimit_pid;

    if( pipe(cpulimit_pipe) == -1 ) {
	perror("pipe");
	return -1;
    }

    if( pipe(main_pipe) == -1 ) {
	perror("pipe");
	return -1;
    }

    if( (cpulimit_pid = fork()) == -1 ) {
	perror("fork");
	return -1;
    }

    if( cpulimit_pid == 0 ) {
	// In cpulimit process
	int cpulimit, main;
	
	// writes to its pipe
	if( close(cpulimit_pipe[0]) ) {
	    perror("close");

	    // Try to let main process know
	    if( close(cpulimit_pipe[1]) ) {
		perror("close");
		cerr << "cpulimit process failed. Bailing..." << endl;
		exit(1);
	    }
	}

	cpulimit = cpulimit_pipe[1];

	// reads from the main process' pipe
	if( close(main_pipe[1]) ) {
	    perror("close");

	    // Try to let main process know
	    if( close(cpulimit_pipe[1]) ) { 
		perror("close");
		cerr << "cpulimit process failed. Bailing..." << endl;
		exit(1);
	    }
	}

	main = main_pipe[0];

	cpulimit_cmd msg = CPULIMIT_START;
	do{
	    if( msg == CPULIMIT_START ) 
		cpulimit_bypid(1, 1, limit, pid);

	    cout << "cpulimit failed." << endl;

	    // If we get here, it means cpulimit died for some reason so alert
	    // the main process
	    
	    msg = CPULIMIT_STOPPED;
	    if( !write_all(cpulimit, &msg, sizeof(cpulimit_cmd)) ) {
		perror("write_all");

		close(cpulimit);
		cerr << "cpulimit process failed. Bailing..." << endl;
		exit(1);
	    }

	    // Wait for command from main process
	    int read_result = read_all(main, &msg, sizeof(cpulimit_cmd));
	    if( read_result == 0 ) {
		msg = CPULIMIT_END;
	    }else if( read_result == -1 ) {
		perror("read_all");

		close(cpulimit);
		cerr << "cpulimit process failed. Bailing..." << endl;
		exit(1);
	    }
	}while(msg != CPULIMIT_END );

	cerr << "cpulimit received end command. Stopping..." << endl;
	exit(1);
    }else{
	// In main process
	
	// writes to its pipe
	if( close(main_pipe[0]) ) {
	    perror("close");

	    // Try to let cpulimit process know
	    if( close(main_pipe[1]) ) {
		perror("close");
		cerr << "main process failed. Bailing..." << endl;
		return -1;
	    }
	}

	// reads from cpulimit's pipe
	if( close(cpulimit_pipe[1]) ) {
	    perror("close");

	    // Try to let cpulimit process know
	    if( close(main_pipe[1]) ) { 
		perror("close");
		cerr << "main process failed. Bailing..." << endl;
		return -1;
	    }
	}

	*mainfd = main_pipe[1];
	*cpulimitfd = cpulimit_pipe[0];

	return cpulimit_pid;
    }

    return -1;
}

int controlLoop(pid_t vmPid, int limit, int checkVMPeriod, char *path,
	char *user, char *password, char *hostname) 
{
    int cpulimitfd, mainfd, cpulimit_pid;
    cpulimit_pid = launchCPULimit(vmPid, limit, &cpulimitfd, &mainfd);
    if( cpulimit_pid == -1 ) {
	cerr << "Failed to launch cpulimit process." << endl;
	return 0;
    }

    // Set up the unix domain socket
    int cmdSocket;
    if( (cmdSocket = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1 ) {
	perror("socket");
	die(-1, mainfd, password, cpulimit_pid);
    }

    int trueInt = 1;
    if( (setsockopt(cmdSocket,SOL_SOCKET,SO_REUSEADDR,&trueInt, sizeof(int))) == -1 ) {
	perror("setsockopt");
	die(cmdSocket, mainfd, password, cpulimit_pid);
    }

    struct sockaddr_un sockPath;

    bzero(&sockPath, sizeof(struct sockaddr_un));

    sockPath.sun_family = AF_UNIX;
    strncpy((char *)&(sockPath.sun_path), CMD_PATH, strlen(CMD_PATH) - 1);

    if( bind(cmdSocket, (struct sockaddr *)&sockPath,
		sizeof(struct sockaddr_un)) == -1 ) {
	perror("bind");
	die(cmdSocket, mainfd, password, cpulimit_pid);
    }

    struct sigaction sa;
    sa.sa_handler = sighandle;
    sigemptyset(&sa.sa_mask);
    sigaddset(&sa.sa_mask, SIGINT);
    sigaddset(&sa.sa_mask, SIGPIPE);
    sa.sa_flags = SA_RESETHAND | SA_RESTART;

    if( sigaction(SIGINT, &sa, NULL) == -1 ) {
	perror("sigaction");
	die(cmdSocket, mainfd, password, cpulimit_pid);
    }

    if( sigaction(SIGPIPE, &sa, NULL) == -1 ) {
	perror("sigaction");
	die(cmdSocket, mainfd, password, cpulimit_pid);
    }

    // Set up timeout for select
    struct timeval vmcheck_to;

    // set up state for select
    fd_set saved, read_fds;
    int maxFd = ( cpulimitfd > cmdSocket ) ? cpulimitfd : cmdSocket;
    FD_ZERO(&saved);
    FD_SET(cmdSocket, &saved);
    FD_SET(cpulimitfd, &saved);

    // Wait for commands from named socket or cpulimit pipe
    // Periodically, check that VM is still running
    bool stop = false;
    while(!sigint && !stop) {
	cout << "Waiting for commands..." << endl;
	int select_ret;

	read_fds = saved;
	vmcheck_to.tv_sec = checkVMPeriod;
	vmcheck_to.tv_usec = 0;
	select_ret = select(maxFd + 1, &read_fds, NULL, NULL, &vmcheck_to);

	if( select_ret == -1 ) {
	    if( errno == EINTR ) {
		cerr << "main: interrupted call to select" << endl;
		continue;
	    }else{
		perror("select");
		return 0;
	    }
	}

	if(FD_ISSET(cmdSocket, &read_fds)) {
	    cout << "Received command." << endl;

	    // Handle socket command
	    vm_cmd msg;
	    if( !read_all(cmdSocket, &msg, sizeof(vm_cmd)) ) {
		die(cmdSocket, mainfd, password, cpulimit_pid);
	    }

	    cpulimit_cmd cpulimit_msg;
	    switch(msg) {
		case VM_UP:
		    if( !powerOnVM(path, user, password, hostname) ) {
			die(cmdSocket, mainfd, password, cpulimit_pid);
		    }

		    // Queue a start event for cpulimit 
		    cpulimit_msg = CPULIMIT_START;
		    if( !write_all(mainfd, &cpulimit_msg, sizeof(cpulimit_cmd)) ) {
			die(cmdSocket, mainfd, password, cpulimit_pid);
		    }

		    break;
		case VM_DOWN:
		    if( !powerOffVM(path, user, password, hostname) ) {
			die(cmdSocket, mainfd, password, cpulimit_pid);
		    }

		    // Queue a stop event for cpulimit
		    cpulimit_msg = CPULIMIT_END;
		    if( !write_all(mainfd, &msg, sizeof(cpulimit_cmd)) ) {
			die(cmdSocket, mainfd, password, cpulimit_pid);
		    }

		    stop = true;

		    break;
		default:
		    // Ignore unknown command
		    break;
	    }
	}

	if(!stop && FD_ISSET(cpulimitfd, &read_fds)) {
	    cout << "Received command from cpulimit." << endl;
	    // Handle cpulimit event
	    cpulimit_cmd msg;
	    if( !read_all(cpulimitfd, &msg, sizeof(cpulimit_cmd)) ) {
		die(cmdSocket, mainfd, password, cpulimit_pid);
	    }

	    if(   msg == CPULIMIT_STOPPED 
	       && !isVMRunning(path, user, password, hostname) ) 
	    {
		if( !powerOnVM(path, user, password, hostname) ) {
		    die(cmdSocket, mainfd, password, cpulimit_pid);
		}

		msg = CPULIMIT_START;
		if( !write_all(mainfd, &msg, sizeof(cpulimit_cmd)) ) {
		    die(cmdSocket, mainfd, password, cpulimit_pid);
		}
	    }
	}

	// Check that the VM is still running
	if( !isVMRunning(path, user, password, hostname) ) {
	    cout << "VM not running, attempting to restart" << endl;
	    // Restart the VM
	    if( !powerOnVM(path, user, password, hostname) ) {
		die(cmdSocket, mainfd, password, cpulimit_pid);
	    }

	    // Tell cpulimit to reconnect to the VM's process
	    cpulimit_cmd msg = CPULIMIT_START;
	    if( !write_all(mainfd, &msg, sizeof(cpulimit_cmd)) ) {
		die(cmdSocket, mainfd, password, cpulimit_pid);
	    }
	}
    }

    die(cmdSocket, mainfd, password, cpulimit_pid);

    return 0;
}

int main(int argc, char *argv[]) {
    bool powerOn = false;
    bool powerOff = false;
    char *user = NULL;
    char *password = NULL;
    char *hostname = NULL;
    char *path = NULL;
    int limit = 0;

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
	    case 'l':
		limit = strtol(optarg, NULL, 10);
		break;
	    default:
		cerr << "Invalid Arg: " << (char)arg << endl 
		     << "Usage: " << argv[0] << USAGE << endl;
		return 1;
	}
    }

    if( (!powerOn && !powerOff) || !user ) {
	cerr << "Usage: " << argv[0] << USAGE << endl;
	return 1;
    }

    if( limit < 1 || limit > 100 ) {
	cerr <<  "Limit must be in range 1-100%" << endl;
	cerr << "Usage: " << argv[0] << USAGE << endl;
	return 1;
    }

    if( hostname == NULL ) {
	hostname = const_cast<char *>(DEFAULT_HOST);
    }

    if( powerOn && powerOff ) {
	cerr << "Ambiguous option: power on or power off?" << endl;
	cerr << "Usage: " << argv[0] << USAGE << endl;
	return 1;
    }


    path = argv[optind];

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

    // Ask for the password
    cout << user << "'s password: ";
    password = get_password();
    cout << endl;
    if( password == NULL ) {
	return 1;
    }

    if( powerOff ) {
	    if( !powerOffVM(path, user, password, hostname) ) return 1;

	    // Zero the password
	    int i = 0;
	    while( password[i] != '\0' ) {
		password[i] = '\0';
		i++;
	    }
	    free(password);
    }else if( powerOn ) {
	    // Need to start the daemon

	    if( !powerOnVM(path, user, password, hostname) ) return 1;
	    
	    pid_t vmPid = getVMPid(path);

	    if( vmPid == -1 ) {
		cerr << "Could not determine PID of newly launched virtual "
		     << "machine...failing." << endl;
		return 1;
	    }

	    cout << "VM PID: " << vmPid << endl;

	    return !controlLoop(vmPid, limit, CHECK_VM_PERIOD, path, user,
		    password, hostname);
    }

    return 0;
}
