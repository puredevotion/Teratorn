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

    perror("system");
    return status;
}

bool startServices(string &errMsg, bool &started) {
    // First, check if the services are already running
    if( !pgrep(const_cast<string&>(AUTHD_CMD)) ) {
	started = true;
	cerr << "VMWare core services not running, now starting..." << endl;
	if( systemExec(const_cast<string&>(SERVICE_START_CORE)) ) {
	    errMsg = "failed to start VMWare core services: " + 
		string(strerror(errno));
	    return false;
	}
    }

    if( !pgrep(const_cast<string&>(HOSTD_CMD)) ) {
	started = true;
	cerr << "VMWare management services not running, now starting..." << endl;
	if( systemExec(const_cast<string&>(SERVICE_START_MGMT)) ) {
	    errMsg = "failed to start VMWare management services: " +
		string(strerror(errno));
	    return false;
	}
    }

    return true;
}

pid_t getVMPid(char *vmPath) {
    // Search for VMWare running processes info
    DIR *vmrun;
    if( (vmrun = opendir(VMRUN_PATH.c_str())) == NULL ) {
	perror("opendir");
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
	perror("malloc");
	return -1;
    }

    do{
	if( readdir_r(vmrun, &entry, resultp) ) {
	    perror("readdir_r");
	    free(resultp);
	    return -1;
	}

	// This requires _BSD_SOURCE (so this isn't super portable)
	if( resultp != NULL && entry.d_type == DT_DIR ) {
	    string newPath = VMRUN_PATH + string(entry.d_name);
	    DIR *vmvmx;
	    if( (vmvmx = opendir(newPath.c_str())) == NULL ) {
		perror("opendir");
		free(resultp);
		return -1;
	    }

	    struct dirent innerEnt;
	    struct dirent **innerResp = 
		(struct dirent **)malloc(sizeof(struct dirent *));

	    if( innerResp == NULL ) {
		perror("malloc");
		return -1;
	    }

	    do {
		if( readdir_r(vmvmx, &innerEnt, innerResp) ) {
		    perror("readdir_r");
		    free(innerResp);
		    free(resultp);
		    return -1;
		}

		if(    innerResp != NULL 
		    && innerEnt.d_type == DT_LNK
		    && strstr(innerEnt.d_name, VMX_CONFIG.c_str()) != NULL )
		{
		    string linkPath = newPath + string("/") + 
			string(innerEnt.d_name);

		    struct stat linkStat;

		    if( stat(linkPath.c_str(), &linkStat) ) {
			perror("stat");
			free(innerResp);
			free(resultp);
			return -1;
		    }

		    char *linkTarget = (char *)malloc(linkStat.st_size);

		    if( linkTarget == NULL ) {
			perror("malloc");
			return -1;
		    }

		    ssize_t linkSize;
		    if( (linkSize = readlink(linkPath.c_str(), linkTarget,
				    linkStat.st_size)) == -1 ) 
		    {
			perror("readlink");
			free(innerResp);
			free(resultp);
			free(linkTarget);
			return -1;
		    }

		    // Found the VM, the directory entry contains the PID
		    char *parsePtr = strstr(entry.d_name, VMX_PID_DELIM.c_str());

		    // This means the directory structure has changed
		    assert(parsePtr != NULL);

		    // Skip '_'
		    parsePtr++;

		    free(linkTarget);
		    free(innerResp);
		    free(resultp);
		    return strtol(parsePtr, NULL, 10);
		} 
	    }while(innerResp != NULL);
	    free(innerResp);
	}
    }while(resultp != NULL);
    free(resultp);

    return -1;
}

char *get_password() {
    // Turn off echo
    struct termios orig, no_echo;

    if( tcgetattr(fileno(stdin), &orig) == -1 ) {
	perror("tcgetattr");
	return NULL;
    }

    no_echo = orig;
    no_echo.c_lflag &= ~ECHO;

    if( tcsetattr(fileno(stdin), TCSAFLUSH, &no_echo) == -1 ) {
	perror("tcsetattr");
	return NULL;
    }

    // Read in the password
    char *password = (char *)malloc(PASS_INIT_SIZE*sizeof(char));

    if( password == NULL ) {
	perror("malloc");
	return NULL;
    }

    int c, size = -1, prev_size = PASS_INIT_SIZE;
    do {
	c = getchar();
	size++;
	if( size >= prev_size ) password = (char *)realloc(password, size*2);

	if( password == NULL ) {
	    perror("realloc");
	    return NULL;
	}

	password[size]= (char)c;
    }while( password[size] != '\0' && c != EOF && !isspace(c) );

    // Turn echo back on
    if( tcsetattr(fileno(stdin), TCSAFLUSH, &orig) == -1 ) {
	perror("tcsetattr");
	if(password) free(password);
	return NULL;
    }

    password[size] = '\0';

    return password;
}

int write_all(int fd, void *buf, int size) {
    int num_write = 0, length;
    while( num_write < size ) {
        length = write(fd, buf, size - num_write);

        // Attempt to finish write if the call was interrupted
        if( length == -1 && errno != EINTR ) {
            return 0;
        }else if( length != -1 ) num_write += length;
    }

    return 1;
}

int read_all(int fd, void *buf, int size) {
    int num_read = 0, length = -1;
    while( num_read < size && 
	   length != 0 ) 
    {
	length = read(fd, buf, size - num_read);

	if( length == 0 ) {
	    return 0;
	}

	if( length == -1 && errno != EINTR ) {
	    return -1;
	}else if( length != -1 ) num_read += length;
    }

    return 1;
}

void termChild(pid_t pid, char *desc) {
    if( pid > 0 ) {
	if( kill(pid, SIGINT) == -1 ) {
	    if( errno != ESRCH ) { // ESRCH == no such child, it already finished
		perror("kill");
	    }
	    return;
	}

	int status;
	if( waitpid(pid, &status, 0) == -1 ) {
	    perror("waitpid");
	    return;
	}

	if( WIFSIGNALED(status) ) {
#ifdef WCOREDUMP
	    if( WCOREDUMP(status) ) {
		cerr << desc << " dumped core." << endl;
	    }else{
		cerr << desc << " terminated by: " << strsignal(WTERMSIG(status))
		     << endl;
	    }
#else
	    cerr << desc << " terminated by: " << strsignal(WTERMSIG(status))
		 << endl;
#endif
	}else if( WIFEXITED(status) ) {
	    cerr << desc << " exited[ " << WEXITSTATUS(status) << "]" << endl;
	}
    }
}
