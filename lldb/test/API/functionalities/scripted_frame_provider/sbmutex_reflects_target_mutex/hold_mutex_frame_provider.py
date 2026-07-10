"""
Frame provider whose get_frame_at_index locks the target's real API
mutex via SBMutex and holds it briefly, from inside the bypassed
scripted-extension callback.

This is the exact pattern that used to deadlock: LLDB's private state
thread can reach this callback without already holding the real API
mutex, so a blocking lock() here is a genuinely new acquisition
attempt, not a safe same-thread recursive re-lock. Before
Target::APIMutexHandle re-resolved Target::GetAPIMutex() on every call
(rather than aliasing a fixed mutex captured at construction time), this
held mutex.lock() call would block waiting for another thread (e.g. the
command thread) that was itself waiting on this thread to finish
processing the stop -- an AB-BA deadlock. Now, because this thread is
running inside a scripted-extension callback, GetAPIMutex() hands it
the thread-local bypass mutex instead, so lock() never contends with
anyone.
"""

import time

from lldb.plugins.scripted_frame_provider import ScriptedFrameProvider

# How long to hold the real API mutex on each get_frame_at_index(0) call.
HOLD_DURATION_SECONDS = 0.2


class HoldMutexFrameProvider(ScriptedFrameProvider):
    @staticmethod
    def get_description():
        return "Provider that holds the real API mutex via SBMutex from get_frame_at_index"

    def get_frame_at_index(self, index):
        if index >= len(self.input_frames):
            return None

        if index == 0:
            mutex = self.target.GetAPIMutex()
            mutex.lock()
            time.sleep(HOLD_DURATION_SECONDS)
            mutex.unlock()

        frame = self.input_frames[index]
        if frame is None:
            return None
        return {"idx": index, "pc": frame.GetPC()}
