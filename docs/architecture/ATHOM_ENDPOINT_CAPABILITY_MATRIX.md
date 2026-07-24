# Athom endpoint and OAuth capability matrix

| Capability | Status | Evidence / boundary |
|---|---|---|
| Authorization endpoint | VERIFIED | `GET https://api.athom.com/oauth2/authorise` |
| Authorization parameters | VERIFIED | `authorization_type=code`, `client_id`, `redirect_uri`, optional `state` |
| Authorization-code lifetime | VERIFIED | 30 seconds |
| Token endpoint | VERIFIED | `POST https://api.athom.com/oauth2/token` |
| Token authentication | VERIFIED | HTTP Basic: client ID and client secret |
| Authorization-code form | VERIFIED | `grant_type=authorization_code`, `authorization_code` |
| Refresh-token form | VERIFIED | `grant_type=refresh_token`, `refresh_token` |
| Access-token expiry | VERIFIED | approximately one hour |
| Refresh-token rotation | VERIFIED | entire returned token must be saved |
| Authenticated user/Homeys | VERIFIED | `GET https://api.athom.com/user/me` |
| Delegation token | VERIFIED | `POST /delegation/token?audience=homey` |
| Homey session | VERIFIED | `POST <remoteUrl>/api/manager/users/login` with delegation token |
| Remote URL freshness | VERIFIED | `/user/me` must be repeated; URLs may change or be null |
| System info | VERIFIED | `GET /api/manager/system/`, `homey.system.readonly` |
| Zones | VERIFIED | `GET /api/manager/zones/zone`, `homey.zone.readonly` |
| Devices | VERIFIED | `GET /api/manager/devices/device`, `homey.device.readonly` |
| Flows | VERIFIED | `GET /api/manager/flow/flow`, `homey.flow.readonly` |
| Advanced Flows | VERIFIED | `GET /api/manager/flow/advancedflow`, `homey.flow.readonly` |
| Moods | VERIFIED | `GET /api/manager/moods/mood`, `homey.mood.readonly` |
| TLS certificate validation | VERIFIED REQUIREMENT | HTTPS remote URLs; ESP-IDF certificate bundle required |
| PKCE | NOT DOCUMENTED | Not implemented |
| Device authorization grant | NOT DOCUMENTED | Not implemented |
| Public/native client without secret | NOT DOCUMENTED | Not assumed |
| Embedded client-secret confidentiality | BLOCKED AS STRONG CLAIM | Physical extraction remains possible |
| CLOUD vs REMOTE_FORWARDED choice | REQUIRES LIVE PROTOCOL VALIDATION | Use fresh `remoteUrl`; do not synthesize URLs |
| Live payload compatibility | REQUIRES LIVE PROTOCOL VALIDATION | Validate with sanitized evidence |
