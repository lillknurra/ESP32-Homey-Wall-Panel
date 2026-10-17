#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "panel_information.h"
static char text[640];
static void format(panel_information_kind_t k, panel_information_t *i)
{ assert(panel_information_format(k,i,text,sizeof(text))); }
int main(void)
{
    panel_information_t i={0};
    for(unsigned k=0;k<PANEL_INFORMATION_COUNT;k++){format((panel_information_kind_t)k,&i);assert(strstr(text,"Ej tillgänglig"));}
    assert(!panel_information_format(PANEL_INFORMATION_COUNT,&i,text,sizeof(text)));
    assert(!panel_information_format(PANEL_INFORMATION_WIFI,NULL,text,sizeof(text)));
    assert(!panel_information_format(PANEL_INFORMATION_WIFI,&i,text,2)); assert(text[0]==0);
    i.wifi_known=i.provisioning_known=true;
    format(PANEL_INFORMATION_WIFI,&i);assert(strstr(text,"Frånkopplad")&&strstr(text,"Stängd"));
    i.wifi_online=i.provisioning_active=true;
    format(PANEL_INFORMATION_WIFI,&i);assert(strstr(text,"Ansluten")&&strstr(text,"Aktiv"));
    i.homey_known=true;i.homey_state=2;i.now_ms=1000;
    panel_information_observe_snapshot(&i.snapshot,PANEL_HOMEY_READ_OK,1000,7);
    format(PANEL_INFORMATION_HOMEY,&i);assert(strstr(text,"READY")&&strstr(text,"Färsk"));
    i.now_ms=121001;format(PANEL_INFORMATION_HOMEY,&i);assert(strstr(text,"Inaktuell"));
    i.now_ms=999;format(PANEL_INFORMATION_HOMEY,&i);assert(strstr(text,"Ej tillgänglig"));
    panel_information_observe_snapshot(&i.snapshot,PANEL_HOMEY_READ_STALE,0,0);
    format(PANEL_INFORMATION_HOMEY,&i);assert(strstr(text,"Inaktuell"));
    panel_information_observe_snapshot(&i.snapshot,PANEL_HOMEY_READ_NOT_FOUND,0,0);
    format(PANEL_INFORMATION_HOMEY,&i);assert(strstr(text,"Saknas"));
    panel_information_observe_snapshot(&i.snapshot,PANEL_HOMEY_READ_INVALID,0,0);
    format(PANEL_INFORMATION_HOMEY,&i);assert(strstr(text,"Ej tillgänglig"));
    i.homey_state=99;format(PANEL_INFORMATION_HOMEY,&i);assert(!strstr(text,"READY"));
    i.firmware_known=true;memset(i.version,'X',sizeof(i.version));strcpy(i.idf_version,"v6.0.1");
    strcpy(i.build_date,"Oct 17 2026");strcpy(i.build_time,"11:12:13");i.version[1]='\n';
    format(PANEL_INFORMATION_FIRMWARE,&i);assert(strstr(text,"X?X")&&strstr(text,"v6.0.1"));
    i.time_known=true;i.swedish_timezone=true;
    format(PANEL_INFORMATION_TIME,&i);assert(strstr(text,"Ej synkroniserad")&&strstr(text,"CET/CEST"));
    i.time_synchronized=true;format(PANEL_INFORMATION_TIME,&i);assert(strstr(text,"Synkroniserad"));
    i.swedish_timezone=false;format(PANEL_INFORMATION_TIME,&i);assert(strstr(text,"Tidszon: Ej tillgänglig"));
    i.memory_known=true;i.internal_free=12345;i.internal_minimum=10;i.psram_free=6000000;i.now_ms=123000;
    format(PANEL_INFORMATION_SYSTEM,&i);assert(strstr(text,"123 s")&&strstr(text,"12345 B")&&strstr(text,"LVGL-marginal: Ej tillgänglig"));
    puts("PATCH087_FORMAT_STATUS_MISSING_TTL_METADATA_BOUNDS PASS");
}
