"""
Test that creating a target through a symlinked (or otherwise aliased)
executable does not create a duplicate module at launch.

When a target is created via a path that is not the file's canonical path (a
symlink, or on Windows a subst/mapped drive), the running process reports its
*real* image path at launch. Module matching needs to compare the canonicalized
paths.
"""

import os
import lldb
from lldbsuite.test.decorators import *
from lldbsuite.test.lldbtest import *
from lldbsuite.test import lldbutil


class TestBreakpointViaSymlink(TestBase):
    NO_DEBUG_INFO_TESTCASE = True

    @skipIfRemote
    def test_breakpoint_resolves_once_via_symlink(self):
        self.build()
        real_exe = self.getBuildArtifact("a.out")

        # A symlink whose real path differs from the path used to create the
        # target.
        link_exe = self.getBuildArtifact("a.out.symlink")
        try:
            if os.path.lexists(link_exe):
                os.remove(link_exe)
            os.symlink(real_exe, link_exe)
        except (OSError, NotImplementedError) as e:
            # Windows needs Developer Mode / SeCreateSymbolicLinkPrivilege.
            self.skipTest("could not create a symlink: %s" % e)
        self.addTearDownHook(lambda: os.remove(link_exe))
        self.assertNotEqual(os.path.realpath(link_exe), link_exe)

        target = self.dbg.CreateTarget(link_exe)
        self.assertTrue(target, VALID_TARGET)

        bkpt = target.BreakpointCreateBySourceRegex(
            "break here", lldb.SBFileSpec("main.c")
        )
        # Resolves against the preloaded module: exactly one location.
        self.assertEqual(
            bkpt.GetNumLocations(), 1, "one breakpoint location before launch"
        )

        process = target.LaunchSimple(None, None, self.get_process_working_directory())
        self.assertState(process.GetState(), lldb.eStateStopped)

        # After launch the running image may report its real path. With module
        # matching canonicalizing paths, the loaded module is the one lldb already
        # preloaded, so the breakpoint keeps exactly one resolved location.
        self.assertEqual(bkpt.GetNumLocations(), 1)
        self.assertEqual(bkpt.GetNumResolvedLocations(), 1)

        # The executable appears as a single module (no duplicate). The module
        # keeps the path it was created with (the symlink), so compare by real
        # path.
        exe_real = os.path.realpath(real_exe)
        matches = [
            m
            for m in target.module_iter()
            if os.path.realpath(m.GetFileSpec().fullpath) == exe_real
        ]
        self.assertEqual(
            len(matches), 1, "exactly one executable module, got %d" % len(matches)
        )
