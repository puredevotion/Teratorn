#ifndef __CPULIMIT_H__
#define __CPULIMIT_H__

enum cpulimit_cmd {
    CPULIMIT_STOPPED,
    CPULIMIT_START,
    CPULIMIT_END
};

int cpulimit_bypid(int verbose_setting, int lazy_setting, 
	int perclimit, int pid);
int cpulimit_byexe(int verbose_setting, int lazy_setting, 
	int perclimit, char *exe);
int cpulimit_bypath(int verbose_setting, int lazy_setting, 
	int perclimit, char *path);
#endif
