#include "panel_homey_light_provisioning.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

static void test_access_policy(void)
{
    assert(!panel_homey_light_provisioning_access_allowed(false, false, false));
    assert(!panel_homey_light_provisioning_access_allowed(false, true, true));
    assert(!panel_homey_light_provisioning_access_allowed(true, false, true));
    assert(!panel_homey_light_provisioning_access_allowed(true, true, false));
    assert(panel_homey_light_provisioning_access_allowed(true, true, true));
    puts("ACCESS_POLICY_TESTS PASS");
}

int main(void){test_access_policy();panel_homey_alias_record_t r;assert(panel_homey_light_provisioning_build("synthetic-light-a","synthetic-on","synthetic-light-b","synthetic-on",&r)==PANEL_HOMEY_ALIAS_STORE_OK);assert(r.entry_count==2U);assert(r.entries[0].dashboard_binding_index==4U);assert(r.entries[1].dashboard_binding_index==5U);assert(strcmp(r.entries[0].raw_device_id,"synthetic-light-a")==0);assert(strcmp(r.entries[1].raw_device_id,"synthetic-light-b")==0);assert(panel_homey_light_provisioning_build("same","on","same","on",&r)==PANEL_HOMEY_ALIAS_STORE_INVALID);assert(panel_homey_light_provisioning_build("","on","b","on",&r)==PANEL_HOMEY_ALIAS_STORE_INVALID);assert(panel_homey_light_provisioning_build("a","on","b","on",NULL)==PANEL_HOMEY_ALIAS_STORE_INVALID);puts("PANEL_HOMEY_LIGHT_PROVISIONING_TESTS PASS");return 0;}
