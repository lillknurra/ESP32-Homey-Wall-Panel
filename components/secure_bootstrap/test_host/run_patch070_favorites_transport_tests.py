#!/usr/bin/env python3
"""Exercise actual passive wrappers and bundled parser without network or device."""
from pathlib import Path
import json
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[3]
C=ROOT/'components/secure_bootstrap'
IDF=Path('/Users/petter/GitHub/esp-idf-v6.0.1')

def function(source, signature):
    start=source.rindex(signature)
    brace=source.index('{',start)
    depth=0
    for i in range(brace,len(source)):
        if source[i]=='{': depth+=1
        elif source[i]=='}':
            depth-=1
            if depth==0: return source[start:i+1]
    raise AssertionError(signature)

runtime=(C/'athom_oauth_runtime.c').read_text()
client=(C/'athom_cloud_client.c').read_text()
helpers='\n\n'.join(function(runtime,s) for s in (
    'static bool athom_inventory_attempt_json_append(',
    'static bool athom_favorites_fetch_diagnostic_json_append('))
template=Path(__file__).with_name('test_patch070_favorites_transport.c').read_text()
with tempfile.TemporaryDirectory() as directory:
    p=Path(directory);source=p/'test.c';binary=p/'test'
    source.write_text(template.replace('/* PATCH070_PRODUCTION_JSON_HELPERS */',helpers).replace(
        '/* PATCH070_PRODUCTION_FAVORITES_REQUEST */',
        function(client,'static bool favorites_pre_response_fin_replay_allowed(')+'\n'+
        function(client,'static esp_err_t favorites_fetch_user_me(')))
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-pedantic','-pthread',
        '-DPATCH070_HOST_TEST','-I',str(C/'include'),'-I',str(C/'test_host'),
        '-I',str(IDF/'components/http_parser'),str(source),
        str(C/'athom_favorites_transport_diag.c'),
        str(IDF/'components/http_parser/http_parser.c'),'-o',str(binary)],check=True,cwd=ROOT)
    result=subprocess.run([str(binary)],check=True,text=True,capture_output=True)
    lines=[l for l in result.stdout.splitlines() if l.startswith('PATCH070_JSON_SAMPLE=')]
    assert len(lines)==1
    sample=json.loads(lines[0].split('=',1)[1])
    assert sample['last_header_read_result']==-0x7100
    assert sample['class']=='header_zero_bytes_error' and sample['first_connection_reused'] is True
    assert all(type(v) in (bool,int,str) or v is None for v in sample.values())
    print(result.stdout)
client=(C/'athom_cloud_client.c').read_text()
favorites=function(client,'static esp_err_t favorites_fetch_user_me(')
assert favorites.count('http_request_limited(')==2
assert favorites.count('athom_favorites_transport_diag_begin(')==2
assert favorites.count('athom_favorites_transport_diag_finish(')==2
get=function(runtime,'static esp_err_t status_get(')
for forbidden in ('http_request','fetch_inventory(','queue_inventory','xQueueSend',
                  'favorites_fetch','http_client_perform','transport_diag_begin'):
    assert forbidden not in get,forbidden
observer=(C/'athom_favorites_transport_diag.c').read_text()
for forbidden in ('esp_http_client_perform(', 'esp_http_client_open(', 'get_and_clear',
                  'printf(', 'malloc(', 'memcpy(data', 'strcpy('):
    assert forbidden not in observer,forbidden
print('PATCH070_PASSIVE_DIAGNOSTICS_WITH_PATCH071_BOUNDED_REPLAY_PRIVACY_SOURCE_GATE PASS')
