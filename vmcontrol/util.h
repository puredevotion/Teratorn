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

#ifndef __UTIL_H__
#define __UTIL_H__

bool pgrep(std::string&);
int systemExec(std::string&);
bool startServices(std::string &, bool &);
pid_t getVMPid(char *);
char *get_password();
int write_all(int, void *, int);
int read_all(int, void *, int);
void termChild(pid_t, char *);

#endif
