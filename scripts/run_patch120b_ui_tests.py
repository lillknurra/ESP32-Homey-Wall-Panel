#!/usr/bin/env python3
"""Compile and test the actual UI model confirmation path on host."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
COMP = ROOT / 'components/secure_bootstrap'
ui_source = (COMP / 'panel_ui.c').read_text()
provider_source = (COMP / 'secure_bootstrap_esp.c').read_text()
information_source = (COMP / 'panel_information.c').read_text()
assert 'Exakt version fastställs där och visas inte före bekräftelsen.' in ui_source
assert 'Exakt version visas först där.' not in ui_source
assert 'out->firmware_update_result = panel_ota_handoff_last_runtime_result();' in provider_source
for phrase in ('Ingen tidigare uppdatering', 'Wi-Fi-anslutningen misslyckades',
               'Tidsynkroniseringen misslyckades', 'Manifest/release avvisad',
               'Bild staged; omstart/bootbekräftelse väntar',
               'Ny app lokalt bootbekräftad', 'Rollback begärd; lyckad återgång ej verifierad'):
    assert phrase in information_source
with tempfile.TemporaryDirectory(prefix='patch120b-ui-') as temp:
    exe = Path(temp) / 'ui_test'
    subprocess.run(['cc', '-std=c11', '-Wall', '-Wextra', '-Werror', '-pedantic',
                    '-I', str(COMP / 'include'),
                    str(COMP / 'panel_ui_model.c'),
                    str(COMP / 'panel_roller_model.c'),
                    str(COMP / 'panel_homey_dashboard_binding.c'),
                    str(ROOT / 'tests/patch120b/test_ui_confirmation.c'), '-o', str(exe)],
                   check=True)
    subprocess.run([str(exe)], check=True)
