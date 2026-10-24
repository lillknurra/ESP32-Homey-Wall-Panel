#include "panel_information.h"
#include <stdio.h>
#include <string.h>

void panel_information_observe_snapshot(panel_information_snapshot_t *out,
    panel_homey_read_result_t result, uint64_t captured_at_ms, uint32_t generation)
{
    if (out == NULL) return;
    out->observed = true;
    out->result = result;
    out->captured_at_ms = result == PANEL_HOMEY_READ_OK ? captured_at_ms : 0U;
    out->generation = result == PANEL_HOMEY_READ_OK ? generation : 0U;
}

const char *panel_information_title(panel_information_kind_t kind)
{
    static const char *const titles[] = {"Wi-Fi-status", "Homey-anslutning",
        "Firmwareversion", "Tidssynkronisering", "Systemdiagnostik"};
    return (unsigned)kind < PANEL_INFORMATION_COUNT ? titles[kind] : "Information";
}

static const char *homey_state(const panel_information_t *i)
{
    static const char *const states[] = {"Laddar", "Försöker igen", "READY", "Fel"};
    return i->homey_known && i->homey_state < 4U
        ? states[i->homey_state] : "Ej tillgänglig";
}

/* App metadata is bounded, never a format string; reject control characters. */
static void metadata(char *out, const char *in, size_t n)
{
    size_t k = 0U;
    for (; k < n && in[k] != '\0'; ++k) {
        unsigned char ch = (unsigned char)in[k];
        out[k] = ch >= 32U && ch <= 126U ? (char)ch : '?';
    }
    out[k] = '\0';
}

bool panel_information_format(panel_information_kind_t kind,
    const panel_information_t *i, char *out, size_t capacity)
{
    if (out == NULL || capacity == 0U) return false;
    out[0] = '\0';
    if (i == NULL || (unsigned)kind >= PANEL_INFORMATION_COUNT) return false;
    int length;
    switch (kind) {
    case PANEL_INFORMATION_WIFI:
        length = snprintf(out, capacity, "STA/IP: %s\nInstallationssession: %s\n\nLokal status vid avläsning.",
            !i->wifi_known ? "Ej tillgänglig" : i->wifi_online ? "Ansluten" : "Frånkopplad",
            !i->provisioning_known ? "Ej tillgänglig" : i->provisioning_active ? "Aktiv" : "Stängd");
        break;
    case PANEL_INFORMATION_HOMEY: {
        const char *snapshot = "Ej tillgänglig";
        if (i->snapshot.observed) {
            if (i->snapshot.result == PANEL_HOMEY_READ_NOT_FOUND ||
                i->snapshot.result == PANEL_HOMEY_READ_NOT_CONFIGURED) snapshot = "Saknas";
            else if (i->snapshot.result == PANEL_HOMEY_READ_STALE) snapshot = "Inaktuell";
            else if (i->snapshot.result == PANEL_HOMEY_READ_OK && i->snapshot.generation != 0U &&
                     i->now_ms >= i->snapshot.captured_at_ms)
                snapshot = i->now_ms - i->snapshot.captured_at_ms <= PANEL_HOMEY_SNAPSHOT_STALE_AFTER_MS
                    ? "Färsk" : "Inaktuell";
        }
        length = snprintf(out, capacity, "Homey: %s\nSTA/IP: %s\nSnapshot: %s\n\nSenaste lokala dashboarddata.\nInget nytt Homey-anrop görs.",
            homey_state(i), !i->wifi_known ? "Ej tillgänglig" : i->wifi_online ? "Ansluten" : "Frånkopplad", snapshot);
        break;
    }
    case PANEL_INFORMATION_FIRMWARE: {
        if (!i->firmware_known) { length = snprintf(out, capacity, "Ej tillgänglig"); break; }
        char version[33], date[17], time[17], idf[33];
        metadata(version, i->version, sizeof(i->version));
        metadata(date, i->build_date, sizeof(i->build_date));
        metadata(time, i->build_time, sizeof(i->build_time));
        metadata(idf, i->idf_version, sizeof(i->idf_version));
        length = snprintf(out, capacity, "Appversion: %s\nByggd: %s %s\nESP-IDF: %s\n\nVersion är inte image-SHA-256.",
            version[0] ? version : "Ej tillgänglig", date[0] ? date : "Ej tillgänglig",
            time[0] ? time : "", idf[0] ? idf : "Ej tillgänglig");
        break;
    }
    case PANEL_INFORMATION_TIME:
        length = snprintf(out, capacity, "Tid: %s\nTidszon: %s\nAutomatisk sommar-/vintertid.\n\nVisar befintlig SNTP-status.\nIngen omsynk startas.",
            !i->time_known ? "Ej tillgänglig" : i->time_synchronized ? "Synkroniserad" : "Ej synkroniserad",
            i->swedish_timezone ? "Svensk CET/CEST" : "Ej tillgänglig");
        break;
    case PANEL_INFORMATION_SYSTEM:
        if (!i->memory_known) {
            length = snprintf(out, capacity, "Drifttid: %llu s\nHomey: %s\nMinne: Ej tillgänglig",
                (unsigned long long)(i->now_ms / 1000U), homey_state(i));
        } else if (!i->lvgl_memory_known || i->lvgl_pool_size == 0U) {
            length = snprintf(out, capacity, "Drifttid: %llu s\nHomey: %s\nIntern heap ledig: %lu B\nIntern heap minimum: %lu B\nPSRAM ledig: %lu B\nLVGL-minne: Ej tillgängligt\n\nMinimum gäller sedan boot.",
                (unsigned long long)(i->now_ms / 1000U), homey_state(i),
                (unsigned long)i->internal_free, (unsigned long)i->internal_minimum,
                (unsigned long)i->psram_free);
        } else {
            length = snprintf(out, capacity, "Drifttid: %llu s\nHomey: %s\nIntern heap ledig: %lu B\nIntern heap minimum: %lu B\nPSRAM ledig: %lu B\nLVGL: %lu/%lu B nu, max %lu B\n\nMinimum gäller sedan boot.",
                (unsigned long long)(i->now_ms / 1000U), homey_state(i),
                (unsigned long)i->internal_free, (unsigned long)i->internal_minimum,
                (unsigned long)i->psram_free,
                (unsigned long)i->lvgl_current_used,
                (unsigned long)i->lvgl_pool_size,
                (unsigned long)i->lvgl_max_used);
        }
        break;
    default: return false;
    }
    if (length < 0 || (size_t)length >= capacity) {
        out[0] = '\0';
        return false;
    }
    return true;
}
