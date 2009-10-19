/*
 * Provides an interface to the VIX API
 *
 * This file is part of Teratorn.
 */

#include <iostream>
#include <cstdlib>
#include <cerrno>
using std::cout;
using std::cerr;
using std::endl;
using std::string;

// VIX API header
#include "vix.h"

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
	cerr << "Unable to start VMWare services: " << startErrMsg << endl;
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
	    cerr << "Error connecting to host: " 
		 << Vix_GetErrorText(err, NULL) << endl;
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

    cout << "Connected to host" << endl;

    // Open virtual machine and get a handle.
    jobHandle = VixVM_Open(hostHandle, vmxFilePath.c_str(), NULL, NULL); 

    err = VixJob_Wait(jobHandle, VIX_PROPERTY_JOB_RESULT_HANDLE,
		      &vmHandle, VIX_PROPERTY_NONE);

    if (err != VIX_OK) {
	cerr << "Error getting vm handle: " 
	     << Vix_GetErrorText(err, NULL) << endl;
	Vix_ReleaseHandle(jobHandle);
	return VIX_INVALID_HANDLE;
    }
     
    Vix_ReleaseHandle(jobHandle);

    cout << "Obtained VM handle" << endl;

    return vmHandle;
}

static bool isVMRunning(VixHandle vmHandle) {
    Bool isRunning;
    VixError err = Vix_GetProperties(vmHandle, VIX_PROPERTY_VM_IS_RUNNING,
	    &isRunning, VIX_PROPERTY_NONE);

    if( err != VIX_OK ) {
	cerr << "Error checking the running state of the VM: "
	     << Vix_GetErrorText(err, NULL) << endl;
	return false;
    }

    if( isRunning ) {
	cout << "VM is running" << endl;
	return true; 
    }

    cout << "VM is not running" << endl;
    return false;
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
	    cerr << "Error powering on the VM: "
		 << Vix_GetErrorText(err, NULL) << endl;
	    Vix_ReleaseHandle(jobHandle);
	    Vix_ReleaseHandle(vmHandle);
	    return false;
	}

	Vix_ReleaseHandle(jobHandle);

	cout << "Powered on the VM. Waiting for it to boot." << endl;
    }

    // Indicates system is up
    jobHandle = VixVM_WaitForToolsInGuest(vmHandle, MIN_5, NULL, NULL);

    err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
    if( err != VIX_OK ) {
	if( err == VIX_E_TOOLS_NOT_RUNNING ||
	    err == VIX_E_TIMEOUT_WAITING_FOR_TOOLS )
	{
	    cerr << "WARNING: VMWare Tools not available in VM" << endl;
	}else{
	    cerr << "Error waiting for VM to boot: "
		 << Vix_GetErrorText(err, NULL) << endl;
	    Vix_ReleaseHandle(jobHandle);
	    Vix_ReleaseHandle(vmHandle);
	    return false;
	}
    }

    Vix_ReleaseHandle(vmHandle);
    Vix_ReleaseHandle(jobHandle);

    cout << "VM is now ready for experiments." << endl;

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
	    cerr << "Error powering off the VM: "
		 << Vix_GetErrorText(err, NULL) << endl;
	    Vix_ReleaseHandle(jobHandle);
	    return false;
	}

	Vix_ReleaseHandle(jobHandle);
	jobHandle = VIX_INVALID_HANDLE;

	cout << "Powered off the VM" << endl;
    }else{
	cerr << "VM is not currently running" << endl;
    }

    return true;
}
