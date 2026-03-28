#!/usr/bin/env python3
"""
test_xmodem_littlefs.py - Automated XMODEM LittleFS Integration Test

Requirement: Req #17 (LittleFS Integration Test)
Sub-reqs:    17.1-17.8

Port of the 1180 test_xmodem_fatfs.py adapted for the Bobbycar-Steering
ESP32 firmware with LittleFS filesystem and XMODEM-CRC file transfer.

Steps:
    17.1  Format filesystem
    17.2  Push File 1 (5 KB)
    17.3  Push File 2 (4 KB, different content)
    17.4  Read-back and compare Files 1 and 2 (via XMODEM send)
    17.5  Push File 3 (4 KB)
    17.6  Delete File 2, verify Files 1 + 3 intact
    17.7  Create folder, push nested file
    17.8  Reset device, verify persistence

Verification: Since the device has no on-device hash command, files are
verified by sending them back via XMODEM and comparing byte-for-byte
against the original data.

Usage:
    python Target/test_xmodem_littlefs.py --port COM13 [--baud 115200]

Dependencies:
    pip install pyserial xmodem
"""

import sys
import os
import time
import hashlib
import random
import argparse
import tempfile
import io

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
REPO_ROOT = os.path.join(SCRIPT_DIR, "..")

try:
    import serial
    from xmodem import XMODEM
except ImportError:
    print("ERROR: Required packages not installed. Run:")
    print("  pip install pyserial xmodem")
    sys.exit(1)

# Test file sizes (must fit in 8 KB XMODEM buffer)
FILE1_SIZE = 5 * 1024    # 5 KB
FILE2_SIZE = 4 * 1024    # 4 KB
FILE3_SIZE = 4 * 1024    # 4 KB
NESTED_SIZE = 1 * 1024   # 1 KB

# Fixed seed for deterministic test data
RANDOM_SEED = 0xDEADBEEF

# Timeouts
CMD_TIMEOUT = 10.0
FORMAT_TIMEOUT = 60.0
XMODEM_TIMEOUT = 30.0


class TestResult:
    """Track pass/fail for each sub-step."""

    def __init__(self):
        self.results = []

    def record(self, step_id, description, passed, elapsed, detail=""):
        self.results.append({
            "step": step_id,
            "desc": description,
            "pass": passed,
            "time": elapsed,
            "detail": detail,
        })
        status = "PASS" if passed else "FAIL"
        msg = f"[{status}] {step_id}: {description} ({elapsed:.1f}s)"
        if detail:
            msg += f" - {detail}"
        print(msg)

    def summary(self):
        print("\n" + "=" * 60)
        print("TEST SUMMARY")
        print("=" * 60)
        total = len(self.results)
        passed = sum(1 for r in self.results if r["pass"])
        failed = total - passed
        for r in self.results:
            status = "PASS" if r["pass"] else "FAIL"
            print(f"  [{status}] {r['step']}: {r['desc']}")
        print(f"\n{passed}/{total} passed, {failed} failed")
        print("=" * 60)
        return failed == 0


def generate_test_data(size, seed):
    """Generate deterministic pseudo-random test data."""
    rng = random.Random(seed)
    return bytes(rng.getrandbits(8) for _ in range(size))


def sha256_hex(data):
    """Compute SHA-256 hex digest."""
    return hashlib.sha256(data).hexdigest()


def wait_for(ser, pattern, timeout=CMD_TIMEOUT):
    """Read serial until pattern found or timeout."""
    buf = b""
    start = time.time()
    while time.time() - start < timeout:
        if ser.in_waiting:
            buf += ser.read(ser.in_waiting)
            text = buf.decode("utf-8", errors="replace")
            if pattern in text:
                return text
        else:
            time.sleep(0.05)
    text = buf.decode("utf-8", errors="replace")
    raise TimeoutError(
        f"Timeout waiting for '{pattern}'. "
        f"Got last 300 chars: {text[-300:]}"
    )


def drain(ser, delay=0.3):
    """Drain buffered serial data."""
    time.sleep(delay)
    ser.reset_input_buffer()


def send_cmd(ser, cmd, expect=None, timeout=CMD_TIMEOUT):
    """Send command and optionally wait for expected response."""
    drain(ser, 0.2)
    ser.write(f"{cmd}\r\n".encode())
    if expect:
        return wait_for(ser, expect, timeout)
    time.sleep(1.0)
    buf = b""
    if ser.in_waiting:
        buf = ser.read(ser.in_waiting)
    return buf.decode("utf-8", errors="replace")


def xmodem_send_file(ser, remote_path, data):
    """Send data to device via XMODEM-CRC after issuing recv command."""
    drain(ser, 0.1)
    ser.write(f"recv {remote_path}\r\n".encode())

    try:
        wait_for(ser, "Ready to receive", timeout=10.0)
    except TimeoutError:
        print(f"  ERROR: Device not ready for recv {remote_path}")
        return False

    time.sleep(0.5)
    ser.reset_input_buffer()

    def getc(size, timeout=1):
        d = ser.read(size)
        return d if d else None

    def putc(d, timeout=1):
        return ser.write(d)

    modem = XMODEM(getc, putc)

    with tempfile.NamedTemporaryFile(delete=False, suffix=".bin") as tmp:
        tmp.write(data)
        tmp_path = tmp.name

    try:
        with open(tmp_path, "rb") as f:
            success = modem.send(f, retry=16)
    finally:
        os.unlink(tmp_path)

    # Wait for device to finish writing
    time.sleep(3.0)
    if ser.in_waiting:
        resp = ser.read(ser.in_waiting).decode("utf-8", errors="replace")
        if "ERROR" in resp or "error" in resp:
            print(f"  Device reported error: {resp.strip()}")
            return False

    drain(ser, 1.0)
    ser.write(b"\r\n")
    time.sleep(1.0)
    drain(ser, 0.5)

    return success


def xmodem_recv_file(ser, remote_path):
    """Receive file from device via XMODEM-CRC send command.

    Returns the received bytes or None on failure.
    """
    drain(ser, 0.1)
    ser.write(f"send {remote_path}\r\n".encode())

    try:
        wait_for(ser, "Start receive now", timeout=10.0)
    except TimeoutError:
        print(f"  ERROR: Device not ready for send {remote_path}")
        return None

    time.sleep(0.5)
    ser.reset_input_buffer()

    def getc(size, timeout=1):
        d = ser.read(size)
        return d if d else None

    def putc(d, timeout=1):
        return ser.write(d)

    modem = XMODEM(getc, putc)

    with tempfile.NamedTemporaryFile(delete=False, suffix=".bin") as tmp:
        tmp_path = tmp.name

    try:
        with open(tmp_path, "wb") as f:
            success = modem.recv(f, retry=16)
        if success:
            with open(tmp_path, "rb") as f:
                data = f.read()
        else:
            data = None
    finally:
        os.unlink(tmp_path)

    time.sleep(2.0)
    drain(ser, 1.0)
    ser.write(b"\r\n")
    time.sleep(1.0)
    drain(ser, 0.5)

    return data


def verify_file(ser, remote_path, expected_data):
    """Verify a file on the device by XMODEM read-back.

    Returns True if the received data matches expected_data
    (truncated to the expected length, since XMODEM pads to
    128-byte blocks).
    """
    received = xmodem_recv_file(ser, remote_path)
    if received is None:
        return False
    # XMODEM pads to 128-byte boundaries; trim to expected length
    trimmed = received[:len(expected_data)]
    return trimmed == expected_data


# -------------------------------------------------------------------
# Test Steps
# -------------------------------------------------------------------

def test_17_1_format(ser, results):
    """17.1 Format filesystem."""
    t0 = time.time()
    try:
        resp = send_cmd(ser, "format", "Format complete",
                        timeout=FORMAT_TIMEOUT)
        passed = "Format complete" in resp
        results.record("17.1", "Format filesystem",
                       passed, time.time() - t0)
    except Exception as e:
        results.record("17.1", "Format filesystem",
                       False, time.time() - t0, str(e))


def test_17_2_push_file1(ser, file1_data, results):
    """17.2 Push File 1 (5 KB)."""
    t0 = time.time()
    try:
        ok = xmodem_send_file(ser, "/test_file1.bin", file1_data)
        if ok:
            ls_resp = send_cmd(ser, "ls /", "test_file1",
                               timeout=15.0)
            ok = "test_file1" in ls_resp
        results.record("17.2",
                       f"Push File 1 ({len(file1_data)} bytes)",
                       ok, time.time() - t0)
    except Exception as e:
        results.record("17.2",
                       f"Push File 1 ({len(file1_data)} bytes)",
                       False, time.time() - t0, str(e))


def test_17_3_push_file2(ser, file2_data, results):
    """17.3 Push File 2 (4 KB)."""
    t0 = time.time()
    try:
        ok = xmodem_send_file(ser, "/test_file2.bin", file2_data)
        if ok:
            ls_resp = send_cmd(ser, "ls /", "test_file2",
                               timeout=CMD_TIMEOUT)
            ok = "test_file2" in ls_resp
        results.record("17.3",
                       f"Push File 2 ({len(file2_data)} bytes)",
                       ok, time.time() - t0)
    except Exception as e:
        results.record("17.3",
                       f"Push File 2 ({len(file2_data)} bytes)",
                       False, time.time() - t0, str(e))


def test_17_4_readback_compare(ser, file1_data, file2_data, results):
    """17.4 Read-back and compare Files 1 and 2 via XMODEM."""
    t0 = time.time()
    try:
        match1 = verify_file(ser, "/test_file1.bin", file1_data)
        match2 = verify_file(ser, "/test_file2.bin", file2_data)

        detail = ""
        if not match1:
            detail += "File1 mismatch. "
        if not match2:
            detail += "File2 mismatch. "

        results.record("17.4",
                       "Read-back compare Files 1+2",
                       match1 and match2,
                       time.time() - t0, detail)
    except Exception as e:
        results.record("17.4",
                       "Read-back compare Files 1+2",
                       False, time.time() - t0, str(e))


def test_17_5_push_file3(ser, file3_data, results):
    """17.5 Push File 3 (4 KB)."""
    t0 = time.time()
    try:
        ok = xmodem_send_file(ser, "/test_file3.bin", file3_data)
        if ok:
            ls_resp = send_cmd(ser, "ls /", "test_file3",
                               timeout=CMD_TIMEOUT)
            ok = "test_file3" in ls_resp
        results.record("17.5",
                       f"Push File 3 ({len(file3_data)} bytes)",
                       ok, time.time() - t0)
    except Exception as e:
        results.record("17.5",
                       f"Push File 3 ({len(file3_data)} bytes)",
                       False, time.time() - t0, str(e))


def test_17_6_delete_file2(ser, file1_data, file3_data, results):
    """17.6 Delete File 2, verify Files 1+3 intact."""
    t0 = time.time()
    try:
        resp = send_cmd(ser, "rm /test_file2.bin", "Removed",
                        timeout=CMD_TIMEOUT)
        deleted = "Removed" in resp

        ls_resp = send_cmd(ser, "ls /", ">", timeout=CMD_TIMEOUT)
        time.sleep(1.0)
        if ser.in_waiting:
            ls_resp += ser.read(
                ser.in_waiting
            ).decode("utf-8", errors="replace")
        file2_gone = "test_file2" not in ls_resp

        match1 = verify_file(ser, "/test_file1.bin", file1_data)
        match3 = verify_file(ser, "/test_file3.bin", file3_data)

        ok = deleted and file2_gone and match1 and match3
        detail = ""
        if not deleted:
            detail += "rm failed. "
        if not file2_gone:
            detail += "file2 still listed. "
        if not match1:
            detail += "file1 mismatch. "
        if not match3:
            detail += "file3 mismatch. "
        results.record("17.6",
                       "Delete File 2, verify Files 1+3",
                       ok, time.time() - t0, detail)
    except Exception as e:
        results.record("17.6",
                       "Delete File 2, verify Files 1+3",
                       False, time.time() - t0, str(e))


def test_17_7_mkdir_and_nested(ser, nested_data, results):
    """17.7 Create folder and push nested file."""
    t0 = time.time()
    try:
        resp = send_cmd(ser, "mkdir /testdir", "Created",
                        timeout=CMD_TIMEOUT)
        mkdir_ok = "Created" in resp

        ls_resp = send_cmd(ser, "ls /", "testdir",
                           timeout=CMD_TIMEOUT)
        dir_listed = "testdir" in ls_resp

        nested_ok = xmodem_send_file(
            ser, "/testdir/nested.bin", nested_data
        )

        if nested_ok:
            ls2 = send_cmd(ser, "ls /testdir", "nested",
                           timeout=CMD_TIMEOUT)
            nested_ok = "nested" in ls2

        ok = mkdir_ok and dir_listed and nested_ok
        detail = ""
        if not mkdir_ok:
            detail += "mkdir failed. "
        if not dir_listed:
            detail += "dir not listed. "
        if not nested_ok:
            detail += "nested file failed. "
        results.record("17.7",
                       "Create folder + nested file",
                       ok, time.time() - t0, detail)
    except Exception as e:
        results.record("17.7",
                       "Create folder + nested file",
                       False, time.time() - t0, str(e))


def test_17_8_persistence(ser, file1_data, file3_data,
                          nested_data, results):
    """17.8 Reset device and verify all files persist."""
    t0 = time.time()
    try:
        ser.write(b"reset\r\n")
        time.sleep(6.0)

        drain(ser, 3.0)

        # Wait for console ready
        ser.write(b"\r\n")
        try:
            wait_for(ser, ">", timeout=10.0)
        except TimeoutError:
            pass
        drain(ser, 1.0)

        # Verify file 1
        match1 = verify_file(ser, "/test_file1.bin", file1_data)

        # Verify file 3
        match3 = verify_file(ser, "/test_file3.bin", file3_data)

        # Verify nested file
        match_n = verify_file(
            ser, "/testdir/nested.bin", nested_data
        )

        # Verify file 2 still deleted
        ls_resp = send_cmd(ser, "ls /", ">",
                           timeout=CMD_TIMEOUT)
        time.sleep(1.0)
        if ser.in_waiting:
            ls_resp += ser.read(
                ser.in_waiting
            ).decode("utf-8", errors="replace")
        file2_gone = "test_file2" not in ls_resp

        ok = match1 and match3 and match_n and file2_gone
        detail = ""
        if not match1:
            detail += "file1 mismatch. "
        if not match3:
            detail += "file3 mismatch. "
        if not match_n:
            detail += "nested mismatch. "
        if not file2_gone:
            detail += "file2 reappeared. "
        results.record("17.8", "Persistence after reset",
                       ok, time.time() - t0, detail)
    except Exception as e:
        results.record("17.8", "Persistence after reset",
                       False, time.time() - t0, str(e))


def main():
    parser = argparse.ArgumentParser(
        description="XMODEM LittleFS Integration Test (Req #17)")
    parser.add_argument("--port", required=True,
                        help="Serial port (e.g. COM13)")
    parser.add_argument("--baud", type=int, default=115200,
                        help="Baud rate (default: 115200)")
    parser.add_argument("--skip-format", action="store_true",
                        help="Skip format step")
    parser.add_argument("--skip-persistence", action="store_true",
                        help="Skip persistence test (requires reset)")
    args = parser.parse_args()

    print("=" * 60)
    print("XMODEM LittleFS Integration Test - Req #17")
    print(f"Port: {args.port} @ {args.baud}")
    print("=" * 60)

    # Generate deterministic test data
    print("\nGenerating test data...")
    file1_data = generate_test_data(FILE1_SIZE, RANDOM_SEED)
    file2_data = generate_test_data(FILE2_SIZE, RANDOM_SEED + 1)
    file3_data = generate_test_data(FILE3_SIZE, RANDOM_SEED + 2)
    nested_data = generate_test_data(NESTED_SIZE, RANDOM_SEED + 3)

    print(f"  File 1: {len(file1_data)} bytes, "
          f"SHA-256={sha256_hex(file1_data)[:16]}...")
    print(f"  File 2: {len(file2_data)} bytes, "
          f"SHA-256={sha256_hex(file2_data)[:16]}...")
    print(f"  File 3: {len(file3_data)} bytes, "
          f"SHA-256={sha256_hex(file3_data)[:16]}...")
    print(f"  Nested: {len(nested_data)} bytes, "
          f"SHA-256={sha256_hex(nested_data)[:16]}...")

    print(f"\nOpening {args.port}...")
    ser = serial.Serial(args.port, args.baud, timeout=1)
    time.sleep(0.5)
    drain(ser)

    results = TestResult()

    try:
        if not args.skip_format:
            print("\n--- 17.1 Format Filesystem ---")
            test_17_1_format(ser, results)
        else:
            print("\n--- 17.1 Format Filesystem [SKIPPED] ---")

        print("\n--- 17.2 Push File 1 ---")
        test_17_2_push_file1(ser, file1_data, results)

        print("\n--- 17.3 Push File 2 ---")
        test_17_3_push_file2(ser, file2_data, results)

        print("\n--- 17.4 Read-Back Compare ---")
        test_17_4_readback_compare(
            ser, file1_data, file2_data, results
        )

        print("\n--- 17.5 Push File 3 ---")
        test_17_5_push_file3(ser, file3_data, results)

        print("\n--- 17.6 Delete File 2 ---")
        test_17_6_delete_file2(
            ser, file1_data, file3_data, results
        )

        print("\n--- 17.7 Create Folder + Nested File ---")
        test_17_7_mkdir_and_nested(ser, nested_data, results)

        if not args.skip_persistence:
            print("\n--- 17.8 Persistence After Reset ---")
            test_17_8_persistence(
                ser, file1_data, file3_data,
                nested_data, results
            )
        else:
            print("\n--- 17.8 Persistence [SKIPPED] ---")

    finally:
        ser.close()

    all_pass = results.summary()
    sys.exit(0 if all_pass else 1)


if __name__ == "__main__":
    main()
