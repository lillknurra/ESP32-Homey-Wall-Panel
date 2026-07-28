#!/usr/bin/env python3
"""Generate a local NVS image containing the Athom OAuth client configuration.
This tool never prints credential values and never flashes automatically.
"""
from pathlib import Path
import argparse, csv, json, os, subprocess, tempfile, zlib, struct
REDIRECT="http://homey-panel.local/oauth/callback"
def main():
 p=argparse.ArgumentParser();p.add_argument('--input',default='local/athom-oauth.json');p.add_argument('--output',default='local/athom-client-config.nvs.bin');p.add_argument('--size',default='0x6000');a=p.parse_args()
 src=Path(a.input);dst=Path(a.output)
 data=json.loads(src.read_text(encoding='utf-8'))
 cid=data.get('client_id','');secret=data.get('client_secret','');redirect=data.get('redirect_uri','')
 if not isinstance(cid,str) or not cid or len(cid)>=128: raise SystemExit('Invalid client_id')
 if not isinstance(secret,str) or not secret or len(secret)>=192: raise SystemExit('Invalid client_secret')
 if redirect!=REDIRECT: raise SystemExit('redirect_uri must exactly match '+REDIRECT)
 # Firmware stores one CRC-protected packed blob. Generate it without displaying values.
 fmt='<IHHI128s192s128s';magic=0x414f4346;version=1;size=struct.calcsize(fmt)
 def enc(v,n): b=v.encode();return b+b'\0'*(n-len(b))
 raw=struct.pack(fmt,magic,version,size,0,enc(cid,128),enc(secret,192),enc(redirect,128));crc=zlib.crc32(raw)&0xffffffff
 raw=struct.pack(fmt,magic,version,size,crc,enc(cid,128),enc(secret,192),enc(redirect,128))
 idf=Path(os.environ.get('IDF_PATH',''))
 gen=idf/'components/nvs_flash/nvs_partition_generator/nvs_partition_gen.py'
 if not gen.is_file(): raise SystemExit('IDF_PATH does not point to ESP-IDF with nvs_partition_gen.py')
 dst.parent.mkdir(parents=True,exist_ok=True)
 with tempfile.TemporaryDirectory() as td:
  csvp=Path(td)/'athom.csv';blob=Path(td)/'client.bin';blob.write_bytes(raw)
  with csvp.open('w',newline='',encoding='utf-8') as f:
   w=csv.writer(f);w.writerow(['key','type','encoding','value']);w.writerow(['athom_cli_cfg','namespace','','']);w.writerow(['client_v1','file','binary',str(blob)])
  subprocess.run(['python3',str(gen),'generate',str(csvp),str(dst),a.size],check=True,stdout=subprocess.DEVNULL)
 os.chmod(dst,0o600)
 print('PASS: local Athom OAuth NVS image generated (credential values not displayed)')
 print(dst)
if __name__=='__main__': main()
