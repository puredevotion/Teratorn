#include <iostream>
using std::cout;
using std::cerr;
using std::endl;

using std::string;

#include <getopt.h>

#include "vix.h"

static const char *USAGE = " [-U] user [-P] password [-u] [-d] <vmpath>";
static const char *OPT_STR = "udU:P:";
static const char *HOSTNAME = "http://127.0.0.1:8222/sdk";

bool powerVMOn(VixHandle);
bool powerVMOff(VixHandle);
VixHandle getVixHandle(char *, char *, char *);

int main(int argc, char *argv[]) {
    bool powerOn = false;
    bool powerOff = false;
    char *user = NULL;
    char *password = NULL;

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
	    case 'P':
		password = optarg;
		break;
	    default:
		cerr << "Usage: " << argv[0] << USAGE << endl;
		return 1;
	}
    }

    if( (!powerOn && !powerOff) || !user || !password) {
	cerr << "Usage: " << argv[0] << USAGE << endl;
	return 1;
    }

    if( powerOn && powerOff ) {
	cerr << "Ambiguous option: attempting to power on" << endl;
	powerOff = false;
    }

    // First, get a handle to the VM
    VixHandle vmHandle = VIX_INVALID_HANDLE;
    if( (vmHandle = getVixHandle(argv[optind], user, password)) == VIX_INVALID_HANDLE ) {
	return 1;
    }

    if( powerOn ) {
	if( powerVMOn(vmHandle) ) return 0;
	else return 1;
    }

    if( powerOff ) {
	if( powerVMOff(vmHandle) ) return 0;
	else return 1;
    }

    return 0;
}

VixHandle getVixHandle(char *path, char *user, char *password) {
    VixHandle hostHandle = VIX_INVALID_HANDLE;
    VixHandle jobHandle = VIX_INVALID_HANDLE;
    VixHandle vmHandle = VIX_INVALID_HANDLE;
    VixError err;
    string vmxFilePath = string("[standard] ") + string(path);

    // Connect to local host
    jobHandle = VixHost_Connect(VIX_API_VERSION,
			       VIX_SERVICEPROVIDER_VMWARE_VI_SERVER,
			       HOSTNAME, // hostName
			       0, // hostPort
			       user, // userName
			       password, // password,
			       0, // options
			       VIX_INVALID_HANDLE, // propertyListHandle
			       NULL, // callbackProc
			       NULL); // clientData

    err = VixJob_Wait(jobHandle,
		     VIX_PROPERTY_JOB_RESULT_HANDLE,
		     &hostHandle,
		     VIX_PROPERTY_NONE);

    if (VIX_OK != err) {
	cerr << "Error connecting to localhost: " << Vix_GetErrorText(err, NULL) << endl;
	return VIX_INVALID_HANDLE;
    }

    Vix_ReleaseHandle(jobHandle);
    jobHandle = VIX_INVALID_HANDLE;

    cout << "Connected to host" << endl;

    // Open virtual machine and get a handle.
    jobHandle = VixVM_Open(hostHandle,
			   vmxFilePath.c_str(),
			   NULL, // callbackProc
			   NULL); // clientData

    err = VixJob_Wait(jobHandle,
		      VIX_PROPERTY_JOB_RESULT_HANDLE,
		      &vmHandle,
		      VIX_PROPERTY_NONE);

    if (VIX_OK != err) {
	cerr << "Error getting vm handle: " 
	     << Vix_GetErrorText(err, NULL) << endl;
	return VIX_INVALID_HANDLE;
    }
     
    Vix_ReleaseHandle(jobHandle);
    jobHandle = VIX_INVALID_HANDLE;

    cout << "Obtained VM handle" << endl;

    return vmHandle;
}

bool powerVMOn(VixHandle vmHandle) {
    VixHandle jobHandle = VIX_INVALID_HANDLE;
    VixError err;

    // Power on the virtual machine.
    jobHandle = VixVM_PowerOn(vmHandle,
			      0, // powerOnOptions,
			      VIX_INVALID_HANDLE, // propertyListHandle,
			      NULL, // callbackProc,
			      NULL); // clientData
     
    err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
    if (VIX_OK != err) {
	cerr << "Error powering on the VM: " 
	     << Vix_GetErrorText(err, NULL) << endl;
	return false;
    }

    Vix_ReleaseHandle(jobHandle);
    jobHandle = VIX_INVALID_HANDLE;

    cout << "Powered on the VM" << endl;

    return true;
}

bool powerVMOff(VixHandle vmHandle) {
    VixError err = VIX_OK;
    VixHandle jobHandle = VIX_INVALID_HANDLE;
     
    // Power off the virtual machine.
    jobHandle = VixVM_PowerOff(vmHandle,
			       VIX_VMPOWEROP_NORMAL, // powerOffOptions
			       NULL, // callbackProc
			       NULL); // clientData
    err = VixJob_Wait(jobHandle, VIX_PROPERTY_NONE);
    if (VIX_OK != err) {
	cerr << "Error powering off the VM: " 
	     << Vix_GetErrorText(err, NULL) << endl;
    }

    Vix_ReleaseHandle(jobHandle);
    jobHandle = VIX_INVALID_HANDLE;

    return false;
}
