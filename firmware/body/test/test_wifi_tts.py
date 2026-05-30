#!/usr/bin/env python3
"""
StackChan Wi-Fi TTS Integration Test

Usage:
    python3 test/test_wifi_tts.py [--ip IP] [--token TOKEN] [--wav PATH]

The script reads default values from bridge/config.yaml.
"""
import argparse
import json
import os
import sys
import time
import yaml

import requests
import serial


def load_bridge_config():
    config_path = os.path.join(os.path.dirname(__file__), "../../../bridge/config.yaml")
    if os.path.exists(config_path):
        with open(config_path, "r") as f:
            return yaml.safe_load(f)
    return {}


def build_parser(defaults):
    """Build argument parser with bridge config defaults."""
    wifi = defaults.get("wifi", {})
    parser = argparse.ArgumentParser(description="StackChan Wi-Fi TTS integration test")
    parser.add_argument(
        "--ip",
        default=wifi.get("base_url", "http://192.168.0.46"),
        help="StackChan device base URL",
    )
    parser.add_argument(
        "--token",
        default=wifi.get("token", ""),
        help="X-StackChan-Token value",
    )
    parser.add_argument(
        "--wav",
        default=os.path.join(os.path.dirname(__file__), "../../../bridge/test-assets/sample.wav"),
        help="Path to test WAV file",
    )
    parser.add_argument(
        "--serial-port",
        default="/dev/stackchan",
        help="Serial port for monitoring",
    )
    parser.add_argument(
        "--baud",
        type=int,
        default=115200,
        help="Serial baud rate",
    )
    parser.add_argument(
        "--skip-serial",
        action="store_true",
        help="Skip serial monitor output",
    )
    parser.add_argument(
        "--no-auth",
        action="store_true",
        help="Skip auth token (unprotected device)",
    )
    parser.add_argument(
        "--stop-wake-first",
        action="store_true",
        help="Stop wake word before playback tests",
    )
    parser.add_argument(
        "--json-output",
        action="store_true",
        help="Output results as JSON (for CI)",
    )
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Verbose output",
    )
    return parser


class StackChanTester:
    def __init__(self, base_url: str, token: str | None, wav_path: str, verbose: bool = False):
        self.base_url = base_url.rstrip("/")
        self.token = token
        self.wav_path = wav_path
        self.verbose = verbose
        self.session = requests.Session()
        self.session.headers.update({
            "User-Agent": "StackChan-Test/1.0",
        })
        if self.token:
            self.session.headers["X-StackChan-Token"] = self.token
        self.results = {}

    def _url(self, path: str) -> str:
        return f"{self.base_url}{path}"

    def _request(self, method: str, path: str, data=None, files=None, raw_body=None):
        url = self._url(path)
        kwargs = {"timeout": 15}
        if data is not None:
            kwargs["json"] = data
        if raw_body is not None:
            kwargs["data"] = raw_body
            kwargs.setdefault("headers", {})
            kwargs["headers"]["Content-Type"] = "application/octet-stream"
        try:
            resp = self.session.request(method, url, **kwargs)
            print(f"  HTTP {resp.status_code} in {resp.elapsed.total_seconds():.2f}s")
            return resp
        except requests.exceptions.ConnectTimeout:
            print(f"  ERROR: Connection timeout to {url}")
            raise
        except requests.exceptions.ConnectionError as e:
            print(f"  ERROR: Cannot connect to {url}: {e}")
            raise

    def get_json(self, path: str) -> dict:
        r = self._request("GET", path)
        r.raise_for_status()
        body = r.json()
        if not body.get("ok"):
            print(f"  Device error: {body}")
        return body

    def post_json(self, path: str, data: dict) -> dict:
        r = self._request("POST", path, data=data)
        r.raise_for_status()
        body = r.json()
        if not body.get("ok"):
            print(f"  Device error: {body}")
        return body

    def post_raw(self, path: str, raw: bytes) -> dict:
        r = self._request("POST", path, raw_body=raw)
        r.raise_for_status()
        return r.json()

    def print_result(self, label: str, ok: bool, detail: str = ""):
        status = "PASS" if ok else "FAIL"
        print(f"  [{status}] {label}{': ' + detail if detail else ''}")
        return ok

    def record(self, name: str, ok: bool):
        self.results[name] = ok
        return ok

    # ------------------------------------------------------------------
    # Tests
    # ------------------------------------------------------------------

    def test_connectivity(self) -> bool:
        print("\n[1/6] Connectivity")
        try:
            body = self.get_json("/health")
            ok = body.get("ok") and body.get("data", {}).get("status") == "ok"
            self.print_result("/health", ok)
            return ok
        except Exception as e:
            self.print_result("/health", False, str(e))
            return False

    def test_wake_status(self) -> dict:
        print("\n[2/6] Wake Word status")
        try:
            body = self.get_json("/wake/status")
            data = body.get("data", {})
            print(f"  autoStart={data.get('autoStart')}, running={data.get('running')}, detected={data.get('detected')}")
            self.print_result("/wake/status", True)
            return data
        except Exception as e:
            self.print_result("/wake/status", False, str(e))
            return {}

    def test_audio_status(self) -> dict:
        print("\n[3/6] Audio status (before playback)")
        try:
            body = self.get_json("/audio/status")
            data = body.get("data", {})
            print(f"  state={data.get('state')}, playing={data.get('playing')}, volume={data.get('volume')}")
            self.print_result("/audio/status", True)
            return data
        except Exception as e:
            self.print_result("/audio/status", False, str(e))
            return {}

    def test_play_wav(self) -> bool:
        print("\n[4/6] Play WAV")
        if not os.path.exists(self.wav_path):
            self.print_result("WAV exists", False, f"Not found: {self.wav_path}")
            return False

        file_size = os.path.getsize(self.wav_path)
        print(f"  WAV file: {self.wav_path} ({file_size} bytes)")

        try:
            with open(self.wav_path, "rb") as f:
                wav_data = f.read()

            print("  Sending /play-wav (raw body)...")
            start = time.time()
            body = self.post_raw("/play-wav", wav_data)
            elapsed = time.time() - start

            ok = body.get("ok")
            data = body.get("data", {})
            print(f"  Response: {json.dumps(body, ensure_ascii=False)}")
            self.print_result("/play-wav responded", ok, f"{elapsed:.2f}s")
            return ok
        except Exception as e:
            self.print_result("/play-wav", False, str(e))
            return False

    def test_audio_during_and_after(self, expected_ms: int = 3000) -> bool:
        print(f"\n[5/6] Audio status during/after playback (expected ~{expected_ms}ms)")
        print("  Waiting for playback to start...")
        time.sleep(0.5)

        try:
            body = self.get_json("/audio/status")
            data = body.get("data", {})
            print(f"  After 0.5s: state={data.get('state')}, playing={data.get('playing')}")
        except Exception as e:
            print(f"  ERROR during status check: {e}")

        print(f"  Waiting {expected_ms}ms for expected playback duration...")
        time.sleep(expected_ms / 1000.0 + 0.5)

        try:
            body = self.get_json("/audio/status")
            data = body.get("data", {})
            print(f"  After wait: state={data.get('state')}, playing={data.get('playing')}")
            not_stuck = data.get("state") in ("Idle", "Finished")
            self.print_result("Playback completed / not stuck", not_stuck)
            return not_stuck
        except Exception as e:
            self.print_result("Post-playback check", False, str(e))
            return False

    def test_health_after(self) -> bool:
        print("\n[6/6] Connectivity after playback")
        try:
            body = self.get_json("/health")
            ok = body.get("ok")
            self.print_result("/health after playback", ok)
            return ok
        except Exception as e:
            self.print_result("/health after playback", False, str(e))
            return False


def run_serial_monitor(port: str, baud: int, duration_sec: float):
    """Print serial output for a limited time."""
    print(f"\n[Serial] Monitoring {port} @ {baud} for {duration_sec:.0f}s...")
    try:
        ser = serial.Serial(port, baud, timeout=1)
        start = time.time()
        while time.time() - start < duration_sec:
            line = ser.readline().decode("utf-8", errors="replace").rstrip()
            if line:
                print(f"  [SC] {line}")
        ser.close()
        print("[Serial] Monitor ended\n")
    except serial.SerialException as e:
        print(f"  [Serial] ERROR: {e}")


def main():
    config = load_bridge_config()
    parser = build_parser(config)
    args = parser.parse_args()

    if args.no_auth:
        token = None
    else:
        token = args.token or None

    print("=" * 60)
    print("StackChan Wi-Fi TTS Integration Test")
    print("=" * 60)
    print(f"Device:    {args.ip}")
    print(f"Auth:      {'yes' if token else 'no'}")
    print(f"WAV:       {args.wav}")
    print(f"Serial:    {args.serial_port} @ {args.baud} {'(skipped)' if args.skip_serial else ''}")

    tester = StackChanTester(args.ip, token, args.wav, verbose=args.verbose)

    # --- Run tests -----------------------------------------------------
    tester.test_connectivity()
    if not tester.results.get("connectivity"):
        print("\n!!! Device unreachable. Stopping tests.")
        if args.json_output:
            print(json.dumps(tester.summary(), indent=2))
        sys.exit(1)

    tester.test_wake_status()

    if args.stop_wake_first:
        tester.test_stop_wake()

    tester.test_audio_status()

    play_ok = tester.test_play_wav()

    if play_ok:
        # Estimate playback duration from WAV
        try:
            import wave
            with wave.open(args.wav, "rb") as wf:
                frames = wf.getnframes()
                rate = wf.getframerate()
                expected_ms = int(frames / rate * 1000)
        except Exception:
            expected_ms = 3000

        # Optionally monitor serial in parallel
        if not args.skip_serial:
            import threading
            monitor_thread = threading.Thread(
                target=run_serial_monitor,
                args=(args.serial_port, args.baud, expected_ms / 1000.0 + 2.0),
                daemon=True,
            )
            monitor_thread.start()

        tester.test_audio_during_and_after(expected_ms)
    else:
        tester.record("not_stuck", False)

    tester.test_health_after()

    # --- Summary -------------------------------------------------------
    summary = tester.summary()
    
    if args.json_output:
        print(json.dumps(summary, indent=2))
    else:
        print("\n" + "=" * 60)
        print("SUMMARY")
        print("=" * 60)
        for name, ok in results.items():
            status = "PASS" if ok else "FAIL"
            print(f"  [{status}] {name}")

    if summary["all_passed"]:
        if not args.json_output:
            print("\nAll tests passed! TTS Wi-Fi playback is working.")
        sys.exit(0)
    else:
        if not args.json_output:
            print("\nSome tests failed. Check output above.")
        sys.exit(1)


if __name__ == "__main__":
    main()
