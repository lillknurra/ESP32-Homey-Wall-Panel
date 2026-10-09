#!/usr/bin/env python3
"""Deterministic real Favorites entry + passive transport hooks + IDF parser."""
from pathlib import Path
import subprocess,tempfile,json
ROOT=Path(__file__).resolve().parents[3]
C=ROOT/'components/secure_bootstrap'
IDF=Path('/Users/petter/GitHub/esp-idf-v6.0.1')
def function(s,sig):
    start=s.rindex(sig);brace=s.index('{',start);depth=0
    for i in range(brace,len(s)):
        if s[i]=='{':depth+=1
        elif s[i]=='}':
            depth-=1
            if depth==0:return s[start:i+1]
    raise AssertionError(sig)
client=(C/'athom_cloud_client.c').read_text(); runtime=(C/'athom_oauth_runtime.c').read_text()
functions='\n'.join(function(runtime,sig) for sig in (
    'static bool athom_inventory_attempt_json_append(',
    'static bool athom_favorites_fetch_diagnostic_json_append(',
    'static bool athom_favorites_fin_recovery_json_append('))
functions+='\n'+function(client,'static bool favorites_pre_response_fin_replay_allowed(')
functions+='\n'+function(client,'static esp_err_t favorites_fetch_user_me(')
template=Path(__file__).with_name('test_patch071_favorites_fin_recovery.c').read_text()
with tempfile.TemporaryDirectory() as temp:
    p=Path(temp); source=p/'test.c';binary=p/'test'
    source.write_text(template.replace('/* PATCH071_PRODUCTION_FUNCTIONS */',functions))
    subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-pedantic','-pthread',
        '-DPATCH070_HOST_TEST','-I',str(C/'include'),'-I',str(C/'test_host'),
        '-I',str(IDF/'components/http_parser'),str(source),str(C/'athom_favorites_transport_diag.c'),
        str(IDF/'components/http_parser/http_parser.c'),'-o',str(binary)],check=True,cwd=ROOT)
    result=subprocess.run([str(binary)],capture_output=True,text=True,check=True)
    lines=[l for l in result.stdout.splitlines() if l.startswith('PATCH071_RECOVERY_JSON_SAMPLE=')]
    assert len(lines)==1
    # helper appends a member to an already-open JSON object.
    sample=json.loads('{'+lines[0].split('=',1)[1].lstrip(',')+'}')['fin_recovery']
    assert sample['attempted'] is True and sample['fresh_connection'] is True
    assert sample['logical_request_attempt_count']==2 and sample['result']=='ok'
    assert sample['original']['tls_query']==0x8008
    assert sample['original']['fetch']['class']=='header_fin_reported'
    print(result.stdout)
favorites=function(client,'static esp_err_t favorites_fetch_user_me(')
assert favorites.count('http_request_limited(')==2
assert all(word not in favorites for word in ('for (','while (','cloud_discover','homey_login','set_header(','http_put','HTTP_METHOD_POST','HTTP_METHOD_PUT'))
get=function(runtime,'static esp_err_t status_get(')
assert all(word not in get for word in ('favorites_fetch','http_request','queue_inventory','xQueueSend'))
# Exact IDF close->INIT and perform INIT->connect guarantee fresh transport.
idf=(IDF/'components/esp_http_client/esp_http_client.c').read_text()
close=function(idf,'esp_err_t esp_http_client_close(')
assert 'client->state = HTTP_STATE_INIT' in close and 'esp_transport_close' in close
helper=function(client,'static esp_err_t http_request_limited(')
assert 'esp_http_client_close(ctx->handle)' in helper
assert helper.index('esp_http_client_set_user_data(ctx->handle, NULL)')<helper.index('athom_favorites_transport_diag_close_result(close_err)')
print('PATCH071_READ_ONLY_SINGLE_REPLAY_AND_FRESH_CONNECTION_SOURCE_GATE PASS')
