/*-*- Mode: C; c-basic-offset: 8; indent-tabs-mode: nil -*-*/

/***
  This file is part of systemd.

  Copyright 2014 Lennart Poettering
  Copyright 2012 Michael Olbrich

  systemd is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published by
  the Free Software Foundation; either version 2.1 of the License, or
  (at your option) any later version.

  systemd is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with systemd; If not, see <http://www.gnu.org/licenses/>.
***/

#include <sys/reboot.h>
#include <linux/reboot.h>
#include <sys/syscall.h>

#include "bus-util.h"
#include "bus-error.h"
#include "special.h"
#include "failure-action.h"

int failure_action(
                Manager *m,
                FailureAction action,
                const char *reboot_arg) {

        int r;

        assert(m);
        assert(action >= 0);
        assert(action < _FAILURE_ACTION_MAX);

        switch (action) {

        case FAILURE_ACTION_NONE:
                break;

        case FAILURE_ACTION_REBOOT: {
                _cleanup_bus_error_free_ sd_bus_error error = SD_BUS_ERROR_NULL;

                log_warning("Rebooting as result of failure.");

                update_reboot_param_file(reboot_arg);
                r = manager_add_job_by_name(m, JOB_START, SPECIAL_REBOOT_TARGET, JOB_REPLACE, true, &error, NULL);
                if (r < 0)
                        log_error("Failed to reboot: %s.", bus_error_message(&error, r));

                break;
        }

        case FAILURE_ACTION_REBOOT_FORCE:
                log_warning("Forcibly rebooting as result of failure.");
                update_reboot_param_file(reboot_arg);
                m->exit_code = MANAGER_REBOOT;
                break;

        case FAILURE_ACTION_REBOOT_IMMEDIATE:
                log_warning("Rebooting immediately as result of failure.");

                sync();

                if (reboot_arg) {
                        log_info("Rebooting with argument '%s'.", reboot_arg);
                        syscall(SYS_reboot, LINUX_REBOOT_MAGIC1, LINUX_REBOOT_MAGIC2, LINUX_REBOOT_CMD_RESTART2, reboot_arg);
                }

                log_info("Rebooting.");
                reboot(RB_AUTOBOOT);
                break;

        default:
                assert_not_reached("Unknown failure action");
        }

        return -ECANCELED;
}

static const char* const failure_action_table[_FAILURE_ACTION_MAX] = {
        [FAILURE_ACTION_NONE] = "none",
        [FAILURE_ACTION_REBOOT] = "reboot",
        [FAILURE_ACTION_REBOOT_FORCE] = "reboot-force",
        [FAILURE_ACTION_REBOOT_IMMEDIATE] = "reboot-immediate"
};
DEFINE_STRING_TABLE_LOOKUP(failure_action, FailureAction);