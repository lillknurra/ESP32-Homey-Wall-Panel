#include "panel_wifi_transaction.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char log[64];
    size_t used;
    int fail_at;
    int calls;
    int executor_calls;
    int fail_executor_call;
    int marker_write_calls;
    bool executor;
    bool backup_present;
    bool candidate_persisted;
    bool restored;
    bool first_install_cleared;
    bool marker_present;
    bool persist_then_fail;
    bool restore_then_fail;
    panel_wifi_txn_marker_t marker;
} fake_t;

enum { OP_BACKUP=1, OP_MARKER_WRITE, OP_MARKER_CLEAR, OP_BACKUP_CLEAR,
       OP_EXECUTOR, OP_CANDIDATE, OP_RESTORE, OP_FIRST_CLEAR };

static int hit(fake_t *f, char tag, int op)
{
    assert(f->used + 1U < sizeof(f->log));
    f->log[f->used++] = tag;
    f->log[f->used] = '\0';
    f->calls++;
    return f->fail_at == op ? -1 : 0;
}

static int backup(void *ctx, bool had)
{
    fake_t *f = ctx;
    if (hit(f, 'B', OP_BACKUP) != 0) return -1;
    f->backup_present = had;
    return 0;
}
static int marker_write(void *ctx, const panel_wifi_txn_marker_t *m)
{
    fake_t *f = ctx;
    f->marker_write_calls++;
    if (hit(f, 'M', OP_MARKER_WRITE) != 0) return -1;
    f->marker = *m;
    f->marker_present = true;
    if (f->fail_at == 100 + f->marker_write_calls) return -1;
    return 0;
}
static int marker_clear(void *ctx, uint32_t generation)
{
    fake_t *f = ctx;
    assert(generation == f->marker.generation);
    if (hit(f, 'C', OP_MARKER_CLEAR) != 0) return -1;
    f->marker_present = false;
    return 0;
}
static int backup_clear(void *ctx)
{
    fake_t *f = ctx;
    if (hit(f, 'D', OP_BACKUP_CLEAR) != 0) return -1;
    f->backup_present = false;
    return 0;
}
static bool executor(void *ctx, uint32_t generation)
{
    fake_t *f = ctx;
    (void)generation;
    f->executor_calls++;
    (void)hit(f, 'E', OP_EXECUTOR);
    if (f->fail_executor_call == f->executor_calls) return false;
    return f->executor;
}
static int candidate(void *ctx, uint32_t generation)
{
    fake_t *f = ctx;
    (void)generation;
    if (hit(f, 'W', OP_CANDIDATE) != 0) return -1;
    assert(f->marker_present);
    f->candidate_persisted = true;
    if (f->persist_then_fail) return -1;
    return 0;
}
static int restore(void *ctx, uint32_t generation)
{
    fake_t *f = ctx;
    (void)generation;
    if (hit(f, 'R', OP_RESTORE) != 0) return -1;
    assert(f->backup_present);
    f->restored = true;
    if (f->restore_then_fail) return -1;
    return 0;
}
static int clear_first(void *ctx, uint32_t generation)
{
    fake_t *f = ctx;
    (void)generation;
    if (hit(f, 'Z', OP_FIRST_CLEAR) != 0) return -1;
    f->first_install_cleared = true;
    return 0;
}
static panel_wifi_txn_ops_t ops_for(fake_t *f)
{
    return (panel_wifi_txn_ops_t){
        .context=f, .backup_prepare_verified=backup,
        .marker_write_verified=marker_write, .marker_clear_verified=marker_clear,
        .backup_clear=backup_clear, .executor_available=executor,
        .candidate_persist_verified=candidate,
        .restore_previous_verified=restore,
        .clear_first_install_verified=clear_first,
    };
}
static void marker_codec(void)
{
    panel_wifi_txn_marker_t in = {PANEL_WIFI_TXN_CANDIDATE_VERIFY_PENDING, true, 11U};
    panel_wifi_txn_marker_t out = {0};
    uint8_t bytes[PANEL_WIFI_TXN_MARKER_SIZE];
    assert(panel_wifi_txn_marker_encode(&in, bytes));
    assert(panel_wifi_txn_marker_decode(bytes, sizeof(bytes), &out));
    assert(out.state == in.state && out.previous_config_present && out.generation == 11U);
    bytes[5] ^= 1U;
    assert(!panel_wifi_txn_marker_decode(bytes, sizeof(bytes), &out));
    assert(!panel_wifi_txn_marker_decode(bytes, sizeof(bytes)-1U, &out));
}
static void candidate_order_and_failures(void)
{
    fake_t f = {.executor=true};
    panel_wifi_txn_ops_t ops = ops_for(&f);
    assert(panel_wifi_txn_begin_candidate(&ops, true, 7U) == PANEL_WIFI_TXN_STEP_OK);
    assert(strncmp(f.log, "EBMEW", 5U) == 0);
    assert(f.marker_present && f.marker.state == PANEL_WIFI_TXN_CANDIDATE_VERIFY_PENDING);
    assert(f.candidate_persisted);
    assert(panel_wifi_txn_finish_candidate(&ops, 7U) == PANEL_WIFI_TXN_STEP_OK);
    assert(!f.marker_present && !f.backup_present);
    assert(strstr(f.log, "CW D") == NULL);

    memset(&f, 0, sizeof(f)); f.executor=false; ops=ops_for(&f);
    assert(panel_wifi_txn_begin_candidate(&ops, true, 8U) == PANEL_WIFI_TXN_STEP_EXECUTOR_UNAVAILABLE);
    assert(!f.candidate_persisted && !f.marker_present && strcmp(f.log, "E") == 0);

    memset(&f, 0, sizeof(f)); f.executor=true; f.fail_at=OP_BACKUP; ops=ops_for(&f);
    assert(panel_wifi_txn_begin_candidate(&ops, true, 9U) == PANEL_WIFI_TXN_STEP_BACKUP_FAILED);
    assert(!f.candidate_persisted && !f.marker_present);

    memset(&f, 0, sizeof(f)); f.executor=true; f.fail_at=OP_MARKER_WRITE; ops=ops_for(&f);
    assert(panel_wifi_txn_begin_candidate(&ops, true, 10U) == PANEL_WIFI_TXN_STEP_MARKER_FAILED);
    assert(!f.candidate_persisted);

    memset(&f, 0, sizeof(f)); f.executor=true; f.fail_at=OP_CANDIDATE; ops=ops_for(&f);
    assert(panel_wifi_txn_begin_candidate(&ops, true, 12U) == PANEL_WIFI_TXN_STEP_CANDIDATE_FAILED);
    assert(f.marker_present && !f.candidate_persisted);

    memset(&f, 0, sizeof(f)); f.executor=true; f.fail_executor_call=2; ops=ops_for(&f);
    assert(panel_wifi_txn_begin_candidate(&ops, true, 13U) == PANEL_WIFI_TXN_STEP_EXECUTOR_UNAVAILABLE);
    assert(f.marker_present && !f.candidate_persisted);

    memset(&f, 0, sizeof(f)); f.executor=true; f.persist_then_fail=true; ops=ops_for(&f);
    assert(panel_wifi_txn_begin_candidate(&ops, true, 14U) == PANEL_WIFI_TXN_STEP_CANDIDATE_FAILED);
    assert(f.marker_present && f.candidate_persisted);

    memset(&f, 0, sizeof(f)); f.executor=true; f.fail_at=102; ops=ops_for(&f);
    assert(panel_wifi_txn_begin_candidate(&ops, true, 15U) == PANEL_WIFI_TXN_STEP_MARKER_FAILED);
    assert(f.candidate_persisted && f.marker_present &&
        panel_wifi_txn_marker_is_unresolved(&f.marker));
}
static void recovery_terminal_order(void)
{
    fake_t f = {.executor=true, .backup_present=true,
        .marker={PANEL_WIFI_TXN_RECOVERY_REQUIRED,true,21U}, .marker_present=true};
    panel_wifi_txn_ops_t ops=ops_for(&f);
    assert(panel_wifi_txn_restore_previous(&ops, &f.marker) == PANEL_WIFI_TXN_STEP_OK);
    assert(f.restored && !f.marker_present && !f.backup_present);
    const char *clear = strchr(f.log, 'C');
    const char *rest = strchr(f.log, 'R');
    assert(rest != NULL && clear != NULL && rest < clear);

    memset(&f, 0, sizeof(f)); f.executor=true;
    f.marker=(panel_wifi_txn_marker_t){PANEL_WIFI_TXN_RECOVERY_REQUIRED,false,22U};
    f.marker_present=true; ops=ops_for(&f);
    assert(panel_wifi_txn_clear_first_install(&ops,&f.marker,true) == PANEL_WIFI_TXN_STEP_OK);
    assert(f.first_install_cleared && !f.marker_present);
    assert(panel_wifi_txn_clear_first_install(&ops,&f.marker,false) == PANEL_WIFI_TXN_STEP_INVALID);
    f.marker.previous_config_present=true;
    assert(panel_wifi_txn_clear_first_install(&ops,&f.marker,true) == PANEL_WIFI_TXN_STEP_INVALID);

    memset(&f, 0, sizeof(f)); f.marker=(panel_wifi_txn_marker_t){PANEL_WIFI_TXN_RECOVERY_REQUIRED,true,23U};
    f.marker_present=true; f.backup_present=true; f.fail_at=OP_MARKER_CLEAR; ops=ops_for(&f);
    assert(panel_wifi_txn_restore_previous(&ops,&f.marker) == PANEL_WIFI_TXN_STEP_MARKER_CLEAR_FAILED);
    assert(f.restored && f.marker_present && f.backup_present);

    memset(&f, 0, sizeof(f)); f.marker=(panel_wifi_txn_marker_t){PANEL_WIFI_TXN_RECOVERY_REQUIRED,true,24U};
    f.marker_present=true; f.backup_present=true; f.fail_at=OP_BACKUP_CLEAR; ops=ops_for(&f);
    assert(panel_wifi_txn_restore_previous(&ops,&f.marker) == PANEL_WIFI_TXN_STEP_BACKUP_CLEANUP_FAILED);
    assert(f.restored && !f.marker_present && f.backup_present);

    memset(&f, 0, sizeof(f)); f.marker=(panel_wifi_txn_marker_t){PANEL_WIFI_TXN_RECOVERY_REQUIRED,true,25U};
    f.marker_present=true; f.backup_present=true; f.restore_then_fail=true; ops=ops_for(&f);
    assert(panel_wifi_txn_restore_previous(&ops,&f.marker) == PANEL_WIFI_TXN_STEP_RESTORE_FAILED);
    assert(f.marker_present && f.backup_present && f.restored);
}

static void boot_quarantine_policy(void)
{
    panel_wifi_txn_marker_t marker={PANEL_WIFI_TXN_CANDIDATE_VERIFY_PENDING,true,50U};
    panel_wifi_txn_boot_policy_t p=panel_wifi_txn_classify_boot(
        true,false,false,NULL,true,false,false);
    assert(!p.quarantine && !p.restore_previous_available && !p.clear_first_install_available);
    p=panel_wifi_txn_classify_boot(true,false,false,NULL,true,true,true);
    assert(p.quarantine && p.restore_previous_available && !p.clear_first_install_available);
    p=panel_wifi_txn_classify_boot(true,true,true,&marker,true,true,true);
    assert(p.quarantine && p.restore_previous_available && !p.clear_first_install_available);
    marker.previous_config_present=false;
    p=panel_wifi_txn_classify_boot(true,true,true,&marker,true,false,false);
    assert(p.quarantine && !p.restore_previous_available && p.clear_first_install_available);
    p=panel_wifi_txn_classify_boot(true,true,true,&marker,true,true,true);
    assert(p.quarantine && !p.restore_previous_available && !p.clear_first_install_available);
    p=panel_wifi_txn_classify_boot(true,true,false,NULL,true,true,true);
    assert(p.quarantine && p.restore_previous_available && !p.clear_first_install_available);
    p=panel_wifi_txn_classify_boot(false,false,false,NULL,true,false,false);
    assert(p.quarantine && !p.restore_previous_available && !p.clear_first_install_available);
    p=panel_wifi_txn_classify_boot(true,true,true,&marker,false,false,false);
    assert(p.quarantine && !p.clear_first_install_available);
}
static void rollback_slot_interleavings(void)
{
    panel_wifi_recovery_slot_t slot; panel_wifi_recovery_slot_init(&slot);
    assert(panel_wifi_recovery_publish_rollback(&slot, 30U));
    assert(panel_wifi_recovery_publish_rollback(&slot, 30U)); /* duplicate */
    assert(panel_wifi_recovery_claim_rollback(&slot, 30U));
    assert(!panel_wifi_recovery_claim_rollback(&slot, 30U));
    assert(!panel_wifi_recovery_claim_manual(&slot, 30U));
    assert(panel_wifi_recovery_finish(&slot,30U,PANEL_WIFI_RECOVERY_OWNER_ROLLBACK_EXECUTING));
    assert(panel_wifi_recovery_rollback_already_attempted(&slot,30U));
    assert(!panel_wifi_recovery_publish_rollback(&slot,30U));
    assert(panel_wifi_recovery_publish_rollback(&slot,31U));
    assert(!panel_wifi_recovery_claim_rollback(&slot,30U)); /* stale generation */
    assert(panel_wifi_recovery_claim_manual(&slot,31U));
    assert(!panel_wifi_recovery_claim_rollback(&slot,31U));
    assert(panel_wifi_recovery_finish(&slot,31U,PANEL_WIFI_RECOVERY_OWNER_MANUAL_SERVICE));
}
int main(void)
{
    marker_codec(); candidate_order_and_failures(); recovery_terminal_order();
    boot_quarantine_policy();
    rollback_slot_interleavings();
    puts("PASS: durable Wi-Fi transaction/recovery production helpers");
    return 0;
}
