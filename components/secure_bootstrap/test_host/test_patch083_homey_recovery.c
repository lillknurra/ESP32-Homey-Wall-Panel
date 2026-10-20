/* Production worker/count_collection path extraction, with deterministic mock transport and clock. */
static void patch083_run_with_response(athom_refresh_origin_t origin, unsigned device_failures, int http_status, esp_err_t perform_error, bool invalidate_session)
{
    reset_patch059_integration_case(0U, 0U, ATHOM_HOMEY_DATA_LOADING);
    s_device_snapshot_store.active = false;
    s_v41_active = true;
    s_v41_wait = 0U;
    s_v41_device_calls = 0U;
    s_v41_device_failures = device_failures;
    s_v41_http_status = http_status;
    s_v41_perform_error = perform_error;
    s_v41_invalid_session = invalidate_session;
    assert(queue_inventory_refresh_if_ready(origin) == ATHOM_REFRESH_QUEUE_OK);
    run_one_worker_command();
    s_v41_active = false;
#ifndef PATCH083_BASELINE_CAPTURE
    const uint32_t expected_wait = origin != ATHOM_REFRESH_ORIGIN_BOOT_AUTO ||
        device_failures == 0U || s_v41_http_status == 401 ||
        s_v41_http_status == 403
        ? 0U : (s_v41_http_status == 429 ? 60000U :
            (s_v41_timeout_between ? 15000U : 5000U));
    assert(s_v41_wait == expected_wait);
#else
    assert(s_v41_wait == 0U);
#endif
    assert(!s_network_reserved && !s_refresh_job_reserved);
    assert(s_response_free_count == s_response_count);
    assert(s_write_attempts == 0U && s_response_double_free_count == 0U);
}

static void patch083_run(athom_refresh_origin_t origin, unsigned device_failures)
{
    patch083_run_with_response(
        origin, device_failures, 200, ESP_ERR_HTTP_INCOMPLETE_DATA, false);
}

static void test_patch083_incomplete_devices_recovery(void)
{
#ifdef PATCH083_BASELINE_CAPTURE
    patch083_run(ATHOM_REFRESH_ORIGIN_BOOT_AUTO, 1U);
    printf("PATCH083_BASELINE_OBSERVED device_calls=%u state=%d snapshots=%u http=%d stage=%s\n",
           s_v41_device_calls, s_homey_data_state, s_snapshot_publish_count,
           s_diagnostic_http_status, s_diagnostic_stage);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_ERROR);
    assert(s_v41_device_calls == 1U);
    assert(s_snapshot_publish_count == 0U && !s_device_snapshot_store.active);
    assert(s_diagnostic_error == ESP_ERR_HTTP_INCOMPLETE_DATA);
    assert(s_diagnostic_http_status == 200);
    puts("PATCH083_BASELINE_REPRODUCTION=PASS");
#else
    patch083_run(ATHOM_REFRESH_ORIGIN_BOOT_AUTO, 0U);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(s_v41_device_calls == 1U && s_snapshot_publish_count == 1U);
    assert(s_discovery_attempts == 0U && s_select_connect_attempts == 0U);

    patch083_run(ATHOM_REFRESH_ORIGIN_BOOT_AUTO, 1U);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(strcmp(s_state_name, "ready") == 0);
    assert(s_v41_device_calls == 2U && s_snapshot_publish_count == 1U);
    assert(s_discovery_attempts == 0U && s_select_connect_attempts == 0U);
    assert(s_device_snapshot_store.active);
    assert(s_device_snapshot_store.snapshot.generation == 1U);
    assert(strstr(s_last_device_url, "/api/manager/devices/device") != NULL);
    assert(strcmp(s_last_device_authorization, "Bearer synthetic") == 0);

    /* Other transport failures retain the existing generic transient policy. */
    patch083_run_with_response(ATHOM_REFRESH_ORIGIN_BOOT_AUTO, 1U, 0,
                               ESP_ERR_TIMEOUT, false);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(s_v41_device_calls == 2U && s_snapshot_publish_count == 1U);

    patch083_run_with_response(ATHOM_REFRESH_ORIGIN_BOOT_AUTO, 1U, 0,
                               ESP_ERR_HTTP_EAGAIN, false);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(s_v41_device_calls == 2U && s_snapshot_publish_count == 1U);

    patch083_run(ATHOM_REFRESH_ORIGIN_BOOT_AUTO, 2U);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_ERROR);
    assert(s_v41_device_calls == 2U && s_snapshot_publish_count == 0U);
    assert(!s_device_snapshot_store.active);
    assert(s_no_snapshot_recovery_pending);
    assert(s_no_snapshot_recovery_delay_ms == PERIODIC_REFRESH_COOLDOWN_MS);

    patch083_run(ATHOM_REFRESH_ORIGIN_PERIODIC, 1U);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_ERROR && s_v41_device_calls == 1U);
    assert(s_no_snapshot_recovery_pending);
    patch083_run(ATHOM_REFRESH_ORIGIN_MANUAL, 1U);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_ERROR && s_v41_device_calls == 1U);
    assert(!s_no_snapshot_recovery_pending);

    s_v41_timeout_between = true;
    patch083_run(ATHOM_REFRESH_ORIGIN_BOOT_AUTO, 3U);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_ERROR && s_v41_device_calls == 3U);
    assert(s_snapshot_publish_count == 0U);
    s_v41_timeout_between = false;

    s_v41_corrupt_identity = true;
    patch083_run(ATHOM_REFRESH_ORIGIN_BOOT_AUTO, 1U);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_ERROR && s_snapshot_publish_count == 0U);
    assert(s_v41_device_calls == 1U);
    s_v41_corrupt_identity = false;

    s_v41_alias_fail = true;
    patch083_run(ATHOM_REFRESH_ORIGIN_BOOT_AUTO, 1U);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_ERROR && s_snapshot_publish_count == 0U);
    assert(s_v41_device_calls == 1U);
    s_v41_alias_fail = false;

    patch083_run_with_response(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, 1U, 200,
        ESP_ERR_HTTP_INCOMPLETE_DATA, true);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_ERROR);
    assert(s_v41_device_calls == 1U && s_snapshot_publish_count == 0U);
    s_v41_invalid_session = false;

    for (unsigned origin = 0U; origin <= ATHOM_REFRESH_ORIGIN_PRESELECTION_RESTORE; ++origin) {
        assert(boot_devices_incomplete_retry_allowed(
            origin, ESP_ERR_HTTP_INCOMPLETE_DATA, "inventory_devices", 0, 0U) ==
            (origin == ATHOM_REFRESH_ORIGIN_BOOT_AUTO));
    }
    assert(boot_devices_incomplete_retry_allowed(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, ESP_ERR_HTTP_INCOMPLETE_DATA,
        "inventory_devices", 200, 0U));
    assert(!boot_devices_incomplete_retry_allowed(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, ESP_ERR_HTTP_INCOMPLETE_DATA,
        "inventory_devices", 401, 0U));
    assert(!boot_devices_incomplete_retry_allowed(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, ESP_ERR_HTTP_INCOMPLETE_DATA,
        "inventory_devices", 403, 0U));
    patch083_run_with_response(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, 1U, 401,
        ESP_ERR_HTTP_INCOMPLETE_DATA, false);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_ERROR);
    assert(s_v41_device_calls == 1U && s_v41_wait == 0U);
    assert(s_snapshot_publish_count == 0U);
    assert(!s_no_snapshot_recovery_pending);
    patch083_run_with_response(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, 1U, 403,
        ESP_ERR_HTTP_INCOMPLETE_DATA, false);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_ERROR);
    assert(s_v41_device_calls == 1U && s_v41_wait == 0U);
    assert(s_snapshot_publish_count == 0U);
    assert(!s_no_snapshot_recovery_pending);

    patch083_run_with_response(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, 1U, 429, ESP_OK, false);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(s_v41_device_calls == 2U && s_v41_wait == 60000U);
    assert(s_snapshot_publish_count == 1U);
    assert(!boot_devices_incomplete_retry_allowed(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, ESP_ERR_HTTP_INCOMPLETE_DATA,
        "inventory_devices", 0, 1U));
    assert(!boot_devices_incomplete_retry_allowed(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, ESP_ERR_HTTP_INCOMPLETE_DATA,
        "inventory_zones", 0, 0U));
    assert(!boot_devices_incomplete_retry_allowed(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, ESP_ERR_TIMEOUT,
        "inventory_devices", 0, 0U));
    assert(!homey_data_failure_is_transient(ESP_ERR_HTTP_INCOMPLETE_DATA, 0));
    puts("PATCH083_BOUNDED_RECOVERY=PASS");
#endif
}

static void test_cached_devices_401_uses_one_existing_oauth_recovery(void)
{
    reset_patch059_integration_case(0U, 0U, ATHOM_HOMEY_DATA_LOADING);
    s_device_snapshot_store.active = false;
    s_v41_active = true;
    s_v41_device_calls = 0U;
    s_v41_device_failures = 1U;
    s_v41_http_status = 401;
    s_v41_perform_error = ESP_FAIL;
    s_mock_refresh_token_available = true;
    s_mock_discovery_status_sequence[0] = 401;
    s_mock_discovery_status_sequence[1] = 200;
    s_mock_discovery_status_sequence_count = 2U;
    s_mock_discovery_status_sequence_index = 0U;
    s_mock_cloud_refresh_succeeds = true;

    assert(queue_inventory_refresh_if_ready(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO) == ATHOM_REFRESH_QUEUE_OK);
    run_one_worker_command();
    s_v41_active = false;

    assert(s_v41_device_calls == 2U);
    assert(s_discovery_attempts == 2U);
    assert(s_mock_cloud_refresh_calls == 1U);
    assert(s_select_connect_attempts == 1U);
    assert(s_snapshot_publish_count == 1U);
    assert(s_device_snapshot_store.active);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(strcmp(s_state_name, "ready") == 0);
    assert(!s_network_reserved && !s_refresh_job_reserved);
    assert(s_write_attempts == 0U);
}

static void test_cached_401_recovery_gate_covers_inventory_stages(void)
{
    const char *stages[] = {
        "favorites_user_me",
        "inventory_zones",
        "inventory_devices",
    };
    for (size_t i = 0U; i < sizeof(stages) / sizeof(stages[0]); ++i) {
        assert(boot_cached_session_401_recovery_allowed(
            ATHOM_REFRESH_ORIGIN_BOOT_AUTO, true, false, ESP_FAIL,
            stages[i], 401));
    }
    assert(!boot_cached_session_401_recovery_allowed(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, true, false, ESP_FAIL,
        "oauth_user_me_http", 401));
    assert(!boot_cached_session_401_recovery_allowed(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, true, false, ESP_FAIL,
        "favorites_user_me", 403));
    assert(!boot_cached_session_401_recovery_allowed(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO, true, true, ESP_FAIL,
        "inventory_zones", 401));
    assert(boot_cached_session_401_recovery_allowed(
        ATHOM_REFRESH_ORIGIN_PERIODIC, true, false, ESP_FAIL,
        "inventory_devices", 401));
}

static void configure_one_bounded_cached_auth_recovery(void)
{
    s_mock_refresh_token_available = true;
    s_mock_discovery_status_sequence[0] = 401;
    s_mock_discovery_status_sequence[1] = 200;
    s_mock_discovery_status_sequence_count = 2U;
    s_mock_discovery_status_sequence_index = 0U;
    s_mock_cloud_refresh_succeeds = true;
}

static void test_favorites_401_recovers_once_and_publishes_complete_inventory(void)
{
    reset_patch059_integration_case(0U, 0U, ATHOM_HOMEY_DATA_LOADING);
    s_device_snapshot_store.active = false;
    configure_one_bounded_cached_auth_recovery();
    s_favorite_status_sequence[0] = 401;
    s_favorite_status_sequence[1] = 200;
    s_favorite_status_sequence_count = 2U;
    assert(queue_inventory_refresh_if_ready(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO) == ATHOM_REFRESH_QUEUE_OK);
    run_one_worker_command();

    assert(s_favorites_fetch_count == 2U);
    assert(s_request_count == 2U); /* Zones + Devices only on recovered attempt. */
    assert(s_discovery_attempts == 2U && s_mock_cloud_refresh_calls == 1U);
    assert(s_select_connect_attempts == 1U);
    assert(s_snapshot_publish_count == 1U && s_device_snapshot_store.active);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(strcmp(s_state_name, "ready") == 0);
    assert(s_write_attempts == 0U);
    assert(!s_network_reserved && !s_refresh_job_reserved);
}

static void test_zones_401_recovers_once_and_publishes_complete_inventory(void)
{
    reset_patch059_integration_case(0U, 0U, ATHOM_HOMEY_DATA_LOADING);
    s_device_snapshot_store.active = false;
    configure_one_bounded_cached_auth_recovery();
    s_zones_http_status_sequence[0] = 401;
    s_zones_http_status_sequence[1] = 200;
    s_zones_http_status_sequence_count = 2U;
    assert(queue_inventory_refresh_if_ready(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO) == ATHOM_REFRESH_QUEUE_OK);
    run_one_worker_command();

    assert(s_favorites_fetch_count == 2U);
    assert(s_request_count == 3U); /* failed Zones, then Zones + Devices. */
    assert(s_discovery_attempts == 2U && s_mock_cloud_refresh_calls == 1U);
    assert(s_select_connect_attempts == 1U);
    assert(s_snapshot_publish_count == 1U && s_device_snapshot_store.active);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(strcmp(s_state_name, "ready") == 0);
    assert(s_write_attempts == 0U);
    assert(!s_network_reserved && !s_refresh_job_reserved);
}

static void test_repeated_cached_401_has_only_one_auth_recovery(void)
{
    reset_patch059_integration_case(0U, 0U, ATHOM_HOMEY_DATA_LOADING);
    s_device_snapshot_store.active = false;
    configure_one_bounded_cached_auth_recovery();
    s_v41_active = true;
    s_v41_device_calls = 0U;
    s_v41_device_failures = 2U;
    s_v41_http_status = 401;
    s_v41_perform_error = ESP_FAIL;
    assert(queue_inventory_refresh_if_ready(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO) == ATHOM_REFRESH_QUEUE_OK);
    run_one_worker_command();
    s_v41_active = false;

    assert(s_v41_device_calls == 2U);
    assert(s_discovery_attempts == 2U && s_mock_cloud_refresh_calls == 1U);
    assert(s_select_connect_attempts == 1U);
    assert(s_snapshot_publish_count == 0U && !s_device_snapshot_store.active);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_ERROR);
    assert(!s_no_snapshot_recovery_pending);
    assert(!s_network_reserved && !s_refresh_job_reserved);
    assert(s_write_attempts == 0U);
}

static void test_favorites_401_cannot_reuse_old_snapshot_for_awning_write(void)
{
    reset_patch059_integration_case(12U, 1U, ATHOM_HOMEY_DATA_LOADING);
    s_favorite_status = 401;
    s_mock_discovery_status = 401;
    assert(queue_inventory_refresh_if_ready(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO) == ATHOM_REFRESH_QUEUE_OK);
    run_one_worker_command();

    assert(s_homey_data_state == ATHOM_HOMEY_DATA_ERROR);
    assert(s_device_snapshot_store.active);
    assert(s_device_snapshot_store.snapshot.generation == 12U);
    assert(s_snapshot_publish_count == 0U);
    assert(s_discovery_attempts == 1U && s_mock_cloud_refresh_calls == 0U);
    assert(athom_oauth_runtime_queue_awning_step(
        1U, PANEL_ROLLER_DIRECTION_UP) == ATHOM_AWNING_QUEUE_NOT_READY);
    assert(s_write_attempts == 0U && s_mock_awning_put_calls == 0U);
    assert(!s_network_reserved && !s_refresh_job_reserved);
}

static void test_favorites_401_refresh_failure_stays_closed(void)
{
    reset_patch059_integration_case(0U, 0U, ATHOM_HOMEY_DATA_LOADING);
    s_device_snapshot_store.active = false;
    s_favorite_status = 401;
    s_mock_refresh_token_available = true;
    s_mock_cloud_refresh_succeeds = false;
    s_mock_discovery_status = 401;
    assert(queue_inventory_refresh_if_ready(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO) == ATHOM_REFRESH_QUEUE_OK);
    run_one_worker_command();

    assert(s_homey_data_state == ATHOM_HOMEY_DATA_ERROR);
    assert(s_mock_cloud_refresh_calls == 1U);
    assert(s_discovery_attempts == 1U && s_select_connect_attempts == 0U);
    assert(s_snapshot_publish_count == 0U && !s_device_snapshot_store.active);
    assert(!s_no_snapshot_recovery_pending);
    assert(s_write_attempts == 0U && s_mock_awning_put_calls == 0U);
    assert(!s_network_reserved && !s_refresh_job_reserved);
}

static void test_favorites_401_not_supported_recovers_through_worker(void)
{
    reset_patch059_integration_case(0U, 0U, ATHOM_HOMEY_DATA_LOADING);
    s_device_snapshot_store.active = false;
    configure_one_bounded_cached_auth_recovery();
    s_favorite_status_sequence[0] = 401;
    s_favorite_status_sequence[1] = 200;
    s_favorite_status_sequence_count = 2U;
    s_favorite_error_sequence[0] = ESP_ERR_NOT_SUPPORTED;
    s_favorite_error_sequence[1] = ESP_OK;
    s_favorite_error_sequence_count = 2U;

    assert(queue_inventory_refresh_if_ready(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO) == ATHOM_REFRESH_QUEUE_OK);
    run_one_worker_command();

    /* Follow the real inventory call order and worker recovery, not only the
     * 401 predicate: fresh Favorites 401 + IDF's unsupported auth error must
     * enter exactly one existing OAuth/delegation recovery. */
    assert(s_favorites_fetch_count == 2U);
    assert(s_discovery_attempts == 2U && s_mock_cloud_refresh_calls == 1U);
    assert(s_request_count == 2U); /* recovered Zones + Devices */
    assert(s_snapshot_publish_count == 1U && s_device_snapshot_store.active);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(strcmp(s_state_name, "ready") == 0);
    assert(!s_network_reserved && !s_refresh_job_reserved);
    assert(s_write_attempts == 0U);
}

static void test_zones_401_not_supported_recovers_through_worker(void)
{
    reset_patch059_integration_case(0U, 0U, ATHOM_HOMEY_DATA_LOADING);
    s_device_snapshot_store.active = false;
    configure_one_bounded_cached_auth_recovery();
    s_collection_error_sequence[0] = ESP_ERR_NOT_SUPPORTED;
    s_collection_status_sequence[0] = 401;
    s_collection_response_sequence_count = 1U;

    assert(queue_inventory_refresh_if_ready(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO) == ATHOM_REFRESH_QUEUE_OK);
    run_one_worker_command();

    assert(s_request_count == 3U); /* failed Zones, then Zones + Devices */
    assert(s_discovery_attempts == 2U && s_mock_cloud_refresh_calls == 1U);
    assert(s_snapshot_publish_count == 1U && s_device_snapshot_store.active);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(strcmp(s_state_name, "ready") == 0);
    assert(!s_network_reserved && !s_refresh_job_reserved);
    assert(s_write_attempts == 0U);
}

static void test_devices_401_not_supported_recovers_through_worker(void)
{
    reset_patch059_integration_case(0U, 0U, ATHOM_HOMEY_DATA_LOADING);
    s_device_snapshot_store.active = false;
    configure_one_bounded_cached_auth_recovery();
    s_collection_error_sequence[1] = ESP_ERR_NOT_SUPPORTED;
    s_collection_status_sequence[0] = 200;
    s_collection_status_sequence[1] = 401;
    s_collection_response_sequence_count = 2U;

    assert(queue_inventory_refresh_if_ready(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO) == ATHOM_REFRESH_QUEUE_OK);
    run_one_worker_command();

    assert(s_request_count == 4U); /* Zones + failed Devices, then both */
    assert(s_discovery_attempts == 2U && s_mock_cloud_refresh_calls == 1U);
    assert(s_snapshot_publish_count == 1U && s_device_snapshot_store.active);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(strcmp(s_state_name, "ready") == 0);
    assert(!s_network_reserved && !s_refresh_job_reserved);
    assert(s_write_attempts == 0U);
}

static void test_not_supported_without_fresh_401_does_not_recover(void)
{
    reset_patch059_integration_case(0U, 0U, ATHOM_HOMEY_DATA_LOADING);
    s_device_snapshot_store.active = false;
    s_favorite_error = ESP_ERR_NOT_SUPPORTED;
    s_favorite_status = 0;

    assert(queue_inventory_refresh_if_ready(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO) == ATHOM_REFRESH_QUEUE_OK);
    run_one_worker_command();

    assert(s_favorites_fetch_count == 1U);
    assert(s_discovery_attempts == 0U && s_mock_cloud_refresh_calls == 0U);
    /* Favorites remains optional for inventory when there is no verified
     * 401; complete Zones + Devices may still publish without OAuth retry. */
    assert(s_request_count == 2U);
    assert(s_snapshot_publish_count == 1U && s_device_snapshot_store.active);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(!s_no_snapshot_recovery_pending);
    assert(!s_network_reserved && !s_refresh_job_reserved);
    assert(s_write_attempts == 0U);
}

static void assert_cached_inventory_401_stage(
    const char *expected_stage, unsigned collection_call_for_401)
{
    reset_case();
    if (expected_stage == NULL) {
        s_favorite_status = 401;
        s_favorite_error = ESP_ERR_NOT_SUPPORTED;
    } else {
        s_favorite_status = 200;
        s_collection_status_sequence[collection_call_for_401] = 401;
        s_collection_error_sequence[collection_call_for_401] =
            ESP_ERR_NOT_SUPPORTED;
        s_collection_response_sequence_count = collection_call_for_401 + 1U;
        for (unsigned i = 0U; i < collection_call_for_401; ++i) {
            s_collection_status_sequence[i] = 200;
            s_collection_error_sequence[i] = ESP_OK;
        }
    }

    const esp_err_t result = athom_cloud_fetch_inventory_from_cached_session(
        &s_cloud_state, "synthetic-homey");
    assert(result == ESP_FAIL);
    assert(strcmp(s_diagnostic_stage, expected_stage != NULL
        ? expected_stage : "favorites_user_me") == 0);
    assert(s_diagnostic_http_status == 401);
    assert(s_diagnostic_error == ESP_FAIL);
    assert(s_transport_stage_raw_error == ESP_ERR_NOT_SUPPORTED);
    assert(s_snapshot_publish_count == 0U && !s_device_snapshot_store.active);
    if (expected_stage == NULL ||
        strcmp(expected_stage, "inventory_zones") == 0) {
        assert(s_collection_parse_count == 0U);
    }
}

static void test_401_not_supported_preserves_stage_and_raw_transport_error(void)
{
    assert_cached_inventory_401_stage(NULL, 0U);
    assert_cached_inventory_401_stage("inventory_zones", 0U);
    assert_cached_inventory_401_stage("inventory_devices", 1U);
}

static void test_favorites_auth_rejection_does_not_publish_inventory(void)
{
    for (int status = 401; status <= 403; status += 2) {
        reset_case();
        s_favorite_status = status;
        esp_err_t effective = ESP_OK;
        const esp_err_t result = athom_cloud_fetch_inventory_from_cached_session(
            &s_cloud_state, "synthetic-homey");
        assert(result != ESP_OK);
        assert(!inventory_verified(result, &effective));
        assert(s_diagnostic_http_status == status);
        assert(s_snapshot_publish_count == 0U);
        assert(!s_device_snapshot_store.active);
        assert(s_collection_parse_count == 0U);
        assert(s_request_count == 0U);
    }
}

static void test_zones_auth_rejection_stops_before_devices(void)
{
    for (int status = 401; status <= 403; status += 2) {
        reset_case();
        s_zones_http_status = status;
        esp_err_t effective = ESP_OK;
        const esp_err_t result = athom_cloud_fetch_inventory_from_cached_session(
            &s_cloud_state, "synthetic-homey");
        assert(result != ESP_OK);
        assert(!inventory_verified(result, &effective));
        assert(s_diagnostic_http_status == status);
        assert(strcmp(s_diagnostic_stage, "inventory_zones") == 0);
        assert(s_snapshot_publish_count == 0U);
        assert(!s_device_snapshot_store.active);
        assert(s_request_count == 1U); /* Zones only; no Devices request. */
        assert(s_collection_parse_count == 0U);
    }
}

static void test_devices_403_does_not_enter_auth_recovery(void)
{
    reset_patch059_integration_case(0U, 0U, ATHOM_HOMEY_DATA_LOADING);
    s_device_snapshot_store.active = false;
    s_v41_active = true;
    s_v41_device_calls = 0U;
    s_v41_device_failures = 1U;
    s_v41_http_status = 403;
    s_v41_perform_error = ESP_FAIL;
    assert(queue_inventory_refresh_if_ready(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO) == ATHOM_REFRESH_QUEUE_OK);
    run_one_worker_command();
    s_v41_active = false;

    assert(s_v41_device_calls == 1U);
    assert(s_discovery_attempts == 0U);
    assert(s_mock_cloud_refresh_calls == 0U);
    assert(s_select_connect_attempts == 0U);
    assert(s_snapshot_publish_count == 0U);
    assert(!s_no_snapshot_recovery_pending);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_ERROR);
    assert(!s_network_reserved && !s_refresh_job_reserved);
}

static void test_cached_devices_401_without_valid_cloud_auth_fails_closed(void)
{
    reset_patch059_integration_case(0U, 0U, ATHOM_HOMEY_DATA_LOADING);
    s_device_snapshot_store.active = false;
    s_v41_active = true;
    s_v41_device_calls = 0U;
    s_v41_device_failures = 1U;
    s_v41_http_status = 401;
    s_v41_perform_error = ESP_FAIL;
    s_mock_discovery_status = 401;
    s_mock_refresh_token_available = false;
    assert(queue_inventory_refresh_if_ready(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO) == ATHOM_REFRESH_QUEUE_OK);
    run_one_worker_command();
    s_v41_active = false;

    assert(s_v41_device_calls == 1U);
    assert(s_discovery_attempts == 1U);
    assert(s_mock_cloud_refresh_calls == 0U);
    assert(s_select_connect_attempts == 0U);
    assert(s_snapshot_publish_count == 0U && !s_device_snapshot_store.active);
    assert(!s_no_snapshot_recovery_pending);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_ERROR);
    assert(strcmp(s_state_name, "homey_connection_error") == 0);
    assert(!s_network_reserved && !s_refresh_job_reserved);
    assert(s_write_attempts == 0U);
}

static void test_repeated_eagain_hands_off_to_no_snapshot_scheduler(void)
{
    reset_patch059_integration_case(0U, 0U, ATHOM_HOMEY_DATA_LOADING);
    s_device_snapshot_store.active = false;
    s_v41_active = true;
    s_v41_device_calls = 0U;
    s_v41_device_failures = 100U;
    s_v41_http_status = 0;
    s_v41_perform_error = ESP_ERR_HTTP_EAGAIN;
    assert(queue_inventory_refresh_if_ready(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO) == ATHOM_REFRESH_QUEUE_OK);
    run_one_worker_command();
    assert(s_v41_device_calls == 3U);
    assert(!s_device_snapshot_store.active && s_snapshot_publish_count == 0U);
    assert(s_no_snapshot_recovery_pending);
    assert(s_no_snapshot_recovery_delay_ms == PERIODIC_REFRESH_COOLDOWN_MS);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_ERROR);
    assert(!s_patch059_queue.has_command);
    assert(!s_network_reserved && !s_refresh_job_reserved);

    /* Once the bounded boot job releases its reservations, the existing
     * scheduler can make one later attempt and publish only complete data. */
    s_v41_device_failures = 0U;
    s_v41_http_status = 200;
    s_v41_perform_error = ESP_OK;
    s_scheduler_iterations = 0U;
    s_scheduler_iteration_limit = 35U;
    s_now_us = 0LL;
    run_periodic_scheduler_for_bounded_ticks();
    s_v41_active = false;
    assert(s_v41_device_calls == 4U);
    assert(s_snapshot_publish_count == 1U && s_device_snapshot_store.active);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_READY);
    assert(strcmp(s_state_name, "ready") == 0);
    assert(!s_no_snapshot_recovery_pending);
    assert(!s_network_reserved && !s_refresh_job_reserved);
}

static void test_invalid_cache_discovery_429_uses_long_bounded_handoff(void)
{
    reset_patch059_integration_case(0U, 0U, ATHOM_HOMEY_DATA_LOADING);
    s_device_snapshot_store.active = false;
    s_v41_active = false;
    s_v41_device_calls = 0U;
    s_cloud_state.selected_homey.remote_url[0] = '\0';
    s_mock_discovery_status = 429;
    assert(queue_inventory_refresh_if_ready(
        ATHOM_REFRESH_ORIGIN_BOOT_AUTO) == ATHOM_REFRESH_QUEUE_OK);
    run_one_worker_command();

    assert(s_discovery_attempts == 3U);
    assert(s_select_connect_attempts == 0U);
    assert(s_v41_device_calls == 0U);
    assert(s_snapshot_publish_count == 0U && !s_device_snapshot_store.active);
    assert(s_no_snapshot_recovery_pending);
    assert(s_no_snapshot_recovery_delay_ms >= ATHOM_HOMEY_DATA_429_RETRY_3_MS);
    assert(s_homey_data_state == ATHOM_HOMEY_DATA_ERROR);
    assert(!s_network_reserved && !s_refresh_job_reserved);
}
