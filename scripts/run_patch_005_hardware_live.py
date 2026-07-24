#!/usr/bin/env python3
from pathlib import Path
import glob, hashlib, json, os, re, signal, subprocess, sys, time, zipfile

REPO=Path.home()/"GitHub"/"ESP32-Homey-Wall-Panel"
IDF=Path.home()/"GitHub"/"esp-idf-v6.0.1"
FIRMWARE=REPO/"build"/"esp32_homey_wall_panel.bin"

def fail(message): print(f"FAIL: {message}",file=sys.stderr); raise SystemExit(1)
def choose_port():
    ports=sorted(set(glob.glob('/dev/cu.usbmodem*')+glob.glob('/dev/cu.usbserial*')))
    if not ports: fail('No serial port found')
    for i,p in enumerate(ports,1): print(f'{i}. {p}')
    value=input('Choose serial port number: ').strip()
    if not value.isdigit() or not 1<=int(value)<=len(ports): fail('Invalid serial port selection')
    return ports[int(value)-1]
def shell(command,capture=False):
    return subprocess.run(['/bin/bash','-lc',command],cwd=REPO,text=True,
        stdout=subprocess.PIPE if capture else None,stderr=subprocess.STDOUT if capture else None)
def capture_monitor(port,seconds):
    command=f'. "{IDF}/export.sh" >/dev/null && idf.py -p "{port}" monitor'
    process=subprocess.Popen(['/bin/bash','-lc',command],cwd=REPO,text=True,
        stdout=subprocess.PIPE,stderr=subprocess.STDOUT,start_new_session=True)
    lines=[]; deadline=time.monotonic()+seconds
    try:
        while time.monotonic()<deadline:
            line=process.stdout.readline() if process.stdout else ''
            if line: lines.append(line)
            elif process.poll() is not None: break
            else: time.sleep(0.05)
    finally:
        if process.poll() is None:
            os.killpg(process.pid,signal.SIGTERM)
            try: process.wait(timeout=3)
            except subprocess.TimeoutExpired: os.killpg(process.pid,signal.SIGKILL)
    return ''.join(lines)
def sanitize(text):
    patterns=[r'Bearer\s+\S+',r'Basic\s+\S+',r'https?://\S+',r'\b(?:\d{1,3}\.){3}\d{1,3}\b',r'\b[0-9A-Fa-f]{2}(?::[0-9A-Fa-f]{2}){5}\b',r'(?i)(access_token|refresh_token|client_secret|authorization_code)\s*[:=]\s*\S+']
    for pattern in patterns: text=re.sub(pattern,'[REDACTED]',text)
    return text

if not FIRMWARE.exists(): fail('Firmware missing; offline validation must pass first')
port=choose_port(); sha=hashlib.sha256(FIRMWARE.read_bytes()).hexdigest()
print(f'Firmware: {FIRMWARE}\nSHA-256: {sha}\nPort: {port}')
if input('Type FLASH to flash this exact firmware: ').strip()!='FLASH': fail('Flash not authorized')
result=shell(f'. "{IDF}/export.sh" && idf.py -p "{port}" flash',capture=True)
if result.returncode: print(result.stdout); fail('Flash failed')
stamp=time.strftime('%Y%m%d-%H%M%S'); evidence=Path.home()/"Downloads"/f"patch-005-hardware-{stamp}"
evidence.mkdir(parents=True); (evidence/'flash.log').write_text(sanitize(result.stdout or ''),encoding='utf-8')
boot=sanitize(capture_monitor(port,25)); (evidence/'boot-sanitized.log').write_text(boot,encoding='utf-8')
required=['Wi-Fi station connected; IP obtained','Provisioning portal started']
missing=[item for item in required if item not in boot]
if missing: fail('Boot evidence missing: '+', '.join(missing))
print('Smoke flash PASS. Real OAuth remains a separate explicit action.')
if input('Type OAUTH to authorize use of the local provisioning portal: ').strip()!='OAUTH':
    print('OAuth not authorized; smoke evidence will be packaged.')
else:
    print('Open the local device portal and POST confirm=I_UNDERSTAND to /live/authorize.')
    print('Enter client ID, hidden client secret and redirect URI only in the local portal.')
    print('Use the generated Athom authorization URL. The direct callback is processed immediately.')
    input('Press Enter after sanitized read-only inventory completes, or Ctrl-C to stop...')
manifest={'firmware_sha256':sha,'flash':'PASS','boot':'PASS','oauth_credentials_in_evidence':False,'commit':False,'push':False,'merge':False}
(evidence/'manifest.json').write_text(json.dumps(manifest,indent=2),encoding='utf-8')
out=Path(str(evidence)+'.zip')
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED) as archive:
    for p in evidence.rglob('*'):
        if p.is_file(): archive.write(p,arcname=f'{evidence.name}/{p.relative_to(evidence)}')
print('UPLOAD ZIP:',out)
