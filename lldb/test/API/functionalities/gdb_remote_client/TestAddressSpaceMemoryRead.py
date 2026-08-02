import lldb
from lldbsuite.test.lldbtest import *
from lldbsuite.test.decorators import *
from lldbsuite.test.gdbclientutils import *
from lldbsuite.test.lldbgdbclient import GDBRemoteTestBase


class TestAddressSpaceMemoryRead(GDBRemoteTestBase):
    """
    End-to-end test that the same numeric address read from two different
    address spaces returns different bytes. The server advertises
    "address-spaces+" in qSupported, reports the spaces via "jAddressSpacesInfo",
    and reads memory with an optional ";address_space:<id>;" suffix on the
    standard memory packet.
    """

    def test(self):
        address_spaces_json = (
            '[{"name":"global","value":1,"is_thread_specific":false},'
            '{"name":"local","value":2,"is_thread_specific":true}]'
        )

        class MyResponder(MockGDBServerResponder):
            def qSupported(self, client_supported):
                return "PacketSize=3fff;QStartNoAckMode+;address-spaces+"

            def qHostInfo(self):
                return "ptrsize:8;endian:little;"

            def _bytes_for_space(self, space):
                if space == 1:
                    return "aabbccdd"
                if space == 2:
                    return "11223344"
                return "E01"

            def _respond_impl(self, packet):
                # The base dispatcher can't parse the ";address_space:<id>;"
                # suffix, so handle suffixed reads here.
                if packet and packet[0] in ("m", "x") and "address_space:" in packet:
                    space = 0
                    for field in packet[1:].split(";"):
                        key, _, value = field.partition(":")
                        if key == "address_space":
                            space = int(value, 16)
                    return self._bytes_for_space(space)
                return super()._respond_impl(packet)

            def x(self, addr, length):
                # Force the client onto the hex "m" read path.
                return ""

            def other(self, packet):
                if packet == "jAddressSpacesInfo":
                    return escape_binary(address_spaces_json)
                return ""

        self.server.responder = MyResponder()
        target = self.dbg.CreateTarget("")
        process = self.connect(target)

        error = lldb.SBError()

        # Same numeric address, two spaces (global == id 1, local == id 2).
        global_bytes = process.ReadMemory(lldb.SBProcessAddress(0x1000, 1), 4, error)
        self.assertSuccess(error)
        self.assertEqual(global_bytes, b"\xaa\xbb\xcc\xdd")

        local_bytes = process.ReadMemory(lldb.SBProcessAddress(0x1000, 2), 4, error)
        self.assertSuccess(error)
        self.assertEqual(local_bytes, b"\x11\x22\x33\x44")

        # Same address, different address space, different bytes.
        self.assertNotEqual(global_bytes, local_bytes)
