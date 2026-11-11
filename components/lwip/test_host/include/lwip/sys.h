#pragma once

#include <pthread.h>
#include <stdint.h>

typedef unsigned sys_prot_t;
typedef pthread_t sys_thread_t;
extern pthread_mutex_t s_test_sys_protect_mutex;
uint32_t sys_now(void);
sys_thread_t xTaskGetCurrentTaskHandle(void);

#define SYS_ARCH_DECL_PROTECT(level) sys_prot_t level
#define SYS_ARCH_PROTECT(level) ((level) = 0U, pthread_mutex_lock(&s_test_sys_protect_mutex))
#define SYS_ARCH_UNPROTECT(level) do { (void)(level); pthread_mutex_unlock(&s_test_sys_protect_mutex); } while (0)
