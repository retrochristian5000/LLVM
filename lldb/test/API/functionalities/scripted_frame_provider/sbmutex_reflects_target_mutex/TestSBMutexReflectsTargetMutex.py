"""
Test that a scripted frame provider calling SBTarget.GetAPIMutex() from
get_frame_at_index gets a handle that reflects the state of the target's
real, shared API mutex, even though the callback's own thread is exempt
from having to serialize on it.

SBMutex is meant to be usable across threads and outlive the call that
created it (e.g. lldb-dap hands it to background workers), so it must
always alias the genuine target mutex rather than the thread-local mutex
the bypass hands out for internal callers. This test drives the same
kind of command-thread / internal-thread race as
TestFrameProviderRegisterCommandAPIMutexDeadlock, interleaving `bt` with
`continue` (hitting the same breakpoint again each time, via a loop in
main.c) so get_frame_at_index runs many times instead of once, and has
the provider call target.GetAPIMutex().try_lock() from inside the
callback: if some other thread happens to hold the real mutex at that
moment, this should observe it as contended.

Only try_lock() is used, which never blocks, so this cannot deadlock
regardless of the outcome. An earlier version of this test tried to
widen the race window by having the callback actually lock() and hold
the mutex for a short duration, on the assumption that whichever thread
reaches this callback already holds the real mutex first. That
assumption is wrong -- LLDB's private state thread can reach this
callback without already holding it -- so that held mutex.lock() call
could genuinely block, and reproducibly deadlocked in practice. Do not
reintroduce a blocking acquisition here.

Observing contention is a genuine cross-thread race, so -- like the
sibling runlock_reentrant_deadlock/was_hit_deadlock/
register_command_api_mutex_deadlock tests -- this is best-effort: it
raises the odds of witnessing it within a single invocation but cannot
guarantee it, and the test does not require it to pass.
"""

import os
import lldb
import lldbsuite.test.lldbutil as lldbutil
from lldbsuite.test.decorators import *
from lldbsuite.test.lldbtest import *


class TestSBMutexReflectsTargetMutex(TestBase):
    NO_DEBUG_INFO_TESTCASE = True

    def test_sbmutex_reflects_target_mutex(self):
        """
        Register a scripted frame provider that checks
        target.GetAPIMutex().try_lock() from get_frame_at_index, then
        repeatedly run `bt` and `continue` through RunCommandInterpreter.
        Should complete without deadlocking, regardless of whether
        contention is observed.
        """
        self.build()

        lldbutil.run_to_name_breakpoint(self, "frame3")

        provider_path = os.path.join(self.getSourceDir(), "sbmutex_frame_provider.py")
        artifact_path = self.getBuildArtifact("contention.txt")
        if os.path.exists(artifact_path):
            os.remove(artifact_path)

        commands = ["command script import " + provider_path]
        commands.append(
            "target frame-provider register "
            "-C sbmutex_frame_provider.ContentionCheckFrameProvider "
            "-k artifact_path -v " + artifact_path
        )
        # `bt` only re-invokes get_frame_at_index when the thread's stack
        # frame list was invalidated by a new stop, so interleave `bt` with
        # `continue` (hitting the same breakpoint again, in a loop in
        # main.c) to get repeated fresh invocations, raising the odds of
        # hitting the race within a single test invocation.
        commands.extend(["bt", "continue"] * 20)
        commands.append("quit")

        stdin_path = self.getBuildArtifact("stdin.txt")
        stdout_path = self.getBuildArtifact("stdout.txt")
        with open(stdin_path, "w") as f:
            f.write("\n".join(commands) + "\n")

        with open(stdin_path, "r") as in_fileH, open(stdout_path, "w") as out_fileH:
            in_sbf = lldb.SBFile(in_fileH.fileno(), "r", False)
            out_sbf = lldb.SBFile(out_fileH.fileno(), "w", False)
            self.assertSuccess(self.dbg.SetInputFile(in_sbf))
            self.assertSuccess(self.dbg.SetOutputFile(out_sbf))
            self.assertSuccess(self.dbg.SetErrorFile(out_sbf))

            options = lldb.SBCommandInterpreterRunOptions()
            options.SetEchoCommands(False)
            options.SetPrintResults(True)
            options.SetStopOnError(False)
            options.SetStopOnCrash(False)

            # try_lock() never blocks, so this can only hang if something
            # else regresses (e.g. a leaked recursive lock count).
            n_errors, quit_requested, has_crashed = self.dbg.RunCommandInterpreter(
                True, False, options, 0, False, False
            )

        with open(stdout_path, "r") as out_fileH:
            output = out_fileH.read()

        self.assertFalse(has_crashed, "lldb should not have crashed")
        self.assertTrue(quit_requested, "quit command should have been processed")
        self.assertEqual(n_errors, 0, f"unexpected errors in output:\n{output}")
        self.assertIn("successfully registered scripted frame provider", output)

        self.assertTrue(
            os.path.exists(artifact_path),
            "get_frame_at_index should have run and recorded at least one outcome",
        )
        with open(artifact_path, "r") as f:
            outcomes = [line.strip() for line in f if line.strip()]

        self.assertTrue(outcomes, "expected at least one recorded outcome")
        self.assertTrue(
            all(o in ("CONTENDED", "UNCONTENDED") for o in outcomes),
            f"unexpected outcome values: {outcomes}",
        )
        # A "CONTENDED" outcome means some other thread held the real
        # mutex at that moment, which proves SBMutex aliases the genuine,
        # shared target mutex rather than the bypass mutex. Whether that
        # race is hit is not guaranteed within a single invocation, so it
        # isn't asserted on here.
