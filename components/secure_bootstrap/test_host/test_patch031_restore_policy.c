#include "athom_restore_policy.h"
#include <assert.h>
#include <stdio.h>
int main(void)
{
 assert(athom_restore_policy_after_store(0,false,false)==ATHOM_RESTORE_POLICY_LOGIN_REQUIRED);
 assert(athom_restore_policy_after_store(-1,true,false)==ATHOM_RESTORE_POLICY_LOGIN_REQUIRED);
 assert(athom_restore_policy_after_store(0,true,true)==ATHOM_RESTORE_POLICY_COMPLETE_SELECTED);
 assert(athom_restore_policy_after_store(0,true,false)==ATHOM_RESTORE_POLICY_FETCH_HOMEYS);
 assert(athom_restore_policy_after_discovery(0,200,1,false,false,true)==ATHOM_RESTORE_POLICY_SELECTION_REQUIRED);
 assert(athom_restore_policy_after_discovery(0,200,5,false,false,true)==ATHOM_RESTORE_POLICY_SELECTION_REQUIRED);
 assert(athom_restore_policy_after_discovery(-1,401,0,false,false,true)==ATHOM_RESTORE_POLICY_REFRESH_AUTH);
 assert(athom_restore_policy_after_discovery(-1,403,0,false,false,true)==ATHOM_RESTORE_POLICY_REFRESH_AUTH);
 assert(athom_restore_policy_after_discovery(-1,401,0,true,false,true)==ATHOM_RESTORE_POLICY_LOGIN_REQUIRED);
 assert(athom_restore_policy_after_discovery(-1,0,0,false,true,true)==ATHOM_RESTORE_POLICY_RETRY_TRANSIENT);
 assert(athom_restore_policy_after_discovery(-1,503,0,false,true,false)==ATHOM_RESTORE_POLICY_CONNECTION_ERROR);
 assert(athom_restore_policy_after_discovery(-1,400,0,false,false,true)==ATHOM_RESTORE_POLICY_CONNECTION_ERROR);
 assert(athom_restore_policy_after_refresh(0)==ATHOM_RESTORE_POLICY_FETCH_HOMEYS);
 assert(athom_restore_policy_after_refresh(-1)==ATHOM_RESTORE_POLICY_LOGIN_REQUIRED);
 /* restore before Wi-Fi: pending must wait */
 assert(!athom_restore_policy_should_start_preselection(false,false,true,false));
 /* Wi-Fi before restore completes: restore-running blocks start */
 assert(!athom_restore_policy_should_start_preselection(true,true,true,false));
 /* both conditions satisfied: exactly one start may be claimed */
 assert(athom_restore_policy_should_start_preselection(true,false,true,false));
 /* consumed pending cannot start again */
 assert(!athom_restore_policy_should_start_preselection(true,false,false,false));
 /* already-running worker cannot start a duplicate */
 assert(!athom_restore_policy_should_start_preselection(true,false,true,true));
 puts("PATCH031_RESTORE_POLICY_TESTS PASS");
 return 0;
}
