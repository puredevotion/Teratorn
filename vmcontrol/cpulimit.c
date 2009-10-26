/**
 * Copyright (c) 2005, by:      Angelo Marletta <marlonx80@hotmail.com>
 *
 * This file may be used subject to the terms and conditions of the
 * GNU Library General Public License Version 2, or any later version
 * at your option, as published by the Free Software Foundation.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 **********************************************************************
 *
 * Simple program to limit the cpu usage of a process
 * If you modify this code, send me a copy please
 *
 * Author:  Angelo Marletta
 * Date:    26/06/2005
 * Version: 1.1
 * Last version at: http://marlon80.interfree.it/cpulimit/index.html
 *
 * Changelog:
 *  - Fixed a segmentation fault if controlled process exited in particular circumstances
 *  - Better CPU usage estimate
 *  - Fixed a <0 %CPU usage reporting in rare cases
 *  - Replaced MAX_PATH_SIZE with PATH_MAX already defined in <limits.h>
 *  - Command line arguments now available
 *  - Now is possible to specify target process by pid
 */

/*
 * Date: 07/10/2009
 *
 * Modified so cpulimit could be used by other C programs.
 *
 * Date: 19/10/2009
 *
 * Modified to use daemon logging. 
 * 
 * This file is part of Teratorn.
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/resource.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>

// teratorn additions
#include <libdaemon/dlog.h>
#include "util.h"
#include "cpulimit.h"

//kernel time resolution (inverse of one jiffy interval) in Hertz
//i don't know how to detect it, then define to the default (not very clean!)
#define HZ 100

//some useful macro
#define min(a,b) (a<b?a:b)
#define max(a,b) (a>b?a:b)

//pid of the controlled process
int pid = 0;
//verbose mode
int verbose = 0;
//lazy mode
int lazy = 0;

static int sigint = 0;

//return ta-tb in microseconds (no overflow checks!)
inline long timediff(const struct timespec *ta, const struct timespec *tb)
{
    unsigned long us =
	(ta->tv_sec - tb->tv_sec) * 1000000 + (ta->tv_nsec / 1000 -
					       tb->tv_nsec / 1000);
    return us;
}

int waitforpid(int pid)
{
    //switch to low priority
    if (setpriority(PRIO_PROCESS, getpid(), 19) != 0) {
	daemon_log(LOG_WARNING, L_LOG_STR("setpriority"));
    }

    int i = 0;

    while (1) {

	DIR *dip;
	struct dirent *dit;

	//open a directory stream to /proc directory
	if ((dip = opendir("/proc")) == NULL) {
	    daemon_log(LOG_ERR, L_LOG_STR("opendir"));
	    return -1;
	}
	//read in from /proc and seek for process dirs
	while ((dit = readdir(dip)) != NULL) {
	    //get pid
	    if (pid == atoi(dit->d_name)) {
		//pid detected
		if (kill(pid, SIGSTOP) == 0 && kill(pid, SIGCONT) == 0) {
		    //process is ok!
		    goto done;
		} else {
		    daemon_log(LOG_ERR, "error: process %d detected, but you don't have permission to control it.",
			    pid);
		}
	    }
	}

	//close the dir stream and check for errors
	if (closedir(dip) == -1) {
	    daemon_log(LOG_ERR, L_LOG_STR("closedir"));
	    return -1;
	}
	//no suitable target found
	if (i++ == 0) {
	    if (lazy) {
		daemon_log(LOG_ERR, "No process found");
		return -1;
	    } else {
		daemon_log(LOG_WARNING, "no target process found. Waiting for it...");
	    }
	}
	//sleep for a while
	sleep(2);
    }

  done:
    daemon_log(LOG_INFO, "Process %d detected", pid);
    //now set high priority, if possible
    if (setpriority(PRIO_PROCESS, getpid(), -20) != 0) {
	daemon_log(LOG_WARNING, L_LOG_STR("setpriority"));
    }
    return 0;

}

//this function periodically scans process list and looks for executable path names
//it should be executed in a low priority context, since precise timing does not matter
//if a process is found then its pid is returned
//process: the name of the wanted process, can be an absolute path name to the executable file
//         or simply its name
//return: pid of the found process
int getpidof(const char *process)
{

    //set low priority
    if (setpriority(PRIO_PROCESS, getpid(), 19) != 0) {
	daemon_log(LOG_WARNING, L_LOG_STR("setpriority"));
    }

    char exelink[20];
    char exepath[PATH_MAX + 1];
    int pid = 0;
    int i = 0;

    while (1) {

	DIR *dip;
	struct dirent *dit;

	//open a directory stream to /proc directory
	if ((dip = opendir("/proc")) == NULL) {
	    daemon_log(LOG_ERR, L_LOG_STR("opendir"));
	    return -1;
	}
	//read in from /proc and seek for process dirs
	while ((dit = readdir(dip)) != NULL) {
	    //get pid
	    pid = atoi(dit->d_name);
	    if (pid > 0) {
		sprintf(exelink, "/proc/%d/exe", pid);
		int size = readlink(exelink, exepath, sizeof(exepath));
		if (size > 0) {
		    int found = 0;
		    if (process[0] == '/'
			&& strncmp(exepath, process, size) == 0
			&& ((size_t) size) == strlen(process)) {
			//process starts with / then it's an absolute path
			found = 1;
		    } else {
			//process is the name of the executable file
			if (strncmp
			    (exepath + size - strlen(process), process,
			     strlen(process)) == 0) {
			    found = 1;
			}
		    }
		    if (found == 1) {
			if (kill(pid, SIGSTOP) == 0
			    && kill(pid, SIGCONT) == 0) {
			    //process is ok!
			    goto done;
			} else {
			    daemon_log(LOG_ERR, "error: process %d detected, but you don't have permission to control it",
				    pid);
			}
		    }
		}
	    }
	}

	//close the dir stream and check for errors
	if (closedir(dip) == -1) {
	    daemon_log(LOG_ERR, L_LOG_STR("closedir"));
	    return -1;
	}
	//no suitable target found
	if (i++ == 0) {
	    if (lazy) {
		daemon_log(LOG_ERR, "No process found.");
		return -1;
	    } else {
		daemon_log(LOG_WARNING, "no target process found. Waiting for it...");
	    }
	}
	//sleep for a while
	sleep(2);
    }

  done:
    daemon_log(LOG_INFO, "Process %d detected", pid);
    //now set high priority, if possible
    if (setpriority(PRIO_PROCESS, getpid(), -20) != 0) {
	daemon_log(LOG_WARNING, L_LOG_STR("setpriority"));
    }
    return pid;

}

//SIGINT and SIGTERM signal handler
void quit(int sig)
{
    sigint = 1;
}

//get jiffies count from /proc filesystem
int getjiffies(int pid)
{
    static char stat[20];
    static char buffer[1024];
    sprintf(stat, "/proc/%d/stat", pid);
    FILE *f = fopen(stat, "r");
    if (f == NULL)
	return -1;
    fgets(buffer, sizeof(buffer), f);
    fclose(f);
    char *p = buffer;
    p = (char *) memchr(p + 1, ')', sizeof(buffer) - (p - buffer));
    int sp = 12;
    while (sp--)
	p = (char *) memchr(p + 1, ' ', sizeof(buffer) - (p - buffer));
    //user mode jiffies
    int utime = atoi(p + 1);
    p = (char *) memchr(p + 1, ' ', sizeof(buffer) - (p - buffer));
    //kernel mode jiffies
    int ktime = atoi(p + 1);
    return utime + ktime;
}

//process instant photo
struct process_screenshot {
    struct timespec when;	//timestamp
    int jiffies;		//jiffies count of the process
    int cputime;		//microseconds of work from previous screenshot to current
};

//extracted process statistics
struct cpu_usage {
    float pcpu;
    float workingrate;
};

//this function is an autonomous dynamic system
//it works with static variables (state variables of the system), that keep memory of recent past
//its aim is to estimate the cpu usage of the process
//to work properly it should be called in a fixed periodic way
//perhaps i will put it in a separate thread...
int compute_cpu_usage(int pid, int last_working_quantum,
		      struct cpu_usage *pusage)
{
#define MEM_ORDER 10
    //circular buffer containing last MEM_ORDER process screenshots
    static struct process_screenshot ps[MEM_ORDER];
    //the last screenshot recorded in the buffer
    static int front = -1;
    //the oldest screenshot recorded in the buffer
    static int tail = 0;

    if (pusage == NULL) {
	//reinit static variables
	front = -1;
	tail = 0;
	return 0;
    }
    //let's advance front index and save the screenshot
    front = (front + 1) % MEM_ORDER;
    int j = getjiffies(pid);
    if (j >= 0)
	ps[front].jiffies = j;
    else
	return -1;		//error: pid does not exist
    clock_gettime(CLOCK_REALTIME, &(ps[front].when));
    ps[front].cputime = last_working_quantum;

    //buffer actual size is: (front-tail+MEM_ORDER)%MEM_ORDER+1
    int size = (front - tail + MEM_ORDER) % MEM_ORDER + 1;

    if (size == 1) {
	//not enough samples taken (it's the first one!), return -1
	pusage->pcpu = -1;
	pusage->workingrate = 1;
	return 0;
    } else {
	//now we can calculate cpu usage, interval dt and dtwork are expressed in microseconds
	long dt = timediff(&(ps[front].when), &(ps[tail].when));
	long dtwork = 0;
	int i = (tail + 1) % MEM_ORDER;
	int max = (front + 1) % MEM_ORDER;
	do {
	    dtwork += ps[i].cputime;
	    i = (i + 1) % MEM_ORDER;
	} while (i != max);
	int used = ps[front].jiffies - ps[tail].jiffies;
	float usage = (used * 1000000.0 / HZ) / dtwork;
	pusage->workingrate = 1.0 * dtwork / dt;
	pusage->pcpu = usage * pusage->workingrate;
	if (size == MEM_ORDER)
	    tail = (tail + 1) % MEM_ORDER;
	return 0;
    }
#undef MEM_ORDER
}

void print_caption()
{
    printf("\n%%CPU\twork quantum\tsleep quantum\tactive rate\n");
}

int cpulimit_entry(int verbose_setting, int lazy_setting, int perclimit,
		   int pid, char *exe, char *path)
{
    verbose = verbose_setting;
    lazy = lazy_setting;

    //used to determine what type of mode to use
    int pid_ok = 0;
    int process_ok = 0;

    if (pid > 0)
	pid_ok = 1;
    if (exe != NULL || path != NULL)
	process_ok = 1;

    if (!process_ok && !pid_ok) {
	daemon_log(LOG_ERR, "cpulimit: you must specify a target process.");
	return 1;
    }
    if ((exe != NULL && path != NULL)
	|| (pid_ok && (exe != NULL || path != NULL))) 
    {
	daemon_log(LOG_ERR, "cpulimit: you must specify exactly one target process.");
	return 1;
    }
    float limit = perclimit / 100.0;
    if (limit < 0 || limit > 1) {
	daemon_log(LOG_ERR, "cpulimit: limit must be in the range 0-100.");
	return 1;
    }
    //parameters are all ok!
    signal(SIGINT, quit);
    signal(SIGTERM, quit);

    //time quantum in microseconds. it's splitted in a working period and a sleeping one
    int period = 100000;
    struct timespec twork, tsleep;	//working and sleeping intervals
    memset(&twork, 0, sizeof(struct timespec));
    memset(&tsleep, 0, sizeof(struct timespec));

  wait_for_process:

    //look for the target process..or wait for it
    if (exe != NULL) {
	if ((pid = getpidof(exe)) == -1) {
	    return 1;
	}
    } else if (path != NULL) {
	if ((pid = getpidof(path)) == -1) {
	    return 1;
	}
    } else {
	if (waitforpid(pid) == -1) {
	    return 1;
	}
    }
    //process detected...let's play


    //init compute_cpu_usage internal stuff
    compute_cpu_usage(0, 0, NULL);
    //main loop counter
    int i = 0;

    struct timespec startwork, endwork;
    long workingtime = 0;	//last working time in microseconds

    if (verbose)
	print_caption();

    float pcpu_avg = 0;

    //here we should already have high priority, for time precision
    while (!sigint) {
	//estimate how much the controlled process is using the cpu in its working interval
	struct cpu_usage cu;
	if (compute_cpu_usage(pid, workingtime, &cu) == -1) {
	    daemon_log(LOG_ERR, "process %d dead!", pid);
	    if (lazy) return 1;
	    //wait until our process appears
	    goto wait_for_process;
	}
	//cpu actual usage of process (range 0-1)
	float pcpu = cu.pcpu;
	//rate at which we are keeping active the process (range 0-1)
	float workingrate = cu.workingrate;

	//adjust work and sleep time slices
	if (pcpu > 0) {
	    twork.tv_nsec =
		min(period * limit * 1000 / pcpu * workingrate,
		    period * 1000);
	} else if (pcpu == 0) {
	    twork.tv_nsec = period * 1000;
	} else if (pcpu == -1) {
	    //not yet a valid idea of cpu usage
	    pcpu = limit;
	    workingrate = limit;
	    twork.tv_nsec = min(period * limit * 1000, period * 1000);
	}
	tsleep.tv_nsec = period * 1000 - twork.tv_nsec;

	//update average usage
	pcpu_avg = (pcpu_avg * i + pcpu) / (i + 1);

	if (verbose && i % 10 == 0 && i > 0) {
	    printf("%0.2f%%\t%6ld us\t%6ld us\t%0.2f%%\n", pcpu * 100,
		   twork.tv_nsec / 1000, tsleep.tv_nsec / 1000,
		   workingrate * 100);
	}

	if (limit < 1 && limit > 0) {
	    //resume process
	    if (kill(pid, SIGCONT) != 0) {
	    	daemon_log(LOG_ERR, "process %d dead!", pid);
		if (lazy) return 1;
		//wait until our process appears
		goto wait_for_process;
	    }
	}

	clock_gettime(CLOCK_REALTIME, &startwork);
	nanosleep(&twork, NULL);	//now process is working        
	clock_gettime(CLOCK_REALTIME, &endwork);
	workingtime = timediff(&endwork, &startwork);

	if (limit < 1) {
	    //stop process, it has worked enough
	    if (kill(pid, SIGSTOP) != 0) {
	    	daemon_log(LOG_ERR, "process %d dead!", pid);
		if (lazy) return 1;
		//wait until our process appears
		goto wait_for_process;
	    }
	    nanosleep(&tsleep, NULL);	//now process is sleeping
	}
	i++;
    }

    // Start the process, if it is stopped
    kill(pid, SIGCONT);

    return 0;
}

int cpulimit_bypid(int verbose_setting, int lazy_setting, int perclimit,
		   int pid)
{
    return cpulimit_entry(verbose_setting, lazy_setting, perclimit, pid,
			  NULL, NULL);
}

int cpulimit_byexe(int verbose_setting, int lazy_setting, int perclimit,
		   char *exe)
{
    return cpulimit_entry(verbose_setting, lazy_setting, perclimit, -1,
			  exe, NULL);
}

int cpulimit_bypath(int verbose_setting, int lazy_setting, int perclimit,
		    char *path)
{
    return cpulimit_entry(verbose_setting, lazy_setting, perclimit, -1,
			  NULL, path);
}
