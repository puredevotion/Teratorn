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

#include <cstdlib>
#include <cerrno>
#include <cstring>
#include <cassert>
#include <cstdlib>
#include <iostream>
using std::string;
using std::cout;
using std::cerr;
using std::endl;

#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <signal.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <libdaemon/dlog.h>

#include "util.h"

static const string SERVICE_START_MGMT = "/etc/init.d/vmware-mgmt restart";
static const string SERVICE_START_CORE = "/etc/init.d/vmware-core restart";
static const string HOSTD_CMD = "vmware-hostd";
static const string AUTHD_CMD = "vmware-authd";

static const string PGREP = "/usr/bin/pgrep ";

static const string VMRUN_PATH = "/var/run/vmware/root_0/";
static const string VMX_CONFIG = "configFile";
static const string VMX_PID_DELIM = "_";

static const int PASS_INIT_SIZE = 25;

bool pgrep(string &command) {
    string total = PGREP + command;
    return (systemExec(total) == 0);
}

int systemExec(string &command) {
    string total = command + string(" >& /dev/null");

    int status = system(total.c_str());
    if( status != -1 ) {
	return WEXITSTATUS(status);
    }

    daemon_log(LOG_ERR, L_LOG_STR("system"));
    return -1;
}

bool startServices(string &errMsg, bool &started) {
    // First, check if the services are already running
    if( !pgrep(const_cast<string&>(AUTHD_CMD)) ) {
	started = true;
	daemon_log(LOG_INFO, "VMWare core services not running, restarting...");
	if( systemExec(const_cast<string&>(SERVICE_START_CORE)) ) {
	    errMsg = "failed to start VMWare core services: ";
	    return false;
	}
    }

    if( !pgrep(const_cast<string&>(HOSTD_CMD)) ) {
	started = true;
	daemon_log(LOG_INFO, 
		"VMWare management services not running, restarting...");
	if( systemExec(const_cast<string&>(SERVICE_START_MGMT)) ) {
	    errMsg = "failed to start VMWare management services: ";
	    return false;
	}
    }

    return true;
}

pid_t getVMPid(char *vmPath) {
    // Search for VMWare running processes info
    DIR *vmrun;
    if( (vmrun = opendir(VMRUN_PATH.c_str())) == NULL ) {
	daemon_log(LOG_ERR, L_LOG_STR("opendir"));
	return -1;
    }

    /* The directory structure is:
     * VMRUN_PATH/timestamp_PID1/configFile -> .vmx file for VM
     * 		 /timestamp_PID2/ -> for other non-VM process
     * 		 ...
     */
 
    struct dirent entry;
    struct dirent **resultp = (struct dirent **)malloc(sizeof(struct dirent *));

    if( resultp == NULL ) {
	daemon_log(LOG_ERR, L_LOG_STR("malloc"));
	closedir(vmrun);
	return -1;
    }

    do{
	if( readdir_r(vmrun, &entry, resultp) ) {
	    daemon_log(LOG_ERR, L_LOG_STR("readdir_r"));
	    free(resultp);
	    closedir(vmrun);
	    return -1;
	}

	// This requires _BSD_SOURCE (so this isn't super portable)
	if( resultp != NULL && entry.d_type == DT_DIR ) {
	    string newPath = VMRUN_PATH + string(entry.d_name);
	    DIR *vmvmx;
	    if( (vmvmx = opendir(newPath.c_str())) == NULL ) {
		daemon_log(LOG_ERR, L_LOG_STR("opendir"));
		free(resultp);
		closedir(vmrun);
		return -1;
	    }

	    struct dirent innerEnt;
	    struct dirent **innerResp = 
		(struct dirent **)malloc(sizeof(struct dirent *));

	    if( innerResp == NULL ) {
		daemon_log(LOG_ERR, L_LOG_STR("malloc"));
		closedir(vmrun);
		closedir(vmvmx);
		return -1;
	    }

	    do {
		if( readdir_r(vmvmx, &innerEnt, innerResp) ) {
		    daemon_log(LOG_ERR, L_LOG_STR("readdir_r"));
		    break;
		}

		if(    *innerResp != NULL 
		    && innerEnt.d_type == DT_LNK
		    && strstr(innerEnt.d_name, VMX_CONFIG.c_str()) != NULL )
		{
		    string linkPath = newPath + string("/") + 
			string(innerEnt.d_name);

		    struct stat linkStat;

		    if( stat(linkPath.c_str(), &linkStat) ) {
			daemon_log(LOG_ERR, L_LOG_STR("stat"));
			free(innerResp);
			free(resultp);
			closedir(vmrun);
			closedir(vmvmx);
			return -1;
		    }

		    char *linkTarget = (char *)malloc(linkStat.st_size);

		    if( linkTarget == NULL ) {
			daemon_log(LOG_ERR, L_LOG_STR("stat"));
			closedir(vmrun);
			closedir(vmvmx);
			return -1;
		    }

		    ssize_t linkSize;
		    if( (linkSize = readlink(linkPath.c_str(), linkTarget,
				    linkStat.st_size)) == -1 ) 
		    {
			daemon_log(LOG_ERR, L_LOG_STR("readlink"));
			free(innerResp);
			free(resultp);
			free(linkTarget);
			closedir(vmrun);
			closedir(vmvmx);
			return -1;
		    }

		    // Found the VM, the directory entry contains the PID
		    char *parsePtr = strstr(entry.d_name, 
			    VMX_PID_DELIM.c_str());

		    // This means the directory structure has changed
		    assert(parsePtr != NULL);

		    // Skip '_'
		    parsePtr++;

		    free(linkTarget);
		    free(innerResp);
		    free(resultp);
		    closedir(vmrun);
		    closedir(vmvmx);
		    return strtol(parsePtr, NULL, 10);
		} 
	    }while(*innerResp != NULL);
	    free(innerResp);
	    closedir(vmvmx);
	}
    }while(resultp != NULL);
    free(resultp);
    closedir(vmrun);

    return -1;
}

char *get_password() {
    // Turn off echo
    struct termios orig, no_echo;

    if( tcgetattr(fileno(stdin), &orig) == -1 ) {
	daemon_log(LOG_ERR, L_LOG_STR("tcgettr"));
	return NULL;
    }

    no_echo = orig;
    no_echo.c_lflag &= ~ECHO;

    if( tcsetattr(fileno(stdin), TCSAFLUSH, &no_echo) == -1 ) {
	daemon_log(LOG_ERR, L_LOG_STR("tcsetattr"));
	return NULL;
    }

    // Read in the password
    char *password = (char *)malloc(PASS_INIT_SIZE*sizeof(char));

    if( password == NULL ) {
	daemon_log(LOG_ERR, L_LOG_STR("malloc"));
	return NULL;
    }

    int c, pos = -1, size = PASS_INIT_SIZE;
    do {
	c = getchar();
	pos++;
	if( (pos+1) >= size ) {
	    size *= 2;
	    password = (char *)realloc(password, size);
	}

	if( password == NULL ) {
	    daemon_log(LOG_ERR, L_LOG_STR("realloc"));
	    return NULL;
	}

	password[pos]= (char)c;
    }while( c != EOF && password[pos] != '\0' && !isspace(c) );

    // Turn echo back on
    if( tcsetattr(fileno(stdin), TCSAFLUSH, &orig) == -1 ) {
	daemon_log(LOG_ERR, L_LOG_STR("tcsetattr"));
	if(password) free(password);
	return NULL;
    }

    password[pos] = '\0';

    return password;
}

void zero_string(char *str) {
    while( *str != '\0' ) {
	*str = '\0';
	str++;
    }
}

int write_all(int fd, void *buf, int size) {
    char *lbuf = static_cast<char *>(buf);
    int num_write = 0, length;
    while( num_write < size ) {
        length = write(fd, lbuf + num_write, size - num_write);

        // Attempt to finish write if the call was interrupted
        if( length == -1 && errno != EINTR ) {
	    daemon_log(LOG_ERR, L_LOG_STR("write"));
            return 0;
        }else if( length != -1 ) num_write += length;
    }

    return 1;
}

int send_all(int socket, void *buf, int length) {
    char *lbuf = static_cast<char *>(buf);
    int total_sent = 0, num_sent = -1;
    while( total_sent < length && num_sent != 0 ) {
	num_sent = send(socket, lbuf + total_sent,
		length - total_sent, 0);

	if( num_sent == -1 && errno != EINTR ) {
	    daemon_log(LOG_ERR, L_LOG_STR("send"));
	    return 0;
	}else if( num_sent != -1 ) total_sent += num_sent;
    }

    return 1;
}

int read_all(int fd, void *buf, int size) {
    char *lbuf = static_cast<char *>(buf);
    int num_read = 0, length = -1;
    while( num_read < size && length != 0 ) {
	length = read(fd, lbuf + num_read, size - num_read);

	if( length == 0 ) {
	    return 0;
	}

	if( length == -1 && errno != EINTR ) {
	    daemon_log(LOG_ERR, L_LOG_STR("read"));
	    return -1;
	}else if( length != -1 ) num_read += length;
    }

    return 1;
}

int recv_all(int socket, void *buf, int size) {
    char *lbuf = static_cast<char *>(buf);
    int total_read = 0, num_read = -1;
    while( num_read < size && num_read != 0 ) {
	num_read = recv(socket, lbuf + total_read, size - total_read, 0);

	if( num_read == 0 ) {
	    return 0;
	}

	if( num_read == -1 && errno != EINTR ) {
	    daemon_log(LOG_ERR, L_LOG_STR("recv"));
	    return -1;
	}else if( num_read != -1 ) total_read += num_read;
    }

    return 1;
}

int get_socket(struct addrinfo *addr_list, bool should_bind) {
    int sock_fd;
    if( ( sock_fd = socket(addr_list->ai_family, addr_list->ai_socktype,
		    addr_list->ai_protocol) ) == -1 ) {
	daemon_log(LOG_ERR, L_LOG_STR("socket"));
	return -1; 
    }

    daemon_log(LOG_INFO, "Socket created: fd=%d", sock_fd);

    int trueval = 1;
    if (setsockopt(sock_fd,SOL_SOCKET,SO_REUSEADDR,&trueval,
		sizeof(int)) == -1) 
    {
	daemon_log(LOG_ERR, L_LOG_STR("setsockopt"));
	return -1;
    }

    if(should_bind) { 
	if( bind(sock_fd, addr_list->ai_addr, addr_list->ai_addrlen) == -1 ) {
	    daemon_log(LOG_ERR, L_LOG_STR("bind"));
	    return -1;
	}

	struct sockaddr_in *addr = (struct sockaddr_in *) addr_list->ai_addr;
	daemon_log(LOG_INFO, "Bound to %d on %s",
		ntohs(addr->sin_port), inet_ntoa(addr->sin_addr));
    }

    return sock_fd;
}
