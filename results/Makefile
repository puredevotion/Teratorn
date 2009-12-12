
default:
	cd 8k/vm_workload && $(MAKE)
	cd 8k/vm_noworkload && $(MAKE)
	cd 8k/normal_host && $(MAKE)
	cd 250k/vm_workload && $(MAKE)
	cd 250k/normal_host && $(MAKE)

clean:
	cd 8k/vm_workload && $(MAKE) clean
	cd 8k/vm_noworkload && $(MAKE) clean
	cd 8k/normal_host && $(MAKE) clean
	cd 250k/vm_workload && $(MAKE) clean
	cd 250k/normal_host && $(MAKE) clean

