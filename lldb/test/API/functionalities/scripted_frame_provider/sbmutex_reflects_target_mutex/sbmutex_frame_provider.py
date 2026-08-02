"""
Frame provider whose get_frame_at_index checks, from inside the bypassed
scripted-extension callback, whether the target's real API mutex is
currently held by a different thread.

Used by TestSBMutexReflectsTargetMutex.py to confirm that SBMutex
(SBTarget::GetAPIMutex()) aliases the real, shared target mutex rather
than the thread-local mutex the bypass hands out for internal callers.

Only try_lock() is used, and it is never held beyond the immediate
check: an earlier version of this provider held the mutex for a short
duration to try to widen the race window, but that meant a genuinely
blocking acquisition from whichever thread invoked this callback -- not
every internal caller (e.g. the private state thread) already holds the
real mutex by the time it gets here, so that held the mutex, which
caused a real deadlock in practice. try_lock() never blocks, so this
cannot deadlock regardless of the outcome.
"""

from lldb.plugins.scripted_frame_provider import ScriptedFrameProvider


class ContentionCheckFrameProvider(ScriptedFrameProvider):
    @staticmethod
    def get_description():
        return "Provider that checks SBMutex contention from get_frame_at_index"

    def __init__(self, input_frames, args):
        super().__init__(input_frames, args)
        self.artifact_path = None
        if self.args is not None:
            value = self.args.GetValueForKey("artifact_path")
            if value.IsValid():
                self.artifact_path = value.GetStringValue(4096)

    def get_frame_at_index(self, index):
        if index >= len(self.input_frames):
            return None

        if index == 0 and self.artifact_path:
            mutex = self.target.GetAPIMutex()
            if mutex.try_lock():
                # Uncontended: this thread already owned the real mutex
                # (recursion) or nobody else holds it right now. Undo the
                # extra recursive lock we just took.
                mutex.unlock()
                outcome = "UNCONTENDED"
            else:
                outcome = "CONTENDED"
            with open(self.artifact_path, "a") as f:
                f.write(outcome + "\n")

        frame = self.input_frames[index]
        if frame is None:
            return None
        return {"idx": index, "pc": frame.GetPC()}
