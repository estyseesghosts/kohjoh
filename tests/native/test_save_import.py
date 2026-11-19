#!/usr/bin/env python3
"""Compile the production importer for the host and exercise data and files."""
import ctypes
import os
from pathlib import Path
import struct
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "src/compat/nitro/src/port_save_import.c"
SIZE = 512 * 1024
SNIP = b"|<--Snip above here to create a raw sav by excluding this DeSmuME savedata footer:"
COOKIE = b"|-DESMUME SAVE-|"
MAGIC = b"NocashGbaBackupMediaSavDataFile\x1a"


class SaveImportTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix="kohjoh-save-test-")
        library = Path(cls.temp.name) / ("import.dll" if os.name == "nt" else "import.so")
        subprocess.run(["gcc", "-std=gnu11", "-Wall", "-Wextra", "-Werror", "-shared",
                        "-DPORT_SAVE_IMPORT_TEST",
                        "-fPIC", str(SOURCE), "-o", str(library)], check=True)
        cls.lib = ctypes.CDLL(str(library))
        cls.lib.port_save_decode.argtypes = [ctypes.c_void_p, ctypes.c_size_t, ctypes.c_void_p,
                                             ctypes.POINTER(ctypes.c_char_p)]
        cls.lib.port_save_decode.restype = ctypes.c_bool
        cls.lib.port_save_import.argtypes = [ctypes.c_char_p, ctypes.c_void_p,
                                             ctypes.POINTER(ctypes.c_char_p)]
        cls.lib.port_save_import.restype = ctypes.c_bool
        cls.lib.port_save_import_failure.restype = ctypes.c_char_p
        cls.lib.port_save_import_errno.restype = ctypes.c_int
        cls.lib.port_save_has_backup.argtypes = [ctypes.c_char_p]
        cls.lib.port_save_has_backup.restype = ctypes.c_bool
        cls.raw = bytes(range(256)) * (SIZE // 256)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def decode(self, data):
        buf = ctypes.create_string_buffer(data)
        out = ctypes.create_string_buffer(SIZE + 16)
        ctypes.memset(ctypes.addressof(out) + SIZE, 0xa5, 16)
        kind = ctypes.c_char_p()
        ok = self.lib.port_save_decode(buf, len(data), out, ctypes.byref(kind))
        self.assertEqual(out.raw[SIZE:], b"\xa5" * 16)
        return ok, out.raw[:SIZE], kind.value

    @staticmethod
    def dsv(raw):
        return raw + SNIP + struct.pack("<6I", SIZE, SIZE, 6, 3, SIZE, 0) + COOKIE

    @staticmethod
    def nocash(raw):
        header = bytearray(0x4c)
        header[:len(MAGIC)] = MAGIC
        header[0x40:0x44] = b"SRAM"
        struct.pack_into("<II", header, 0x44, 0, len(raw))
        return bytes(header) + raw

    @staticmethod
    def packed(body, declared=SIZE):
        header = bytearray(0x50)
        header[:len(MAGIC)] = MAGIC
        header[0x40:0x44] = b"SRAM"
        # The length at 0x48 counts from 0x4c: the size word plus the body.
        struct.pack_into("<III", header, 0x44, 1, 4 + len(body), declared)
        return bytes(header) + body

    def test_supported_formats(self):
        # Mix literal, short fill and long fill; contents are known independently.
        body = b"\x03ABC\x82Z" + b"\x80\xff\xff\xff" * 8 + b"\x80\xff\x03\x00\x00"
        expected = b"ABCZZ" + b"\xff" * (SIZE - 5)
        for data, raw in ((self.dsv(self.raw), self.raw), (self.nocash(self.raw), self.raw),
                          (self.packed(body), expected)):
            ok, output, kind = self.decode(data)
            self.assertTrue(ok)
            self.assertEqual(output, raw)
            self.assertIsNotNone(kind)

    def test_trailing_chunks_after_sram_block(self):
        # Real No$GBA exports (a Platinum and a HeartGold packed save) end
        # with a LIFF block and a STOP marker after the SRAM block's
        # declared length; those bytes are not save data.
        tail = (b"LIFF" + bytes([1, 0, 0, 0, 9, 0, 0, 0, 0, 0, 1, 0, 0, 0x80, 0, 0, 1, 0])
                + b"STOP" + bytes(10))
        body = b"\x03ABC\x82Z" + b"\x80\xff\xff\xff" * 8 + b"\x80\xff\x03\x00\x00"
        expected = b"ABCZZ" + b"\xff" * (SIZE - 5)
        for data, raw in ((self.packed(body) + tail, expected),
                          (self.nocash(self.raw) + tail, self.raw)):
            ok, output, kind = self.decode(data)
            self.assertTrue(ok)
            self.assertEqual(output, raw)
        # The terminator must still end exactly the declared packed data.
        early = self.packed(body + b"\x00")
        self.assertFalse(self.decode(early[:0x48] + (4 + len(body) + 1).to_bytes(4, "little")
                                     + early[0x4c:] + tail)[0])
        short = bytearray(self.packed(body) + tail)
        short[0x48:0x4c] = (4 + len(body) - 1).to_bytes(4, "little")
        self.assertFalse(self.decode(bytes(short))[0])

    def test_packed_container_can_equal_raw_image_size(self):
        # A legal compressed file can have the same length as a raw image.
        # The decoder accepts it, but open_save currently gives raw size
        # precedence; the file must be exported to raw before use there.
        body = bytearray(b"\x80\xff\x55\x10")  # 4181 bytes of 0xff
        remaining = SIZE - 4181
        while remaining:
            count = min(remaining, 127)
            body.append(count)
            body.extend(b"A" * count)
            remaining -= count
        body.append(0)
        container = self.packed(body)
        self.assertEqual(len(container), SIZE)
        ok, output, kind = self.decode(container)
        self.assertTrue(ok)
        self.assertEqual(output, b"\xff" * 4181 + b"A" * (SIZE - 4181))
        self.assertEqual(kind, b"No$GBA packed")

    def test_reject_malformed(self):
        valid_dsv = self.dsv(self.raw)
        malformed_dsv = bytearray(valid_dsv)
        struct.pack_into("<I", malformed_dsv, SIZE + len(SNIP) + 4, SIZE - 1)
        bad_method = bytearray(self.nocash(self.raw))
        struct.pack_into("<I", bad_method, 0x44, 2)
        wrong_length = bytearray(self.packed(b"\x00"))
        struct.pack_into("<I", wrong_length, 0x48, 20)
        samples = [self.raw, valid_dsv[:-1], bytes(malformed_dsv), bytes(bad_method),
                   bytes(wrong_length), self.nocash(self.raw)[:-1],
                   self.packed(b"\x80\x33\x00\x00\x00"),
                   self.packed(b"\x7fA"), self.packed(b"\x80\x33\xff"),
                   self.packed(b"\x80\xff\xff\xff" * 9 + b"\x00"),
                   self.packed(b"\x00", SIZE - 1), self.packed(b"\x00")]
        for data in samples:
            with self.subTest(size=len(data), prefix=data[:8]):
                self.assertFalse(self.decode(data)[0])

    def test_import_preserves_original_and_collision(self):
        directory = Path(self.temp.name) / "success"
        directory.mkdir()
        path = directory / "heartgold.sav"
        original = self.dsv(self.raw)
        path.write_bytes(original)
        backup = Path(str(path) + ".orig")
        backup.write_bytes(b"old backup")
        out = ctypes.create_string_buffer(SIZE)
        kind = ctypes.c_char_p()
        self.assertTrue(self.lib.port_save_import(os.fsencode(path), out, ctypes.byref(kind)))
        self.assertEqual(path.read_bytes(), self.raw)
        self.assertEqual(backup.read_bytes(), b"old backup")
        self.assertEqual(Path(str(path) + ".orig.1").read_bytes(), original)
        self.assertTrue(self.lib.port_save_has_backup(os.fsencode(path)))
        # A second conversion attempt rejects raw data and leaves every file untouched.
        self.assertFalse(self.lib.port_save_import(os.fsencode(path), out, ctypes.byref(kind)))
        self.assertEqual(path.read_bytes(), self.raw)

    def test_import_nocash_formats(self):
        body = b"\x80\xff\xff\xff" * 8 + b"\x80\xff\x08\x00\x00"
        for name, original, raw in (
            ("uncompressed", self.nocash(self.raw), self.raw),
            ("packed", self.packed(body), b"\xff" * SIZE),
        ):
            with self.subTest(format=name):
                directory = Path(self.temp.name) / name
                directory.mkdir()
                path = directory / "heartgold.sav"
                path.write_bytes(original)
                out = ctypes.create_string_buffer(SIZE)
                kind = ctypes.c_char_p()
                self.assertTrue(self.lib.port_save_import(os.fsencode(path), out,
                                                          ctypes.byref(kind)))
                self.assertEqual(path.read_bytes(), raw)
                self.assertEqual(out.raw, raw)
                self.assertEqual(Path(str(path) + ".orig").read_bytes(), original)
                self.assertFalse(self.lib.port_save_import(os.fsencode(path), out,
                                                           ctypes.byref(kind)))
                self.assertEqual(path.read_bytes(), raw)

    def test_collision_with_non_eexist_errno(self):
        directory = Path(self.temp.name) / "collision-errno"
        directory.mkdir()
        path = directory / "heartgold.sav"
        original = self.dsv(self.raw)
        path.write_bytes(original)
        backup = Path(str(path) + ".orig")
        backup.write_bytes(b"existing backup")
        out = ctypes.create_string_buffer(SIZE)
        kind = ctypes.c_char_p()
        os.environ["PORT_SAVE_IMPORT_TEST_FAULT"] = "collision_errno"
        try:
            self.assertTrue(self.lib.port_save_import(os.fsencode(path), out, ctypes.byref(kind)))
        finally:
            del os.environ["PORT_SAVE_IMPORT_TEST_FAULT"]
        self.assertEqual(backup.read_bytes(), b"existing backup")
        self.assertEqual(Path(str(path) + ".orig.1").read_bytes(), original)
        self.assertEqual(path.read_bytes(), self.raw)
        self.assertIsNone(self.lib.port_save_import_failure())

    def test_backup_creation_failure_leaves_input(self):
        directory = Path(self.temp.name) / "full"
        directory.mkdir()
        path = directory / "heartgold.sav"
        original = self.dsv(self.raw)
        path.write_bytes(original)
        for index in range(1000):
            suffix = ".orig" if index == 0 else f".orig.{index}"
            Path(str(path) + suffix).write_bytes(b"occupied")
        out = ctypes.create_string_buffer(SIZE)
        kind = ctypes.c_char_p()
        self.assertFalse(self.lib.port_save_import(os.fsencode(path), out, ctypes.byref(kind)))
        self.assertEqual(path.read_bytes(), original)
        self.assertTrue(self.lib.port_save_has_backup(os.fsencode(path)))

    def test_interrupted_writes_and_replace(self):
        for fault in ("backup", "temp", "rename"):
            with self.subTest(fault=fault):
                directory = Path(self.temp.name) / fault
                directory.mkdir()
                path = directory / "heartgold.sav"
                original = self.dsv(self.raw)
                path.write_bytes(original)
                out = ctypes.create_string_buffer(SIZE)
                kind = ctypes.c_char_p()
                os.environ["PORT_SAVE_IMPORT_TEST_FAULT"] = fault
                try:
                    self.assertFalse(self.lib.port_save_import(os.fsencode(path), out,
                                                               ctypes.byref(kind)))
                    expected_stage = {"backup": b"write or verify backup",
                                      "temp": b"write or verify temporary image",
                                      "rename": b"replace active save"}
                    self.assertEqual(self.lib.port_save_import_failure(), expected_stage[fault])
                finally:
                    del os.environ["PORT_SAVE_IMPORT_TEST_FAULT"]
                backup = Path(str(path) + ".orig")
                self.assertTrue(backup.exists())
                if fault == "backup":
                    self.assertEqual(backup.read_bytes(), original[:len(original) // 2])
                    self.assertEqual(path.read_bytes(), original)
                elif fault == "temp":
                    self.assertEqual(backup.read_bytes(), original)
                    self.assertEqual(path.read_bytes(), original)
                else:
                    self.assertEqual(backup.read_bytes(), original)
                    self.assertFalse(path.exists())
                self.assertTrue(self.lib.port_save_has_backup(os.fsencode(path)))

    def test_backup_directory_scan(self):
        directory = Path(self.temp.name) / "scan"
        directory.mkdir()
        path = directory / "heartgold.sav"
        unrelated = ["heartgold.sav.origx", "heartgold.sav.orig.foo", "heartgold.sav.tmp.",
                     "heartgold.sav2.orig", "heartgold.sa.orig"]
        for name in unrelated:
            (directory / name).write_bytes(b"x")
        self.assertFalse(self.lib.port_save_has_backup(os.fsencode(path)))
        numbered = directory / "heartgold.sav.orig.27"
        numbered.write_bytes(b"original")
        self.assertTrue(self.lib.port_save_has_backup(os.fsencode(path)))
        numbered.unlink()
        (directory / "heartgold.sav.tmp.42").write_bytes(b"temporary")
        self.assertTrue(self.lib.port_save_has_backup(os.fsencode(path)))
        missing_parent = directory / "missing" / "heartgold.sav"
        self.assertTrue(self.lib.port_save_has_backup(os.fsencode(missing_parent)))


if __name__ == "__main__":
    unittest.main()
