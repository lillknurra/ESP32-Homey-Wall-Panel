#!/usr/bin/env python3
"""Execute the original and generated restart decision against identical mocks."""
from pathlib import Path
import argparse, importlib.util, re, subprocess, tempfile
root = Path(__file__).resolve().parents[3]
p = argparse.ArgumentParser(); p.add_argument('--output-dir', type=Path); args = p.parse_args()
out = args.output_dir or Path(tempfile.mkdtemp(prefix='PATCH076_V32_TEST_', dir=Path.home()/'Downloads'))
out.mkdir(parents=True, exist_ok=True)
spec = importlib.util.spec_from_file_location('instrument', root/'scripts/instrument_rgb_v32.py')
m = importlib.util.module_from_spec(spec); spec.loader.exec_module(m)
raw = Path('/Users/petter/GitHub/esp-idf-v6.0.1/components/esp_lcd/rgb/esp_lcd_panel_rgb.c').read_bytes()
generated = m.instrument(raw)
assert re.sub(r'\n/\* V32_BEGIN \*/\n.*?\n/\* V32_END \*/\n', '', generated, flags=re.S).encode() == raw
try: m.instrument(raw+b' ')
except ValueError: pass
else: raise AssertionError('unmatched source must fail closed')

def function(text):
    start = text.index('static IRAM_ATTR void lcd_rgb_panel_try_restart_transmission(')
    brace = text.index('{', start); depth = 1; end = brace+1
    while depth:
        depth += (text[end]=='{') - (text[end]=='}'); end += 1
    return text[start:end]

stub = r'''#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "panel_rgb_diag_model.h"
#define IRAM_ATTR
#define RGB_LCD_NEEDS_SEPARATE_RESTART_LINK 0
#define portENTER_CRITICAL_ISR(x) ((void)(x))
#define portEXIT_CRITICAL_ISR(x) ((void)(x))
typedef struct { int bb_size, fb_bits_per_pixel, spinlock, bounce_pos_px;
 size_t bb_eof_count, expect_eof_count; struct { bool need_restart; } flags;
 unsigned char *bounce_buffer[2]; int dma_chan, dma_bb_link, dma_fb_links[3], cur_fb_index;
 struct { int dev; } hal; } esp_rgb_panel_t;
static unsigned reset_calls, start_calls, fifo_calls, fill_calls;
static panel_diag_state_t diag;
static void lcd_ll_fifo_reset(int a) {(void)a; fifo_calls++;}
static void gdma_reset(int a) {(void)a; reset_calls++;}
static void gdma_start(int a, int b) {(void)a; (void)b; start_calls++;}
static int gdma_link_get_head_addr(int a) {return a;}
static void lcd_rgb_panel_fill_bounce_buffer(esp_rgb_panel_t *p, unsigned char *b)
 {(void)b; fill_calls++; p->bounce_pos_px += p->bb_size/(p->fb_bits_per_pixel/8);}
static void panel_render_diag_decision(uint32_t actual, uint32_t expected, bool req, bool policy, bool restart)
 {panel_diag_decision(&diag, 100, actual, expected, req, policy, restart);}
static void panel_render_diag_restarted(void) {panel_diag_restarted(&diag, 101);}
'''
main = r'''
int main(void) {
 for (unsigned actual=0;actual<=26;actual++) for(unsigned request=0;request<2;request++)
 for(unsigned pos=0;pos<=400;pos+=100) {
  esp_rgb_panel_t a={.bb_size=200,.fb_bits_per_pixel=16,.bounce_pos_px=(int)pos,
                    .bb_eof_count=actual,.expect_eof_count=24,.flags={request}};
  esp_rgb_panel_t b=a; memset(&diag,0,sizeof(diag)); diag.metrics.vsync=3;
  reset_calls=start_calls=fifo_calls=fill_calls=0; original(&a);
  unsigned resets=reset_calls, starts=start_calls, fifos=fifo_calls, fills=fill_calls;
  reset_calls=start_calls=fifo_calls=fill_calls=0; instrumented(&b);
  assert(memcmp(&a,&b,sizeof(a))==0);
  assert(resets==reset_calls && starts==start_calls && fifos==fifo_calls && fills==fill_calls);
  bool restart=CONFIG_LCD_RGB_RESTART_IN_VSYNC || request || actual<24;
  assert(diag.metrics.restart_decisions==(unsigned)restart);
  assert(diag.metrics.restart_paths==(unsigned)restart);
  assert(diag.metrics.shortfall==(unsigned)(!CONFIG_LCD_RGB_RESTART_IN_VSYNC && actual<24));
  assert(diag.metrics.startup_shortfall==0);
 }
 panel_diag_state_t s={0}; panel_diag_vsync(&s,100);panel_diag_vsync(&s,16675);
 panel_diag_vsync(&s,33250);assert(s.metrics.vsync_min_us==16575 && s.metrics.vsync_max_us==16575);
 panel_diag_decision(&s,33250,20,24,true,false,true);panel_diag_restarted(&s,33251);
 assert(s.metrics.shortfall==1 && s.metrics.restart_decisions==1 && s.metrics.restart_paths==1);
 assert(s.metrics.last_reason==(PANEL_DIAG_REQUEST|PANEL_DIAG_SHORTFALL));
 panel_diag_state_t early={0};panel_diag_vsync(&early,1);
 panel_diag_decision(&early,1,0,24,false,false,true);assert(early.metrics.startup_shortfall==1);
 for(unsigned i=0;i<PANEL_DIAG_EVENTS+3;i++)panel_diag_event(&early,i,1,i,24);
 assert(early.count==PANEL_DIAG_EVENTS && early.metrics.dropped_events==4);
 panel_diag_event_t e;assert(panel_diag_pop(&early,&e) && e.us==1);
 unsigned n=1;while(panel_diag_pop(&early,&e))n++;assert(n==PANEL_DIAG_EVENTS);
 puts("V32_ACCOUNTING_AND_REAL_DRIVER_DECISION_EQUIVALENCE PASS");return 0;
}
'''
source=stub+function(raw.decode()).replace('lcd_rgb_panel_try_restart_transmission','original',1)+'\n'+function(generated).replace('lcd_rgb_panel_try_restart_transmission','instrumented',1)+main
(out/'driver_equivalence.c').write_text(source)
for policy in (0,1):
    exe=out/f'driver_policy_{policy}'
    subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror',f'-DCONFIG_LCD_RGB_RESTART_IN_VSYNC={policy}',
        '-I',str(root/'components/panel_render_diag/include'),str(out/'driver_equivalence.c'),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('V32_SOURCE_HASH_GUARD_INSERTION_REVERSIBILITY PASS')

# Execute the actual allocator wrappers with success, failure and zero-size cases.
alloc_test = r"""#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
static char storage;
static unsigned calls, failures;
static size_t last_bytes;
void *__real_lv_malloc_core(size_t n) {calls++; return n==42 ? NULL : &storage;}
void *__real_lv_realloc_core(void *p, size_t n) {(void)p; calls++; return n==42 || n==0 ? NULL : &storage;}
void panel_render_diag_alloc_failure(size_t n,uint32_t caps) {assert(caps==0); failures++;last_bytes=n;}
void *__wrap_lv_malloc_core(size_t n);
void *__wrap_lv_realloc_core(void *p,size_t n);
int main(void) {
 assert(__wrap_lv_malloc_core(8)==&storage && calls==1 && failures==0);
 assert(__wrap_lv_malloc_core(42)==NULL && calls==2 && failures==1 && last_bytes==42);
 assert(__wrap_lv_realloc_core(&storage,8)==&storage && calls==3 && failures==1);
 assert(__wrap_lv_realloc_core(&storage,42)==NULL && calls==4 && failures==2);
 assert(__wrap_lv_realloc_core(&storage,0)==NULL && calls==5 && failures==2);
 puts("V32_REAL_LVGL_ALLOC_WRAPPER_PRESERVATION PASS");return 0;
}
"""
(out/'alloc_test.c').write_text(alloc_test)
subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror','-I',str(root/'components/panel_render_diag/include'),
 str(root/'components/panel_render_diag/panel_lvgl_alloc_diag.c'),str(out/'alloc_test.c'),'-o',str(out/'alloc_test')],check=True)
subprocess.run([str(out/'alloc_test')],check=True)

# Execute the production ISR hooks and the real bounded reporter using API stubs.
stubs=out/'stubs';(stubs/'freertos').mkdir(parents=True,exist_ok=True)
(stubs/'esp_attr.h').write_text('#pragma once\n#define IRAM_ATTR\n#define DRAM_ATTR\n')
(stubs/'esp_timer.h').write_text('#pragma once\n#include <stdint.h>\nextern uint64_t test_time;\nstatic inline int64_t esp_timer_get_time(void) {return (int64_t)test_time;}\n')
(stubs/'esp_heap_caps.h').write_text('#pragma once\n#include <stddef.h>\n#define MALLOC_CAP_INTERNAL 1\n#define MALLOC_CAP_8BIT 2\n#define MALLOC_CAP_SPIRAM 4\nstatic inline size_t heap_caps_get_free_size(unsigned c) {(void)c;return 100000;}\nstatic inline size_t heap_caps_get_minimum_free_size(unsigned c) {(void)c;return 90000;}\n')
(stubs/'esp_log.h').write_text('#pragma once\nvoid test_log(const char *,const char *,...);\n#define ESP_LOGI(...) test_log(__VA_ARGS__)\n#define ESP_LOGE(...) test_log(__VA_ARGS__)\n')
(stubs/'freertos/FreeRTOS.h').write_text('#pragma once\n#include <stdint.h>\ntypedef uint8_t StackType_t;typedef struct {char data[300];} StaticTask_t;typedef int portMUX_TYPE;\n#define portMUX_INITIALIZER_UNLOCKED 0\n#define pdMS_TO_TICKS(n) (n)\n#define portENTER_CRITICAL(x) ((void)(x))\n#define portEXIT_CRITICAL(x) ((void)(x))\n#define portENTER_CRITICAL_ISR(x) ((void)(x))\n#define portEXIT_CRITICAL_ISR(x) ((void)(x))\n#define portENTER_CRITICAL_SAFE(x) ((void)(x))\n#define portEXIT_CRITICAL_SAFE(x) ((void)(x))\n')
(stubs/'freertos/task.h').write_text('#pragma once\n#include "FreeRTOS.h"\n#include <stddef.h>\nextern void (*test_task)(void *);extern unsigned test_stack_bytes,test_priority,test_suspended;extern uint64_t test_time;\nstatic inline void *xTaskCreateStatic(void (*f)(void *),const char *n,unsigned d,void *a,unsigned p,StackType_t *s,StaticTask_t *t) {(void)n;(void)a;(void)s;test_task=f;test_stack_bytes=d;test_priority=p;return t;}\nstatic inline void vTaskDelay(unsigned ms) {test_time+=(uint64_t)ms*1000;}\nstatic inline void vTaskSuspend(void *p) {(void)p;test_suspended++;}\n')
hooks = r"""#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdint.h>
uint64_t test_time=100;
void (*test_task)(void *);
unsigned test_stack_bytes,test_priority,test_suspended,test_lines;
uint64_t last_line;
void test_log(const char *tag,const char *fmt,...) {
 (void)tag;(void)fmt; assert(test_time>=last_line+1000000);last_line=test_time;test_lines++;
}
""" + '#include "'+str(root/'components/panel_render_diag/panel_render_diag.c')+'"\n'+ r"""
int main(void) {
 for(unsigned i=1;i<=128;i++) assert(panel_render_diag_eof()==(i%64==0));
 assert(s_state.metrics.eof==128);
 panel_render_diag_bounce_sample(12);panel_render_diag_bounce_sample(4);
 assert(s_state.metrics.bounce_samples==2 && s_state.metrics.bounce_max_us==12);
 panel_render_diag_vsync();test_time+=16575;panel_render_diag_vsync();
 panel_render_diag_decision(20,24,true,false,true);panel_render_diag_restarted();
 assert(s_state.metrics.shortfall==1 && s_state.metrics.restart_paths==1);
 panel_render_diag_restart_requested();assert(s_state.metrics.request_calls==1);
 panel_render_diag_flash_interval(50,123,0);assert(s_state.metrics.flash_count==1 && s_state.metrics.flash_max_us==123);
 panel_render_diag_alloc_failure(42,0);assert(s_state.metrics.alloc_failures==1);
 panel_render_diag_start();panel_render_diag_start();
 assert(test_task && test_stack_bytes==4096 && test_priority==0);
 test_task(NULL);assert(test_lines==900 && test_suspended==1);
 assert(s_state.count==0);
 puts("V32_PRODUCTION_HOOKS_SAMPLING_FLASH_MEMORY_AND_900_REPORT_BOUND PASS");return 0;
}
"""
(out/'hooks_test.c').write_text(hooks)
subprocess.run(['cc','-std=c11','-O2','-Wall','-Wextra','-Werror','-I',str(stubs),'-I',str(root/'components/panel_render_diag/include'),
 str(out/'hooks_test.c'),'-o',str(out/'hooks_test')],check=True)
subprocess.run([str(out/'hooks_test')],check=True)
