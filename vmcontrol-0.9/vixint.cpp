/* 
 * Copyright 2009 Dan McNulty, Tom Baars
 *
 * This file is part of Teratorn.
 *
 * Teratorn is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * Provides an interface to the VIX API
 */

#include <iostream>
#include <cstdlib>
#include <cerrno>
using std::cout;
using std::cerr;
using std::endl;
using std::string;

// C headers
extern "C" {
#include <libdaemon/dlog.h>

// VIX API header
#include "vix.h"
}

// Project includes
#include "vixint.h"
#include "util.h"

static int MIN_5 = 60*5;
static int VMCONNECT_FUDGE = 2; // time to wait before retrying a connection
static int VMCONNECT_RETRY = 3; // number of times to retry

static VixHandle getVixHandle(char *path, char *user, char *password, 
	char *hostname) 
{
    VixHandle hostHandle = VIX_INVALID_HANDLE;
    VixHandle jobHandle = VIX_INVALID_HANDLE;
    VixHandle vmHandle = VIX_INVALID_HANDLE;
    VixError err;
    string vmxFilePath = string("[standard] ") + string(path);

    string startErrMsg;
    bool started = false;
    if( !startServices(startErrMsg, started) ) {
	startErrMsg = string("Unabled to start VMWare services: ") +
	    startErrMsg;
	daemon_log(LOG_ERR, "%s", startErrMsg.c_str());
	return VIX_INVALID_HANDLE;
    }

    // Connect to local host
    for(int i = 0; i < VMCONNECT_RETRY; i++) {
	jobHandle = VixHost_Connect(VIX_API_VERSION,
				   VIX_SERVICEPROVIDER_VMWARE_VI_SERVER,
				   hostname, 0, user, password, 0, 
				   VIX_INVALID_HANDLE, NULL, NULL); 

	err = VixJob_Wait(jobHandle, VIX_PROPERTY_JOB_RESULT_HANDLE,
			  &hostHandle, VIX_PROPERTY_NONE);

	if (err != VIX_OK) {
	    string tmpErr = string("Error connecting to host: ") +
		string(Vix_GetErrorText(err, NULL));
	    daemon_log(LOG_ERR, "%s", tmpErr.c_str());
	    Vix_ReleaseHandle(jobHandle);

	    // Retry connection?
	    if( started && i < (VMCONNECT_RETRY-1) ) {
		sleep(VMCONNECT_FUDGE);
		continue;
	    }

	    return VIX_INVALID_HANDLE;
	}

	Vix_ReleaseHandle(jobHandle);
	break;
    }

    // Open virtual machine and get a handle.
    jobHandle = VixVM_Open(hostHandle, vmxFilePath.c_str(), NULL, NULL); 

    err = VixJob_Wait(jobHandle, VIX_PROPERTY_JOB_RESULT_HANDLE,
		      &vmHandle, VIX_PROPERTY_NONE);

    if (err != VIX_OK) {
	string tmpErr = string("Error getting vm handle: ") +
	    Vix_GetErrorText(err, NULL);
	daemon_log(LOG_ERR, "%s", tmpErr.c_str());
	Vix_ReleaseHandle(jobHandle);
	return VIX_INVALID_HANDLE;
    }
     
    Vix_ReleaseHandle(jobHandle);

    return vmHandle;
}

static bool isVMRunning(VixHandle vmHandle) {
    Bool isRunning;
    VixError err = Vix_GetProperties(vmHandle, VIX_PROPERTY_VM_IS_RUNNING,
	    &isRunning, VIX_PROPERTY_NONE);

    if( err != VIX_OK ) {
	string tmpErr = string("Error checking the running state of the VM: ")
	    + Vix_GetErrorText(err, NULL);
	daemon_log(LOG_ERR, "%s", tmpErr.c_str());
	return false;
    }

    return isRunning;
}

bool isVMRunning(char *path, char *user, char *password, char *hostname) {
    VixHandle vmHandle = getVixHandle(path, user, password, hostname);
    if( vmHandle == VIX_INVALID_HANDLE ) {
	return false;
    }

    return isVMRunning(vmHandle);
}

/* Currently unused
static bool isVMToolsInstalled(VixHandle vmHandle) {
    VixToolsState toolsState;
    VixError err = Vix_GetProperties(vmHandle, VIX_PROPERTY_VM_TOOLS_STATE,
	    &toolsState);

    if( err != VIX_OK ) {
	cerr << "Error checking the tools state of the VM: "
	     << Vix_GetErrorText(err, NULL) << endl;
	return false;
    }

    if( toolsState != VIX_TOOLSSTATE_RUNNING )
	return false;

    return true;
}
*/

bool powerOnVM(char *path, char *user, char *password, char *hostname) {
    VixHandle jobHandle = VIX_INVALID_HANDLE;
    VixError err;

    VixHandle vmHandle = getVixHandle(path, user, password, hostname);
    if( vmHandle == VIX_INVALID_HANDLE ) {
	return false;
    }

    if( !isVMRunning(vmHandle) ) {
	// Power on the virtual machine.
	jobHandle = VixVM_PowerOn(vmHandle, 0, VIX_INVALID_HANDLE, NULL, NULL);

	err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
	if (VIX_OK != err && err != VIX_E_VM_IS_RUNNING) {
	    string tmpErr = string("Error powering on the VM: ") +
		Vix_GetErrorText(err, NULL);
	    daemon_log(LOG_ERR, "%s", tmpErr.c_str());
	    Vix_ReleaseHandle(jobHandle);
	    Vix_ReleaseHandle(vmHandle);
	    return false;
	}

	Vix_ReleaseHandle(jobHandle);

	daemon_log(LOG_INFO, "Powered on the VM. Waiting for it to boot.");
    }else{
	daemon_log(LOG_INFO, "VM already on, checking for VMWare tools.");
    }

    // Indicates system is up
    jobHandle = VixVM_WaitForToolsInGuest(vmHandle, MIN_5, NULL, NULL);

    err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
    if( err != VIX_OK ) {
	if( err == VIX_E_TOOLS_NOT_RUNNING ||
	    err == VIX_E_TIMEOUT_WAITING_FOR_TOOLS )
	{
	    daemon_log(LOG_WARNING, 
		    "WARNING: VMWare Tools not available in VM.");
	}else{
	    string tmpErr = string("Error waiting for VM to boot: ") +
		Vix_GetErrorText(err, NULL);
	    daemon_log(LOG_ERR, "%s", tmpErr.c_str());
	    Vix_ReleaseHandle(jobHandle);
	    Vix_ReleaseHandle(vmHandle);
	    return false;
	}
    }

    Vix_ReleaseHandle(vmHandle);
    Vix_ReleaseHandle(jobHandle);

    daemon_log(LOG_ERR, "VM is now ready for experiments.");

    return true;
}

bool powerOffVM(char *path, char *user, char *password, char *hostname) {
    VixError err = VIX_OK;
    VixHandle jobHandle = VIX_INVALID_HANDLE;

    VixHandle vmHandle = getVixHandle(path, user, password, hostname);
    if( vmHandle == VIX_INVALID_HANDLE ) {
	return false;
    }

    // Power off the virtual machine
    if( isVMRunning(vmHandle) ) {
	jobHandle = VixVM_PowerOff(vmHandle,
				   VIX_VMPOWEROP_NORMAL,
				   NULL,
				   NULL);
	err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
	if (err != VIX_OK) {
	    string tmpErr = string("Error powering off the VM: ")
		+ Vix_GetErrorText(err, NULL);
	    daemon_log(LOG_ERR, "%s", tmpErr.c_str());
	    Vix_ReleaseHandle(jobHandle);
	    return false;
	}

	Vix_ReleaseHandle(jobHandle);
	jobHandle = VIX_INVALID_HANDLE;

	daemon_log(LOG_INFO, "Powered off the VM.");
    }else{
	daemon_log(LOG_INFO, "VM is not currently running.");
    }

    return true;
}
