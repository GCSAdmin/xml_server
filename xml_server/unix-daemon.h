/* $%BEGINLICENSE%$
 Copyright (c) 2010, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License as
 published by the Free Software Foundation; version 2 of the
 License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 02110-1301  USA

 $%ENDLICENSE%$ */

#ifndef __UNIX_DAEMON_H__
#define __UNIX_DAEMON_H__

#ifndef _WIN32
#include <sys/wait.h> /* wait4 */
#include <sys/resource.h> /* getrusage */
#include <unistd.h>
#endif // _WIN32

#include <sys/stat.h>


#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <signal.h>

#include <glib.h>

int unix_proc_keepalive(int *child_exit_status);
void unix_daemonize(void);

#endif
