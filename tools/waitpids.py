"""Block until the named processes exit, on pidfs handles rather than pid numbers.

The checks in this project run for minutes - `check_machines.py` and
`check_solutions.py` for several - and something has to wait for them without
polling. The obvious `until pgrep -f check_machines.py; do sleep 20; done` is
wrong in a way that looks like success: the watcher's own command line contains
the pattern, so `pgrep` matches *itself* and the loop either exits at once or
never fires. Four watches in one session expired having never seen the run they
were watching, and each looked exactly like a quiet wait. A bare pid number has
the smaller version of the same problem - it can be reused between the look and
the wait.

A pidfd is a handle to **one** process and nothing else. `poll()` on it reports
POLLIN exactly when that process exits, the handle cannot come to mean another
process, and it works for a process this one did not fork (Linux 5.3 and
later). There is no interval to choose and nothing to match.

Start a run as `( cmd > out 2>&1 & echo $! > out.pid )` and wait for it with
`waitpids.py @out.pid`. Several pids may be given, and one that has already
exited is not an error - there is nothing left to wait for.

This file is the port's own tooling; it is not a transcription.
"""

# SPDX-License-Identifier: GPL-2.0-only

import os
import select
import sys


def wait(pids):
    """Return once every one of `pids` has exited."""
    poller = select.poll()
    open_fds = 0

    for pid in pids:
        try:
            fd = os.pidfd_open(pid)
        except (ProcessLookupError, PermissionError, OSError):
            continue          # already gone, or not ours: nothing to wait for
        poller.register(fd, select.POLLIN)
        open_fds += 1

    while open_fds:
        for fd, _ in poller.poll():
            poller.unregister(fd)
            os.close(fd)
            open_fds -= 1


def main(argv):
    pids = []
    for arg in argv:
        if arg.startswith("@"):
            with open(arg[1:]) as f:
                pids += [int(w) for w in f.read().split()]
        else:
            pids.append(int(arg))

    if not pids:
        sys.exit("usage: waitpids.py <pid> [<pid> ...] | @<file of pids>")

    wait(pids)


if __name__ == "__main__":
    main(sys.argv[1:])
