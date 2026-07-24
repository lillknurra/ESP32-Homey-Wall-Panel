#!/usr/bin/env python3
from pathlib import Path
import glob
import hashlib
import json
import os
import re
import subprocess
import sys
import time
import zipfile

REPO = Path.home() / "GitHub" / "ESP32-Homey-Wall-Panel"
IDF = Path.home() / "GitHub" / "esp-idf-v6.0.1"
FW = REPO / "build" / "esp32_homey_wall_panel.bin"
DOWNLOADS = Path.home() / "Downloads"
CAPTURE_HELPER = r'''
import serial
import sys
import time

port = sys.argv[1]
seconds = float(sys.argv[2])
reset_mode = sys.argv[3]

ser = serial.Serial(port=port, baudrate=115200, timeout=0.10)
try:
    if reset_mode == "electronic":
        ser.dtr = False
        ser.rts = True
        time.sleep(0.15)
        ser.rts = False
        time.sleep(0.15)
    deadline = time.monotonic() + seconds
    while time.monotonic() < deadline:
        try:
            data = ser.read(4096)
        except serial.SerialException:
            if reset_mode == "reprovision":
                break
            raise
        if data:
            sys.stdout.buffer.write(data)
            sys.stdout.buffer.flush()
finally:
    ser.close()
'''

REQUIRED_BOOT_MARKERS = [
    "Patch 005H.1 secure local hardware bootstrap",
    "BOOTSTRAP_READY display=true touch=true softap=true oauth_locked=true runtime_rotation=true",
    "Provisioning code is displayed locally and is never logged",
]
CLOSED_MARKER = (
    "BOOTSTRAP_CLOSED softap=false http=false code_wiped=true "
    "physical_reopen_required=true ip_obtained=true"
)
IP_MARKER = "BOOTSTRAP_STATUS wifi_configured=true ip_obtained=true oauth_locked=true"
PHYSICAL_MARKER = "BOOTSTRAP_PHYSICAL_REPROVISION hold_ms=5000 accepted=true"
RESTART_MARKER = "BOOTSTRAP_RESTARTING reason=physical_reprovision"


def fresh_bootstrap_tail(*segments: str) -> str:
    """Return only serial output after the final physical-reprovision restart marker."""
    combined = "".join(segments)
    marker_at = combined.rfind(RESTART_MARKER)
    if marker_at < 0:
        return ""
    return combined[marker_at + len(RESTART_MARKER):]


def has_fresh_bootstrap_markers(*segments: str) -> bool:
    tail = fresh_bootstrap_tail(*segments)
    return bool(tail) and all(marker in tail for marker in REQUIRED_BOOT_MARKERS)


class MandatoryCheckFailed(RuntimeError):
    def __init__(self, check_name: str):
        super().__init__(check_name)
        self.check_name = check_name


def fail(message: str) -> None:
    print(f"FAIL: {message}", file=sys.stderr)
    raise SystemExit(1)


def confirm(prompt: str) -> bool:
    answer = input(f"{prompt} [yes/no]: ").strip().lower()
    if answer not in {"yes", "no"}:
        fail("answer must be exactly yes or no")
    return answer == "yes"


def require_confirm(checks: dict[str, bool], name: str, prompt: str) -> None:
    passed = confirm(prompt)
    checks[name] = passed
    if not passed:
        raise MandatoryCheckFailed(name)


def ports() -> list[str]:
    return sorted(set(glob.glob("/dev/cu.usbmodem*") + glob.glob("/dev/cu.usbserial*")))


def wait_for_port(port: str, present: bool, timeout: float) -> bool:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if Path(port).exists() == present:
            return True
        time.sleep(0.20)
    return False


def capture_serial(port: str, seconds: int, reset_mode: str) -> str:
    command = (
        f'. "{IDF}/export.sh" >/dev/null && '
        'python -c "$CAPTURE_HELPER" "$SERIAL_PORT" "$CAPTURE_SECONDS" "$RESET_MODE"'
    )
    result = subprocess.run(
        ["/bin/bash", "-c", command],
        cwd=REPO,
        text=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        env={
            **os.environ,
            "CAPTURE_HELPER": CAPTURE_HELPER,
            "SERIAL_PORT": port,
            "CAPTURE_SECONDS": str(seconds),
            "RESET_MODE": reset_mode,
            "PYTHONDONTWRITEBYTECODE": "1",
        },
        check=False,
    )
    raw_output = result.stdout or b""
    decoded_output = raw_output.decode("utf-8", errors="replace")
    if result.returncode != 0:
        fail(f"serial capture failed ({result.returncode}):\n{decoded_output}")
    return decoded_output


def capture_boot(port: str) -> tuple[str, str]:
    print("Opening serial port before reset and attempting an electronic DTR/RTS reset...")
    boot = capture_serial(port, 45, "electronic")
    if all(marker in boot for marker in REQUIRED_BOOT_MARKERS):
        return boot, "ELECTRONIC_DTR_RTS"

    print("\nThe electronic reset did not produce all boot markers.")
    print("This Waveshare model has no separate RESET button.")
    print("Disconnect USB, wait at least 3 seconds, then reconnect it.")
    input("Press Enter BEFORE disconnecting USB: ")
    if not wait_for_port(port, False, 30):
        fail("serial port did not disappear; USB was not disconnected")
    print("Reconnect USB now. Waiting for the serial port...")
    if not wait_for_port(port, True, 45):
        available = ", ".join(ports()) or "none"
        fail(f"serial port did not return as {port}; available: {available}")
    boot = capture_serial(port, 45, "none")
    return boot, "USB_POWER_CYCLE"


def sanitize(text: str) -> str:
    text = re.sub(r"WIFI:T:[^;]+;S:[^;]*;P:[^;]*;;", "[WIFI_QR_REDACTED]", text)
    text = re.sub(r"(?im)(wifi:connected with\s+)[^,\r\n]+", r"\1[SSID_REDACTED]", text)
    text = re.sub(r"(?im)(ssid\s*[:=]\s*)[^,;\r\n]+", r"\1[SSID_REDACTED]", text)
    text = re.sub(r"(?im)(softap\s+ssid\s*[:=]\s*)[^,;\r\n]+", r"\1[SSID_REDACTED]", text)
    text = re.sub(r"(?im)(password|passphrase|provisioning[_ -]?code|panel[_ -]?code)\s*[:=]\s*\S+", r"\1=[REDACTED]", text)
    text = re.sub(r"\b[0-9A-Fa-f]{2}(?::[0-9A-Fa-f]{2}){5}\b", "[MAC_REDACTED]", text)
    text = re.sub(r"\b(?:25[0-5]|2[0-4]\d|1?\d?\d)(?:\.(?:25[0-5]|2[0-4]\d|1?\d?\d)){3}\b", "[IP_REDACTED]", text)
    text = re.sub(r"\b[A-HJ-NP-Z2-9]{12}\b", "[CODE_REDACTED]", text)
    return text


def redaction_status(sanitized: str) -> dict[str, bool]:
    return {
        "ssid_in_evidence": bool(re.search(r"wifi:connected with\s+(?!\[SSID_REDACTED\])", sanitized, re.I)),
        "ip_in_evidence": bool(re.search(r"\b(?:25[0-5]|2[0-4]\d|1?\d?\d)(?:\.(?:25[0-5]|2[0-4]\d|1?\d?\d)){3}\b", sanitized)),
        "mac_in_evidence": bool(re.search(r"\b[0-9A-Fa-f]{2}(?::[0-9A-Fa-f]{2}){5}\b", sanitized)),
        "wifi_password_in_evidence": bool(re.search(r"(?i)(password|passphrase)\s*[:=]\s*(?!\[REDACTED\])\S+", sanitized)),
        "provisioning_code_in_evidence": bool(re.search(r"\b[A-HJ-NP-Z2-9]{12}\b", sanitized)),
    }


def write_evidence(
    *,
    sha256: str,
    flash_status: str,
    boot_capture_method: str,
    checks: dict[str, bool],
    serial_text: str,
    overall: str,
    failure: str | None,
) -> Path:
    stamp = time.strftime("%Y%m%d-%H%M%S")
    evidence = DOWNLOADS / f"patch-005h1e-hardware-{stamp}"
    evidence.mkdir(parents=True, exist_ok=False)
    sanitized = sanitize(serial_text)
    redaction = redaction_status(sanitized)
    redaction_pass = not any(redaction.values())
    if overall == "PASS" and not redaction_pass:
        overall = "FAIL"
        failure = failure or "evidence_redaction"
    (evidence / "serial-sanitized.log").write_text(sanitized, encoding="utf-8")
    manifest = {
        "firmware_sha256": sha256,
        "flash": flash_status,
        "boot_capture_method": boot_capture_method,
        "reset_button_required": False,
        "checks": checks,
        "overall": overall,
        "first_failure": failure,
        "oauth_locked": True,
        "oauth_performed": False,
        "ssid_in_evidence": redaction["ssid_in_evidence"],
        "ip_in_evidence": redaction["ip_in_evidence"],
        "mac_in_evidence": redaction["mac_in_evidence"],
        "wifi_password_in_evidence": redaction["wifi_password_in_evidence"],
        "provisioning_code_in_evidence": redaction["provisioning_code_in_evidence"],
        "evidence_redaction": redaction_pass,
        "commit": False,
        "push": False,
        "merge": False,
    }
    (evidence / "manifest.json").write_text(
        json.dumps(manifest, indent=2, sort_keys=True), encoding="utf-8"
    )
    output = Path(str(evidence) + ".zip")
    with zipfile.ZipFile(output, "w", zipfile.ZIP_DEFLATED) as archive:
        for path in evidence.rglob("*"):
            if path.is_file():
                archive.write(path, arcname=f"{evidence.name}/{path.relative_to(evidence)}")
    return output


def main() -> int:
    if not FW.exists():
        fail("firmware missing")
    serial_ports = ports()
    if not serial_ports:
        fail("serial port missing")
    for index, port_name in enumerate(serial_ports, 1):
        print(f"{index}. {port_name}")
    selection = input("Choose serial port: ").strip()
    if not selection.isdigit() or not 1 <= int(selection) <= len(serial_ports):
        fail("invalid port")
    port = serial_ports[int(selection) - 1]
    sha256 = hashlib.sha256(FW.read_bytes()).hexdigest()
    print("Firmware:", FW)
    print("SHA-256:", sha256)
    print("Type FLASH to flash, or CAPTURE to reuse the firmware already flashed.")
    action = input("Action [FLASH/CAPTURE]: ").strip()
    if action not in {"FLASH", "CAPTURE"}:
        fail("action must be exactly FLASH or CAPTURE")

    flash_status = "REUSED_PREVIOUS_PASS"
    if action == "FLASH":
        result = subprocess.run(
            ["/bin/bash", "-c", f'. "{IDF}/export.sh" && idf.py -p "{port}" flash'],
            cwd=REPO,
            text=True,
            check=False,
        )
        if result.returncode:
            fail("flash failed")
        flash_status = "PASS_CURRENT_RUN"
    else:
        print("Skipping flash. Existing firmware will be reused.")

    boot, boot_capture_method = capture_boot(port)
    combined = boot
    checks: dict[str, bool] = {"flash_transport": True}
    missing = [marker for marker in REQUIRED_BOOT_MARKERS if marker not in boot]
    if missing:
        checks["boot_markers"] = False
        output = write_evidence(
            sha256=sha256,
            flash_status=flash_status,
            boot_capture_method=boot_capture_method,
            checks=checks,
            serial_text=combined,
            overall="FAIL",
            failure="boot_markers",
        )
        fail("missing bootstrap markers; partial evidence: " + str(output))
    checks["boot_markers"] = True

    try:
        require_confirm(checks, "display_visible", "Is the display visible?")
        require_confirm(checks, "bootstrap_text_readable", "Is all bootstrap text clearly readable at normal wall distance?")
        require_confirm(checks, "code_label_separated", "Is the Code label clearly separated from the code with no overlap?")
        require_confirm(checks, "qr_visible", "Is the QR code fully visible?")
        require_confirm(checks, "qr_scannable", "Can the QR code be scanned?")
        require_confirm(checks, "touch_button_visible", "Is the Touch test button clearly visible?")

        print("Tap the on-screen Touch test button now.")
        touch_log = capture_serial(port, 15, "none")
        combined += touch_log
        checks["touch_event_detected"] = "BOOTSTRAP_TOUCH_EVENT detected=true" in touch_log
        if not checks["touch_event_detected"]:
            raise MandatoryCheckFailed("touch_event_detected")

        require_confirm(checks, "softap_visible", "Is HomeyPanel-Setup visible as a Wi-Fi network?")
        require_confirm(checks, "wrong_password_rejected", "Was an intentionally wrong SoftAP password rejected?")
        require_confirm(checks, "correct_displayed_code_accepted", "Was the displayed code accepted as the SoftAP password?")
        require_confirm(checks, "provisioning_portal_reachable", "Is the provisioning portal reachable at 192.168.4.1?")

        print("Do not submit the Wi-Fi form yet.")
        input("Prepare valid Wi-Fi credentials, then press Enter. Submit immediately when capture starts: ")
        print("Capturing for 90 seconds. Submit the form now...")
        post = capture_serial(port, 90, "none")
        combined += post
        checks["ip_obtained_marker"] = IP_MARKER in combined
        if not checks["ip_obtained_marker"]:
            raise MandatoryCheckFailed("ip_obtained_marker")
        checks["bootstrap_closed_marker"] = CLOSED_MARKER in combined
        if not checks["bootstrap_closed_marker"]:
            raise MandatoryCheckFailed("bootstrap_closed_marker")

        require_confirm(checks, "wifi_connected_display", "Does the panel clearly show Wi-Fi connected?")
        require_confirm(checks, "bootstrap_closed_display", "Does the panel clearly show Bootstrap closed?")
        require_confirm(checks, "softap_disappeared", "Did HomeyPanel-Setup disappear?")
        require_confirm(checks, "http_endpoint_closed", "Is the bootstrap HTTP page no longer reachable?")
        print("Prepare to hold the physical BOOT button continuously for at least five seconds.")
        input("Press Enter, then immediately hold BOOT until the panel restarts: ")
        reprovision_log = capture_serial(port, 15, "reprovision")
        combined += reprovision_log
        checks["physical_reprovision_marker"] = PHYSICAL_MARKER in reprovision_log
        if not checks["physical_reprovision_marker"]:
            raise MandatoryCheckFailed("physical_reprovision_marker")
        checks["physical_restart_marker"] = RESTART_MARKER in reprovision_log
        if not checks["physical_restart_marker"]:
            raise MandatoryCheckFailed("physical_restart_marker")
        print("Waiting for fresh bootstrap after physical reprovisioning...")
        if wait_for_port(port, False, 5):
            if not wait_for_port(port, True, 45):
                raise MandatoryCheckFailed("serial_port_return_after_reprovision")
        else:
            time.sleep(2)
        fresh_log = capture_serial(port, 45, "none")
        combined += fresh_log
        checks["fresh_bootstrap_markers"] = has_fresh_bootstrap_markers(
            reprovision_log, fresh_log
        )
        if not checks["fresh_bootstrap_markers"]:
            raise MandatoryCheckFailed("fresh_bootstrap_markers")
        require_confirm(checks, "fresh_bootstrap_visible", "Is fresh bootstrap visible on the panel after physical reprovisioning?")
    except MandatoryCheckFailed as error:
        output = write_evidence(
            sha256=sha256,
            flash_status=flash_status,
            boot_capture_method=boot_capture_method,
            checks=checks,
            serial_text=combined,
            overall="FAIL",
            failure=error.check_name,
        )
        fail(f"mandatory hardware check failed: {error.check_name}. Partial evidence: {output}")

    output = write_evidence(
        sha256=sha256,
        flash_status=flash_status,
        boot_capture_method=boot_capture_method,
        checks=checks,
        serial_text=combined,
        overall="PASS",
        failure=None,
    )
    print("UPLOAD ZIP:", output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
