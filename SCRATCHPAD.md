# ESP32 Homey Wall Panel – Product Scratchpad

> **Aktuell läge — 2026-10-16:** PR #94 är verifierad MERGED till `main`.
> Patch084 är den integrerade och fysiskt verifierade produktionsbaslinjen.
> Panelens app-image har SHA-256
> `440a646b227ce96ecca8699cc230e37e092dc8cad404be4bb5a88de40e5d1e27`.
> Read-only runtime visade Homey READY, färsk/giltig inventeringssnapshot och
> giltiga markisalias; operatören bekräftade widgets, menyer, dubbelblink och
> inställningsfeedback. Patch084:s särskilda HTTP-retry aktiverades inte vid
> den observationen (första boot-inventoryförsöket lyckades). Käll-/test- och
> fysisk evidens är separata; se `docs/handoff/CURRENT_STATE.md` och
> `docs/handoff/HANDOFF.md`.
>
> **Aktuell prioritet:**
> 1. Patch085: säker styrning av Solskydd (`awning_2`) och Balkongmarkis
>    (`awning_3`) enligt befintlig femstegsmodell.
> 2. Separat senare patch: Markis (`awning_1`) med riktning UPP/STOPP/NED.
> 3. RGB-/helskärmsflimmer.
> 4. Långtidsstabilitet och minnesmarginal.
> 5. Framtida Verisure-integration.
>
> **Säkerhetsgrind för markisstyrning:** Ingen Homey-write får aktiveras förrän
> capability-kontrakt och kommandosemantik för respektive enhet verifierats,
> fail-closed-villkor och alias-/READY-/freshness-grindar testats, och fysisk
> styrning separat och uttryckligen auktoriserats. Ingen antagen capability,
> optimistisk positionsuppdatering eller fysisk manöver ingår i denna plan.
>
> Patch083:s bounded återhämtning vid `ESP_ERR_HTTP_INCOMPLETE_DATA` ingår i
> Patch084-källan och dess offline-regressioner passerade. Patch084-runtime
> återhämtade sig på första försöket; den särskilda retry-vägen är därför inte
> fysiskt verifierad. Wi-Fi STA/APSTA, QR-/lösenordsdialog och avbrytflöde
> ingår i integrerad kod; operatörsobservationen verifierade den normala UI:n,
> inte omprovisionering eller permanent Wi-Fi-commit.
>
> RGB-/helskärmsflimmer är fortfarande olöst. `NVS_BACKUP=NOT_VERIFIED`;
> target-LVGL-poolens marginal och långtidsminne är inte verifierade. Två
> historiska hosttestfel har tidigare dokumenterats; se respektive
> Patch084-testunderlag för deras klassificering.
>
> Alla äldre status- och `PENDING`-poster nedan är bevarad historik vid sina
> ursprungliga datum. De är inte den aktuella prioriteringslistan. Patch083/
> Patch084-status ovan ersätter specifikt äldre PENDING-punkter för Wi-Fi-
> refresh, Wi-Fi-dialog/cancel, borttagning av SAVED-modal och dubbelblink där
> dessa uppgifter faktiskt slutfördes. Äldre tekniska risker gäller fortfarande
> när de uttryckligen anges ovan.

## Projektmål

Utveckla en stabil, responsiv och lättanvänd väggpanel för Homey med:

- Synkroniserad tid och datum.
- Tydliga inställningsmenyer.
- Visning av aktuella markislägen.
- Visning av belysningens aktuella status.
- Säker manövrering av markiser och belysning.
- Stabil uppstart och långvarig drift.

## Status och evidensklasser

- **COMPLETE** – verifierat färdigt inom uttryckligen angiven evidensklass.
- **IN PROGRESS** – pågående arbete; återstående gate anges.
- **PENDING** – återstående arbete som inte startats.
- **BLOCKED** – konkret blockerare hindrar nästa steg.

Offlinevalidering, operatörsobservation och verifierad runtime-evidens hålls
åtskilda. Ett build- eller flash-PASS bevisar inte stabil drift.

## Projektstatus – Patch076 V23–V37 (historik; aktuell status se V41)

Git-baslinje kontrollerad mot handoff: primary `main`, development-branchen
`patch-073-network-time-sync`, cached `origin/main` och remote `main` har HEAD
`176e1a634eba060bd53427f2d12791db61fbc53e`. Development worktree är
`~/GitHub/ESP32-Homey-Wall-Panel-Patch073` och saknar avsiktligt
upstream. Samtliga befintliga ocommittade Patch073–076-ändringar ska bevaras.
Handoffens äldre V21-fastext är tidigare dokumenterat föråldrad; den uppdateras
inte i detta uppdrag.

- **COMPLETE – historisk runtime-evidens:** V22 fångade stackoverflow i
  `main` och `bootstrap_rotat`, två gånger vardera.
- **COMPLETE – V23 offlineändring:** main-stack 3584 → 8192 byte och
  rotation-stack 4096 → 8192 byte. Pollingens snapshot flyttades från stacken
  till statiskt, taskägt arbetsminne, vilket minskade pollingens stackanvändning.
- **COMPLETE – build-evidens:** V23 clean ESP-IDF v6.0.1-build PASS;
  image validation PASS.
- **COMPLETE – flash-evidens:** V24 app-only flash och flashverifiering PASS.
  Då installerad V23-firmware SHA256:
  `a31a5fac07de696f847548ce4abbf8c530bc0d9dc9f338746507379036c3ec78`.
- **COMPLETE – registrerad operatörsobservation efter V24:** inga upprepade
  omstarter; Homey-status ”Ansluten”; Ytterbelysning och Dekorbelysning ”Tänd”;
  Markis visar styrknappar; Solskydd och Balkongmarkis ”Okänd”.

### Kvarstående evidensgränser

- **PENDING:** ny verifierad runtime-stackmätning och längre stabilitetsprov.
  Stackrättningen har positiv operatörsobservation men är inte runtime-mätt:
  `RUNTIME_STACK_FIX=NOT_VERIFIED`.
- **PENDING:** oberoende verifiering av aktuell Homey READY och färskt snapshot
  efter V24. UI-texten ”Ansluten” är operatörsobservation:
  `HOMEY_READY_RUNTIME=NOT_VERIFIED`.
- `HOST_TESTS=PARTIAL`: 7/9 suites PASS; två dokumenterat preexisting fel
  kvarstår: Favorites assertion och Patch059 dashboard polling assertion.
- `NVS_BACKUP=NOT_VERIFIED`. Befintliga credentials, Wi-Fi, Homey-session,
  Homey-binding och ljusbindningar ska bevaras.
- NVS auto-erase och awning write path är disabled. Synliga styrknappar
  innebär inte verifierad eller auktoriserad markisstyrning.

### V26 – genomförd lokal aliasprovisionering

**COMPLETE – avgränsad skrivgate:** separat auktorisering gav exakt ett POST.
HTTP 200 och exakt JSON-resultat `{"result":"pass"}` mottogs.
`LOCAL_ALIAS_PROVISION=PASS`; auktoriseringen är förbrukad.
Ingen proxy, redirect, retry, extra GET, reset, flash, serial eller
Homey-mutation utfördes i V26. Inget nytt försök är tillåtet automatiskt.

Sanerad evidens:
`~/Downloads/PATCH076_V26_PREP_22pmci_w/post_result.sanitized.json`.

**COMPLETE – registrerad operatörsobservation efter V26, förtydligad i V27:**
Homey ”Ansluten”; Ytterbelysning och Dekorbelysning ”Tänd”.
Solskydd och Balkongmarkis är båda fysiskt helt uppdragna och visar vardera
fyra blå/turkosa horisontella streck. Den tidigare beskrivningen ”grå” är
ersatt av operatörens förtydligande. Panelen har inte startat om enligt
operatören. Ingen rörelse har begärts eller utförts av agenten.

**PENDING:** `POST_PROVISION_SNAPSHOT=NOT_VERIFIED` och
`POSITION_DISPLAY=NOT_VERIFIED`. POST-handlerns godkännande bekräftar dess
READY-/selected-Homey-gate vid skrivningen, men är inte en oberoende mätning av
ett nytt lyckat periodiskt inventorysnapshot.

Uppdragets formulering ”V26 väntar på separat NVS-auktorisering” är överspelad
av ovanstående verifierade POST-evidens. Den tidigare väntan bevaras som
historisk gate, inte som aktuell rätt att upprepa skrivningen.

### V27 – verifierad rendering och kvarstående positionsavvikelse

**COMPLETE – offline källgranskning:** utvecklingskällorna matchar V23:s
`source_after.json`. Ingen firmwareändring, build eller panelkontakt utfördes.

- `create_roller_card()` i `components/secure_bootstrap/panel_ui.c`
  skapar fyra basstreck, 56×5 pixlar, med mörkt blågrå färg `#435562`.
  Fyllningen har samma storlek och placering med ljusblå färg `#8BD3F7`,
  och ligger ovanpå respektive basstreck.
- `render_roller_widget()` visar samtliga fyra basstreck endast i verifierat
  positionsläge. Fyllningar visas uppifrån när `line < position_step`.
  Vid 0/1/2/3/4 fyllda staplar syns alltså 0/1/2/3/4 ljusblå och
  4/3/2/1/0 mörka staplar. Fyra synliga streck betyder inte i sig fyra fyllda.
- `panel_roller_position_to_step()` använder inversion och avrundning:
  `floor((1.0 - homey_value) * 4 + 0.5)`.
  Homey 1,00 (helt uppe) ger 0 fyllda och fyra mörka basstreck.
  Homey 0,00 (helt nere) ger fyra ljusblå fyllda staplar.
- Saknat, icke-numeriskt, icke-ändligt eller utanför 0–1, osäker bindning,
  ej READY eller stale snapshot ger ”Okänd” och inga staplar.
  Färsk verifierad unavailable-status ger ”Otillgänglig” och inga staplar.
  Markis/widget 0 visar inga positionsstaplar oavsett numeriskt läge.
- Staplar får inte visas från ett obekräftat defaultvärde. Ett giltigt,
  tillgängligt och färskt positionssnapshot krävs; efter V26:s aliasändring
  måste en lyckad normal inventering publicera värdet med rätt bindning.
  Renderern kontrollerar modellens flaggor, inte inventeringstransporten
  direkt. Snapshotframgång är därför inte oberoende verifierad av observationen.
- Fyra tydligt ljusblå/turkosa fyllningar `#8BD3F7` kan enligt koden inte
  representera 0 av 4 fyllda staplar. Fyra mörkt blågrå basstreck `#435562`
  kan däremot beskrivas som blå och motsvarar 0 fyllda.

**IN PROGRESS – runtime/positionsverifiering:** koden har korrekt riktning
och fyllnadsgrad för det tidigare verifierade Homey-kontraktet.
V28:s kompletterande operatörsunderlag anger att tidigare foto visar fyra
streck per markis som är mörkare än pilarna. Bedömningen är LIKELY att de är
mörka, tomma basstreck, vilket stämmer med helt uppdragna markiser och
Homey 1,00 = 0 fyllda. Detta är inte en ny oberoende runtime-/värdesmätning
och avslutar inte fullständig positionsverifiering.
`POSITION_DISPLAY_CORRECT=NOT_VERIFIED`. Ingen riktning ska inverteras
ytterligare och ingen kodkorrigering är motiverad av denna evidens ensam.

**PENDING – nästa steg:** granska ett operatörsfoto av panelens nuvarande
visning medan båda markiserna förblir helt uppe. Ingen flytt, knapptryckning,
provisionering eller paneloperation. Om fotot bekräftar fyra ljusa fyllningar,
förbered separat avgränsad read-only-verifiering av rapporterat värde,
bindning och snapshotfärskhet; den är inte auktoriserad här.

## Historisk produktbaseline – PR #93

- [x] PR #93 integrerad i main.
- [x] Firmware byggd med ESP-IDF v6.0.1.
- [x] Firmware installerad på fysisk panel.
- [x] Wi-Fi fungerar efter lyckad uppstart.
- [x] Homey readiness verifierad.
- [x] Fresh snapshot verifierad.
- [x] Tre markiser matchade och tillgängliga.
- [x] Favorites-status verifierad.
- [x] Patch071 FIN-recovery runtime PASS.
- [x] Dashboard visas på fysisk display.
- [ ] Touch och samtliga UI-flöden verifierade.
- [ ] Stabil första uppstart verifierad.
- [ ] Långtidsstabilitet verifierad.

Observera: Den första uppstarten efter PR #93-flashningen misslyckades med svart display och uteblivna HTTP-svar. GET 1–9 misslyckades före manuell strömcykling. Efter strömcykling fungerade panelen. Rotorsaken är inte fastställd.

## Patch073: Synkroniserad tid – implementation och historik

**Historisk Patch073-gate:** Implementation PASS offline; fokuserade tester PASS;
ren ESP-IDF v6.0.1-build PASS. Då var runtime `NOT_VERIFIED` och ingen flash
gjord inom Patch073. Funktionen ingår senare i installerad utvecklingsfirmware;
fullständig separat klockverifiering efter V24 återstår.

- [x] Implementera automatisk SNTP/NTP efter Wi-Fi IP-anslutning, utan väntan på synkronisering och med högst en initiering.
- [x] Använd svensk tidszon `Europe/Stockholm` med `CET-1CEST,M3.5.0/2,M10.5.0/3`.
- [x] Hantera sommar- och vintertid; hosttester täcker 2026 års övergångar.
- [x] Visa korrekt HH:MM med 24-timmarsformat.
- [x] Visa svenskt veckodags- och datumformat.
- [x] Uppdatera klockan vid första synk och minutbyte, utan sekundvis helskärmsrendering.
- [x] Hantera Wi-Fi-återanslutning utan dubbelinitiering av SNTP.
- [x] Fortsätt fungera utan tillgänglig NTP-server; Homey-readiness-vägen blockeras inte och osynkroniserad tid behåller placeholdern.
- [x] Verifiera LVGL-trådsäkerhet: callback ändrar endast synkflagga och UI uppdateras under displaylåset.
- [x] Kör fokuserade tester: klocka/DST, UI-modell, dashboardbindning och Patch069 Favorites readiness-isolering.
- [x] Genomför ren ESP-IDF-build för ESP32-S3.

Releasekandidat: `~/Downloads/PATCH073_RELEASE_20261010T000126Z/build_clean_final/esp32_homey_wall_panel.bin`. Historiskt var Patch074 nästa uppgift; den har därefter implementerats offline.

## Patch074: Inställningsmeny – implementation och historik

**Status:** Implementation PASS offline; Patch074:s fokuserade hosttester PASS; ren ESP-IDF v6.0.1-build PASS. Den kända äldre Favorites-dashboardtesten rapporterar fortsatt `PREEXISTING_FAIL`; Patch069 readiness-isolering och Patch071 FIN-test passerade. Fysisk runtime är `NOT_VERIFIED`.

### Skärm och navigation

- [x] X uppe till höger i Inställningar.
- [x] Skärminställningar direkt i huvudmenyn.
- [x] Separat Avancerat-meny.
- [x] Bakåtpil i Avancerat.
- [x] X uppe till höger i Avancerat.
- [x] Visa aktuella inställningsvärden.
- [x] Fast rubrikrad och separat scrollbart innehåll.
- [x] Touchytor minst 48 × 48 pixlar för menyrader.

### Avancerat

- [x] Anslutningar under Avancerat; ej implementerade funktioner visas som otillgängliga.
- [x] System under Avancerat; ej implementerade funktioner visas som otillgängliga.
- [x] Befintliga destruktiva bekräftelsevägar finns kvar; Homey- och Athom-stubbar är inte aktiverade.

### Beständig lagring

- [x] Återanvänd befintlig dual-slot NVS-store utan schema- eller partitionsändring.
- [x] Automatisk NVS-sparning via begränsad worker och sammanslagning av snabba ändringar.
- [x] Återläsning före UI/dashboard-initiering; saknad eller ogiltig post ger defaults utan settings-store-skrivning.
- [x] Bevarande vid app-only-flash verifierat offline genom oförändrad NVS-layout/store; fysisk flash är inte testad.
- [x] Synlig bekräftelse på lyckad sparning och fel vid misslyckad commit/readback.
- [x] Fokuserade tester för navigation, lagring, korrupt slot samt commit/readback-fel.
- [x] Ren ESP-IDF v6.0.1-build; appen ryms i befintlig factory-partition.
- [ ] Fysisk runtime-verifiering av navigation och sparning efter omstart/app-only-flash.

Releasekandidat: `~/Downloads/PATCH074_RELEASE_20261010T001500Z/esp32_homey_wall_panel.bin`. Patch073:s klockfunktioner ingår och regressionstesterna passerade. Ingen flash utfördes inom denna historiska Patch074-gate; senare V24-flash redovisas ovan.

## Patch075: Grafisk hemskärm och status – implementation och historik

**Offline implementation och verifiering:** PASS\
**Fysisk runtime:** PARTIALLY_OBSERVED; full funktionsverifiering saknas

### Hemskärm och navigation

- [x] Visa HH:MM utan datum och behåll anslutningstexten Ansluter/Ansluten.
- [x] Behåll den befintliga hamburgerikonen, placeringen och inställningsfunktionen.
- [x] Visa tre rullgardinswidgetar och två belysningswidgetar på hemsidan.
- [x] Behåll swipe mellan hemsidan och sida 2; tredje sidan är dold/inaktiv.
- [x] Flytta säkerhetsytan till sida 2 med tydlig placeholder när Verisure-data inte är verifierad.
- [x] Bevara Patch074-inställningsmenyn och Patch073-klockan.

### Rullgardinsstatus

- [x] Visa fyra streck i ett centralt fält för verifierade lägen med fem steg; fyll från översta strecket.
- [x] Visa Okänd eller Otillgänglig i samma fält utan streck eller extra statusrad.
- [x] Visa Otillgänglig endast när färsk, auktoritativ Homey-data uttryckligen anger unavailable.
- [x] Behandla stale data, osäker bindning och saknad position som Okänd.
- [x] Rita diskreta upp-/nedpilar med LVGL-linjer, utan beroende av font-glyphs eller ny Homey-write-path.
- [x] Lägg till och host-testa en begränsad femstegsmodell.
- [ ] Fysisk runtime- och layoutverifiering på 480 × 480.

Historiskt saknade Patch075 verifierad numerisk position och visade därför
Okänd. V23/V25 har nu verifierat numeriskt positionsstöd offline; V26:s
operatörsobservation och kvarstående positionsverifiering redovisas i den
aktuella Patch076-planen. Styrning kräver fortsatt separat säker integration.

### Belysning

- [x] Visa minimalistiska widgets med ikon och auktoritativ status.
- [x] Återanvänd den befintliga säkrade Favorite-toggle-funktionen utan optimistisk bekräftelse.
- [x] Visa neutralt tillstånd när status saknas eller är otillgänglig.

**Patch075 fokuserade tester:** PASS\
**Ren ESP-IDF v6.0.1-build, ESP32-S3:** PASS\
**Partitionstabell och NVS-layout:** oförändrade i källan.\
**Firmwarekandidat:** `~/Downloads/PATCH075_RELEASE_20261010T230444Z/esp32_homey_wall_panel.bin`\
**App SHA-256:** `fb650b936e23014bcb9032d1a4eb7e860186afc5f68b865c962e2326960ca5b5`

Fysisk observation efter full strömcykling: displayen blev synlig; Homey var `ready`; snapshot var färsk (generation 6); samtliga tre awnings var matchade/tillgängliga med befintliga bindningar. Numeriska positioner förblev ej verifierade. De fysiska pilarna visades tidigare som tomma rutor; Patch076 ersätter glyphs med LVGL-linjer. Pilfixen är offline-byggd men ännu inte fysiskt verifierad.

## Aktiv Patch076-plan – gemensam prioritering

### Prioritet 0 – förändringsstyrd visning och stabila Favorites

**Status: IN PROGRESS – V37 offlinekorrigering klar; fysisk verifiering återstår.**
`ACTIVE_ISSUE=BACKGROUND_REFRESH_AND_FAVORITES_TRANSIENT_PRESENTATION`.
V36:s minst sex minuters operatörskontroll bekräftar stabil uppstart, korrekt
favoritstatus, fungerande swipe/menyer och frånvaro av tidigare kraftiga
renderingsartefakter. Rotorsaken till att dessa upphörde är fortfarande UNKNOWN.
Ny film visar kort bakgrundsflimmer samt transient fallbacknamn/Okänd för lampor.
V37:s publika clear-lucka och onödiga LVGL-operationer är reproducerade och
korrigerade offline; nästa steg är separat kandidat-/app-only-flashgranskning.
Ingen fysisk operation eller ny aliasprovisionering är auktoriserad.

### Prioritet 1 – V25: positionsvisning för markiser

**Status: IN PROGRESS – offlinegranskning genomförd; RESULT=PARTIAL, runtimekrav återstår.**
Ingen provisionering ingår i V25. Detta dokumentuppdrag kör inte V25 igen.

Återanvänd den befintliga privata V9-bindningsfilen utanför Git:
`~/Library/Application Support/ESP32-Homey-Wall-Panel/patch076-v9-position-binding-20261011/awning_runtime_binding.private.json`.
Privata identifierare och rå payload får inte skrivas till arbetsplanen.

V25:s rapporterade offlineunderlag:

- **COMPLETE – kodkontrakt:** widget 0, Markis, använder
  `windowcoverings_state`; widget 1, Solskydd, och widget 2, Balkongmarkis,
  använder `windowcoverings_set`.
- **COMPLETE – positionsmodell:** ändliga numeriska värden inom 0–1;
  Homey 1,0 = uppe och 0,0 = nere enligt tidigare evidens.
  `fraction_down = 1.0 - homey_value` ger fem nivåer, 0–4 fyllda staplar.
  Värdena 1,00 / 0,75 / 0,50 / 0,25 / 0,00 ger 0 / 1 / 2 / 3 / 4 staplar.
- **COMPLETE – privat filkontroll:** schema, tre unika enheter, widgetindex
  0–2 och capabilities matchar sparad V6/V9-evidens; fil 0600 och katalog 0700.
- **COMPLETE – offline dry-run:** explicit `--private-binding` och
  `--dry-run` med `scripts/provision_patch_051_awning_bindings.py`, utan
  `--panel-url`; noll nätverkshändelser och ingen NVS-skrivning.
- **COMPLETE – kodkontrakt för ljus:** aliasmerge bevarar befintliga
  ljusbindningar. Panelens aktuella NVS-innehåll är inte verifierat.
- **PENDING – oberoende runtimekrav:** selected-Homey-digesten matchar historisk
  evidens, men aktuell panelidentitet/selected Homey måste verifieras under
  separat tillåten gate. `V9_SELECTED_HOMEY_BINDING=PARTIAL` i V25; V26:s POST godkändes av serverns digestgate.
- **PENDING – generationskrav:** filens positiva generation är giltig,
  men POST-handlern jämför inte payload-generation med tidigare generation.
  Lagringens egna monotona generationsnummer ger inte payload anti-replay.
  `V9_GENERATION_CHECK=PARTIAL`; full replay-spärr får inte påstås verifierad.

V23 har redan positionsstödet. Efter korrekt aliasprovisionering behövs ett
nytt lyckat inventeringssnapshot; ordinarie inventeringsväg återläser aliasen.
Inget nytt inventory-anrop eller någon panelkontakt ingår i V25.
De kvarstående riskerna måste lösas eller accepteras uttryckligen för ett
avgränsat diagnostiskt försök; V25 PARTIAL får inte behandlas som allmänt PASS.

### Prioritet 2 – lokal aliasprovisionering

**Status: COMPLETE – V26:s enda auktoriserade POST har passerat.**

Historisk gate och krav för varje eventuellt framtida, separat beslutat försök:

- Granska V25-resultatet och kvarstående identitets-, generations- och
  backupbegränsningar före beslut. Ingen automatisk fortsättning från dry-run.
- Verifiera aktuell panelidentitet, selected Homey och bindningsgeneration
  inom uttryckligen auktoriserad kontroll. Påstå inte anti-replay utifrån
  enbart ett positivt generationsfält.
- Skriv endast godkända markisalias till lokal NVS; bevara Homey-inloggning,
  Wi-Fi och ljusbindningar. Inga Homey-kommandon eller markisrörelser.
- V26 krävde följande separata operatörsmeddelande (nu förbrukat):
  `AUTHORIZE_PATCH076_V26_ONE_LOCAL_AWNING_ALIAS_PROVISION_WITH_DOCUMENTED_RISKS`.
  Exakt panel-IP, payloadidentitet, skrivbudget och verifieringsbudget måste
  vara fastställda. Högst ett POST; ingen redirect eller retry.
- Verifiera sanerat resultat och invänta operatörsobservation. Stoppa vid
  avvisning, fel eller osäker skrivstatus utan ytterligare försök.

### Prioritet 3 – positionsverifiering

**Status: IN PROGRESS – aliaspublicering PASS; positionsverifiering återstår.**

- Kontrollera Solskydds och Balkongmarkisens rapporterade position.
- Kontrollera rätt riktning och fem nivåer i positionsstaplarna.
- Kontrollera säker hantering av okända, stale, felaktiga och otillgängliga
  värden. Behandla inte önskat läge som verifierat fysiskt läge.
- Skilj operatörsobservation från verifierad runtime-/binding-evidens.
  Ingen markisrörelse ingår i verifieringen.

### Prioritet 4 – layoutkorrigering

**Status: COMPLETE – V28 implementation/offlinegeometri och V29 operatörens layoutfoton. Interaktionsrendering är fortfarande PARTIAL och hanteras i prioritet 0.**

- Åtgärda överlappande UPP/STOPP/NED-knappar för Markis.
- Kontrollera att samtliga knappar ryms inom kortet på 480×480-displayen.
- Validera layouten utan att aktivera markisens write path.

### Prioritet 5 – säker styrning

**Status: PENDING – capability-/kommandogranskning och separat auktorisering.**

- Utred och implementera styrning först efter verifierade capabilities,
  kommando-/positionssemantik, korrekt UI och separat uppdrag.
- Bevara fail-closed-beteende och krav på explicit rörelseauktorisering.
- Ingen markisrörelse ingår i den nuvarande planen.

### Bevarade Patch076-beslut och säkerhetsarbete

- **COMPLETE – historisk read-only-evidens:** tre markiser matchade och
  tillgängliga med aliasbindningar. Pilglyphs ersatta med LVGL-linjeprimitiver;
  fokuserat test och ren build PASS, detaljerad fysisk pilverifiering återstår.
- **COMPLETE – positionsunderlag:** sparad V6/V9 capability-inventory och
  riktningsevidens återanvändes i V25. Ingen ny inventoryinsamling behövdes.
  Readback använder enum för Markis och numeriskt 0–1 för de två andra.
- **PENDING – inför framtida styrning:** fastställ/verifiera gettable/settable,
  kommandosemantik, bindning och fail-closed-beteende för den exakta skrivvägen.
- **PENDING:** begränsa samtidiga kommandon, debounce, timeout och felåterkoppling
  innan styrning aktiveras; teknisk granskning och separat auktorisering krävs.
- **PENDING:** verifiera fysisk position utan att förväxla önskat läge med
  rapporterat läge. Inga rörelser ingår i nuvarande plan.
- Awning write path förblir disabled. Det äldre snapshotkontraktet bevarade
  bara aliasmatchning, availability och boolean; V23 bevarar även verifierad
  numerisk capability-data. Detta visar readback-stöd, inte säker styrning.

## Framtida Patch077: Utökad Verisure-integration

- [ ] Fastställ om verifierad read-only Verisure-data finns tillgänglig via projektets godkända Homey-integration.
- [ ] Visa endast bekräftade statusfält och tydlig okänd status.
- [ ] Ingen larmaktivering eller avaktivering utan separat säkerhetsgranskning och auktorisering.

## Öppna tekniska problem

### Första uppstarten efter firmwareflash

**Status:** IN PROGRESS – V23-korrigering byggvaliderad, positiv observation efter V24.\
**Root cause:** V22 verifierade stackoverflow i main och bootstrap_rotat;
otillräcklig stackbudget är den riktade V23-förklaringen. Fullständigt samband
med varje äldre svart-display-/kallstartsfall och långtidsstabilitet är inte bevisat.

**Observation:** Efter appflash och RTS hard-reset observerades svart display och oåtkomlig HTTP-runtime. Efter full strömcykling blev displayen synlig och Homey/snapshot/awing-bindningar fungerade. Detta stärker att uppstartsskillnaden följer resetsekvensen, men fastställer inte rotorsaken.

- [ ] Förbered säker journalinsamling.
- [ ] Läs befintlig runtime journal efter separat auktorisering.
- [x] Analysera reset reason och boot-milstolpar i V22: fem uppstarter och RTC_SW_CPU_RST efter krascher; korrupta backtraces gav ingen exakt felrad.
- [ ] Undersök BSP/LCD/touch-initiering om journalen motiverar det.
- [x] Avgör om en firmwarekorrigering behövs: V23 riktad stackreparation byggvaliderad och installerad via V24.
- [ ] Verifiera uppstartssäkerhet före nästa flash.
- [ ] Genomför stabilitetstest efter nästa flash.
- [ ] Utöka den lokala collector-harnessen för journalendpointen; Patch072-harnessen stöder i nuläget inte denna endpoint. Använd en separat beständig, fail-closed budget på högst två GET och högst 256 journalposter per GET.

Detta problem ska finnas kvar i planen, men offlineutveckling av nya funktioner kan fortsätta parallellt. Gör ingen firmwareändring förrän evidens pekar på en konkret defekt.

### Övrigt

- [ ] Cloud discovery `0x7002`: rotorsak ej bevisad.
- [ ] Två tidigare hosttestfel kvarstår: Favorites-dashboardassertion och Patch059 dashboard polling assertion; `HOST_TESTS=PARTIAL`.
- [ ] Verifiera UI-touch på fysisk panel.
- [ ] Genomför längre stabilitetsprov.

## Säkerhetsprinciper och uppdragsgräns

- Ingen Homey-write utan godkännande och ingen markisstyrning utan verifierat
  kommando. Ingen gissad capability-semantik.
- Skilj offline PASS från runtime PASS; skydda befintliga lokala Git-ändringar.
- Ingen firmwareflash, NVS-skrivning eller Homey-mutation utan separat,
  aktuell och avgränsad auktorisering.
- Bevara befintliga credentials och bindningar; ingen automatisk radering,
  återställning eller reprovisionering av Homey-sessionen.
- Homey-kommunikation följer Internet/Athom-only-policyn; ingen LAN/PAT-fallback.
- Planfiler uppdateras endast efter uttryckligt uppdrag, såsom V27/V28, i de två
  uttryckligen angivna `SCRATCHPAD.md`-filerna, med bevarad tidigare plan.
  Den tidigare sammanslagningen tillät bara dokumentändringar. V28 tillåter
  avgränsad UI-kod, tester och offlinebuild. V30 tillåter riktad renderingsanalys,
  tester och isolerad offlinebuild; ingen panelkontakt, HTTP, reset, serial,
  flash, NVS-operation eller Homey-operation ingår i V30.
- Ingen Git-commit, push, PR eller merge. Ingen ändring av handoff eller filer
  utanför det uttryckliga uppdragets scope. Befintliga ocommittade ändringar bevaras.
- Återanvänd relevant passerad validering när källidentiteten är oförändrad;
  kör inte om tester eller V25 automatiskt för att uppdatera planen.

## Arbetsprocess

ChatGPT bestämmer prioriteringar och uppdrag samt granskar resultat. VS Code Codex implementerar, testar och rapporterar evidens.

Efter varje funktionellt utvecklingssteg:

1. Uppdatera relevanta checkboxar.
2. Lägg till eventuella nya problem.
3. Registrera verifierad status.
4. Markera nästa prioriterade uppgift.
5. Markera inte något som klart utan evidens.

`SCRATCHPAD.md` är en levande plan. Den är inte auktoritativ källa för Git-SHA, säkerhetsregler eller runtimebevis.

## V28 – dashboard och inställningslayout

**Status: COMPLETE – avgränsad UI-implementation/offlinevalidering; samlat RESULT=PARTIAL på grund av två äldre hosttestfel.**

Beslutad visuell sidindelning med oförändrade sex Homey-widgetindex:

| Sida | Kort och placering | Index |
|---|---|---|
| 1 – Favoriter | Solskydd övre vänster; Balkongmarkis övre höger | 1, 2 |
| 1 – Favoriter | Ytterbelysning nedre vänster; Dekorbelysning nedre höger | 4, 5 |
| 2 – Markis | Ett större, centralt Markis-kort | 0 |
| 3 – Verisure | Ett centralt read-only statuskort | 3 |

- Statiska placeringsdata skapar varje widget exakt en gång. Horisontell
  swipe, tre sidindikatorer och normal uppstart på sida 1 bevaras.
- Favoritkort 220×176; Markis-kort 416×320 med tre separata 112×112-kontroller.
  UPP/STOPP/NED är fortfarande disabled och saknar nya kommando-callbacks.
- Verisure visar befintlig modellstatus genom befintlig formatter;
  ingen aktivering, avaktivering eller ny skrivväg.
- Inställningarnas SKÄRM-rubrik tas bort. Sex knappar, inklusive Avancerat,
  visas i en kolumn med 52 pixlars höjd och sex pixlars radavstånd.
  Med feedbackfält används 380 av 392 pixlar; huvudmenyn behöver inte skrollas.
  Avancerat behåller separat undermeny, skrollning och skyddade funktioner.
- Ljusinteraktioner för index 4/5 bevaras. Positionsfärger, inversion,
  femstegsmodell, Homey-bindningar, OAuth, NVS och styrbehörigheter ändras inte.
- V26 är COMPLETE inom sin skrivgate och dess auktorisering är förbrukad.
  Ingen aliasprovisionering ska upprepas.
- V27 offline-rendering är klar och fotoavläsningen är sannolikt förenlig
  med helt uppe; fullständig positionsverifiering kvarstår.
- Ingen fysisk panelkontakt eller flash ingår. Runtime-UI är NOT_RUN.

### V28 – faktisk valideringsstatus

- Layoutgeometri och skapande-/säkerhetsinvarianter PASS: fyra kort på
  Favoriter, ett Markis-kort, ett Verisure-kort, oförändrade index, inga
  geometriska överlapp och inga nya markis-/larmcallbacks.
- UI-modell/dashboardbindning, Patch074 inställningar/store och Patch075
  widgets/positionsmodell PASS. Patch069 readiness och NVS-startup-gate PASS.
- Det första nya layouttestet hade en felaktig funktionsavgränsning i
  testverktyget; korrigerat och omkört PASS. Felet är bevarat utanför Git.
- Äldre Favorites assertion och Patch059 dashboard polling assertion
  återkom vid samma dokumenterade assertions. De är preexisting och
  oförändrade: `HOST_TESTS=PARTIAL`, `PREEXISTING_HOST_FAILURES=2`.
- Ren isolerad ESP-IDF v6.0.1-build från actual Patch073–076/V28-källor PASS.
  Effektiv main-stack 8192; rotation-stack 8192 och statisk polling-snapshot
  bevarade. Partitionstabell oförändrad.
- ELF ESP32-S3/Xtensa och image validation PASS; checksum och imagehash giltiga.
  Firmware 1691792 byte, 400 byte större än V23, inom factory-partitionen
  0x600000 med 4599664 byte kvar.
- Kandidat:
  `~/Downloads/PATCH076_V28_OFFLINE_47lb46hc/build_final/esp32_homey_wall_panel.bin`.
  SHA256:
  `acb75336e760f57b1ed2671b3c8cfcac2b86d96483c416dc737d340eb72a6461`.
- Geometri-PASS är offline-evidens, inte verifierad LVGL-touch eller visning på
  fysisk panel. `RUNTIME_UI_VALIDATION=NOT_RUN`; NVS-backup ej verifierad.
- Nästa gate är separat granskning av exakt kandidat och separat uttrycklig
  app-only-flashauktorisering med dokumenterade risker. Ingen flash är gjord
  eller auktoriserad av V28. Vid avslut av denna offlinegate var V23 installerad;
  V29:s senare app-only-flash redovisas nedan.


## V29 – installerad V28 och operatörsevidens

**COMPLETE – flashgate:** `V29_APP_ONLY_FLASH=PASS`. Exakt en app-only-flash
vid 0x10000 med verifierade skrivdata. V29-auktoriseringen är förbrukad.
Installerad V28 SHA256:
`acb75336e760f57b1ed2671b3c8cfcac2b86d96483c416dc737d340eb72a6461`.
V26 är COMPLETE och dess auktorisering förbrukad; bindningarna ska inte
provisioneras igen.

**COMPLETE – registrerad operatörens foto-/skärmobservation:**
`V29_LAYOUT_OPERATOR_VISUAL=PASS`. Sida 1 visar Solskydd, Balkongmarkis,
Ytterbelysning och Dekorbelysning; sida 2 Markis med separerade knappar;
sida 3 Verisure. Inställningarnas sex knappar syns utan skrollning och
Avancerat kan öppnas. Homey visar ”Ansluten”; båda lamporna visar ”Tänd”.

**IN PROGRESS – observerad interaktionsrendering:**
`V29_RUNTIME_UI_VALIDATION=PARTIAL`.
Operatörens 31-sekunders video visar normal stillastående bild men kraftiga
överlagringar/felplacerade kopior av sidor, texter, ramar och knappar under
swipe samt menyövergångar. Bilddelar kan ligga kvar tills nästa interaktion.
Ingen tydlig firmwareomstart observeras i filmen. Detta är renderingsartefakter,
inte enbart kameraflimmer. Fullständig positionsverifiering enligt V27
återstår; båda markiserna är helt uppdragna enligt operatören.

## V30 – riktad LVGL/RGB-renderingsanalys

**COMPLETE – offlineanalys och tester; RESULT=PARTIAL, grafikfelet olöst.**
`RENDER_ARTIFACT_CAUSE=UNKNOWN`; `RUNTIME_RENDER_FIX=NOT_VERIFIED`.
Ingen firmwarekorrigering eller buffertökning gjordes, eftersom offlinegranskningen
inte avgränsade en konkret sannolik orsak till de observerade artefakterna.

Verifierad käll-/host-evidens:

- Produktionens `panel_ui_refresh()` hoppar redan över page reassert när
  `lv_obj_is_scrolling(pager)` är true. `pager_event()` löser aktiv sida vid
  SCROLL_END. V30:s verkliga LVGL 9.6-hosttest kör ESP-refresh-grenen, tre
  sidanimationer, upprepade refresh samt tids-/positions-/ljusmodelluppdateringar
  och åtta menyöppningar/stängningar. 137 partiella bilder jämförs pixelvis
  med full omritning och matchar; pager-offset bevaras under animation.
- Flush-sänkan i hosttestet är synkron CPU-kopiering. Ingen RGB/DMA, faktisk
  touch, VSYNC, PSRAM-bandbredd eller FreeRTOS-samtidighet testas där.
  Favorite-readiness stubbades fail-closed utan kommando- eller Homey-anrop.
- V23/V28-manifest visar identiska `secure_bootstrap_esp.c` och
  `sdkconfig.defaults`: displaystart, rotation-task, 8192-stackar och den
  statiska snapshoten ändrades inte av V28. V23:s fullständiga UI-källkopia
  saknas i det bevarade V23-underlaget; UI-jämförelsen får inte kallas komplett.
- Faktisk V28/V30-konfiguration: en 460800-byte RGB565-framebuffer i PSRAM,
  480×160 LVGL-partialbuffer och lika stor software-rotationbuffer (153600
  byte vardera), samt två interna 20-raders bouncebuffertar (38400 byte totalt).
  Ingen avoid-tearing/full-refresh/direct-mode. RGB-drivern kopierar
  partialbuffer synkront innan flush-ready; ingen verifierad tidig bufferåteranvändning.
- CPU uppdaterar den framebuffer som bounce/scanout samtidigt läser.
  Tearing och bounce-deadline/PSRAM-belastning är kvarstående hypoteser;
  de har inte separerats eller visats orsaka filmens kvarliggande bildfel.
  BSP använder faktiskt bouncebuffer men portens bb_mode-flagga är false;
  med nuvarande partialmode används ingen väntsemafor, så detta bevisar inte
  en felaktig flush-completion i den installerade konfigurationen.
- Löpande Homey-/tids-/inaktivitetsuppdateringar och LVGL-timerhanteraren
  använder samma rekursiva displaylås. Initieringen i `display_init()`
  registrerar dock perf-callbacks och ändrar input-inställningar före låset:
  `DISPLAY_LOCKING=FAIL` för en fullständig låsaudit. Denna äldre startupbrist
  är inte verifierad orsak till senare interaktionsartefakter och ändrades inte.
- `lv_refr_now()` används vid power/wake-transitioner, inte i varje normal
  refresh. Wake-vägen har egen page reassert; dess samband med filmen är okänt.

Faktisk V30-validering:

- Real-LVGL render-test PASS; UI-modell/dashboardbindning, Patch074 settings,
  Patch075 widgets och V28 layout/säkerhetsinvarianter PASS.
- `UI_HOST_TESTS=PASS`; `HOST_TESTS=PARTIAL` och
  `PREEXISTING_HOST_FAILURES=2`. Tidigare Favorites/Patch059-fel återanvänds
  med oförändrad relevant källidentitet; ingen ny full hosttestkörning.
- Första hostkörningen med 64 KiB-pool föll med null text under layout.
  Arm64-hostens objekt/pointerstorlekar skiljer sig från Xtensa32; hostpoolen
  ökades till 256 KiB och testerna passerade. Firmwarepoolen är oförändrad.
  Detta är inte verifiering av panelens lediga runtime-heap. Fel och backtrace
  är bevarade utanför Git.
- Ren isolerad ESP-IDF v6.0.1-build PASS; ELF/image/checksum/hash PASS.
  Firmware 1691792 byte; factory 0x600000, 4599664 byte kvar.
  Statisk DIRAM-användning 225077 byte och länkarens återstående DIRAM
  116683 byte, oförändrat mot V28. Detta är inte ledig heap efter task-/BSP-start.
  Inga task-/LVGL-/bounce-/framebufferbudgetar ökades. Runtime-RAM ej mätt.
- Produktionskällorna är oförändrade. NVS auto-erase disabled, awning writes
  disabled, widgetindex/bindningar/callbacks/positionsmodell och layout bevarade.
- Evidens/build: `~/Downloads/PATCH076_V30_OFFLINE_2i8y5u_4/`.
  Binär: `build/esp32_homey_wall_panel.bin`.
  SHA256: `6979747ad729bc35c12a4008e67d970a55da5b584a214371987d950f1b63e6e2`.
  Detta är ett ombygge av oförändrad firmware, inte en render-fix-kandidat.
- Båda SCRATCHPAD uppdateras identiskt. Ingen flash/panelkontakt, serial,
  reset, NVS/Homey-operation eller Git-publicering. Handoff oförändrad.

**PENDING – en riktad diagnostisk observation:** förbered en enda tidskorrelerad
insamling av RGB bounce/VSYNC-deadlineutfall, flush/refresh-tider och
LVGL-minnesfel under en kort swipe/menysekvens. Instrumentering/transport,
privacy och budget måste först fastställas offline. Varje eventuell fysisk
insamling eller diagnostikflash kräver separat uppdrag och uttrycklig gate;
ingen sådan åtgärd är nu auktoriserad. Ingen markis-, ljus- eller larmaktivering.
En ny flash av oförändrad V30-binär är inte motiverad som grafikfix.

## Gemensam plan och fortsatt arbetsflöde

Sammanslagen 2026-10-11 för `PATCH076_SCRATCHPAD_TWO_WORKTREE_MERGE`, med den
ursprungliga Product Scratchpad som grund. Patch073–075-uppgifter,
arkitekturbeslut, releasekandidater, journalplan, Verisure-idéer och framtida
styrningskrav är bevarade. Historiska prioriteringar har ersatts av en enda
aktiv Patch076-prioritering; äldre evidens gäller sin ursprungliga gate.

Development-worktreens `SCRATCHPAD.md` är aktiv arbetsplan under Patch073–076.
Vid framtida uppdateringar ska båda filerna kontrolleras och eventuell skillnad
rapporteras. Ingen automatisk synkronisering eller ändring av den andra
katalogen får ske utan uttryckligt uppdrag. Sammanslagningen och V27:s
uttryckligt auktoriserade dokumentuppdatering skriver identisk plan till båda.

`NEXT_ACTION=PATCH076_V42_V41_APP_ONLY_FLASH_GATE`

V33:s diagnostikflash och V36:s V35-flash är genomförda; båda auktoriseringarna
är förbrukade. V37-kandidaten har endast offlinevaliderats. Nästa steg är separat
V38 app-only-flashgranskning med dokumenterade risker; ingen fysisk operation är
nu auktoriserad. V34-insamling är inte verifierat utförd och dess gamla fönster/
auktorisering får inte återanvändas. V26 är COMPLETE och får inte upprepas.
Tidigare kraftiga grafikstörningar har upphört enligt operatören; rotorsak är ej
verifierad. V37 avser kort bakgrundsflimmer/Favorites mellanlägen, inte RGB-timing.
V27:s positionsmodell och övriga prioriteringar bevaras.

## V31 – RGB-observerbarhet och diagnostikplan

**COMPLETE – offlinegranskning; RESULT=PARTIAL.** Installerad V28 saknar direkt
mätning av EOF-underskott och automatiska DMA-omstarter. ESP32-S3 har ingen
underrun-händelse i denna HAL; avsaknad av fellogg utesluter inte RGB-fel.
VSYNC-jitter och LVGL-flushlatens är indirekta mått. Drivrutinen kan starta om
DMA vid för få bounce-EOF även när RESTART_IN_VSYNC är avstängd.
Flash/cache-exponeringen är verifierad i kod/config, men inte som orsak till
operatörens bildförskjutningar. V28:s loggar räcker inte för full A/B/C-separation.
Plan: `~/Downloads/PATCH076_V31_PLAN_jcqb9994/RESULT.md`.
Ingen källändring eller fysisk operation utfördes i V31.

## V32 – begränsad RGB-diagnostikinstrumentering

**COMPLETE – implementation och offlinevalidering; runtime NOT_VERIFIED.**
Detta är en diagnostikkandidat, ingen funktionell grafikfix. ACTIVE_ISSUE är
fortfarande `LVGL_INTERACTION_RENDER_ARTIFACTS`; rotorsaken är UNKNOWN.

- ESP-IDF v6.0.1 RGB-original SHA256:
  `da391ef23c849e492db12bba8a19ab81934915396b9ee78d148e26a37a4edaa9`.
  Hashspärrad generator gör enbart markerade, reversibla tillägg i byggkatalogen.
  CMake ersätter exakt en RGB-källfil i det verkliga esp_lcd-målet. Originalets
  övriga källor, privata headers, beroenden och callbacks bevaras. Installerad
  IDF och BSP är oförändrade; ingen konfliktande RGB-implementation länkas.
- Mätpunkter: bounce-EOF; faktisk/förväntad EOF vid beslutet; EOF-underskott;
  separata restart-begäran/beslut/utförda reset-start-vägar; VSYNC-intervall;
  sampled bounce-arbete (var 64:e EOF). Första två VSYNC särredovisas som startup.
  Ett underskott plus begärd restart räknas som ett beslut, med båda orsaksbitarna.
  Utförd väg är inte bevis för lyckad fysisk återhämtning eller hårdvaru-underrun.
- ISR: inga loggar, allokeringar, mutexar eller flashskrivningar. IRAM-kod,
  interna DRAM-räknare och korta ISR-säkra spinlocks. Systemtimern är verifierad
  i IRAM. ISR-overhead redovisas som instruktioner/operationer; målplattformens
  mikrosekundkostnad, cache-/busstall och observereffekt är ännu inte mätta.
- Befintliga flush-/refresh-/lock-/swipe-loggar återanvänds. Meny- och page-end-
  händelser får monotona mikrosekundmarkörer. Lågprioriterad statisk task ger
  högst en sammanfattning/sekund, högst 900 efter start, därefter vilande.
  Volatil 16-events kö, högst fyra events per rapport; tappade events räknas.
  Framtida fångst måste ske inom rapportfönstret, utan implicit reset/rearm.
- Minnesmått: internt free/minimum och PSRAM free. LVGL builtin malloc/realloc-
  fel observeras med wrappers som bevarar originalanrop och returvärden.
  Befintlig heap-failure-callback behåller ägarskapet och utökas med räknare;
  den generiska heap-observationen börjar först när originalet registrerar hooken.
- Tid och resultat för exakt befintliga diaglog partition-write-anrop registreras
  i RAM. Inga nya flash-/NVS-skrivningar. Detta mäter inte alla flashoperationer
  eller exakt cache-disable-tid. FLASH_INTERVAL_MEASUREMENT=PARTIAL;
  MEMORY_MEASUREMENT=PARTIAL för täckningsgränserna.
- Displaytiming/buffertar/refreshstrategi, XIP-config, main/rotation-stack 8192,
  snapshotägarskap, layout/index/bindningar och Homey-/ljusfunktion bevaras.
  NVS auto-erase och awning write path förblir disabled.
  V30:s startup-displaylåsbrist är oförändrad och inte bevisad flimmerorsak.

Validering: instrumenterings-/driverbeslutsekvivalens PASS för båda restart-
policygrenarna; wrappers/EOF-sampling/eventkö/rapportbudget PASS; real-LVGL
137 bilder PASS; UI/bindning, settings, V28-layout och journal/invarianter PASS.
HOST_TESTS=PARTIAL; två tidigare Favorites/Patch059-fel är PREEXISTING, inte PASS.
Ren isolerad build, ELF/image, partitionstabell och git diff --check PASS.
Första byggförsökets privata include-sökvägsfel är korrigerat och bevarat i logg.

Statisk intern RAM-delta +5496 byte (data +496, BSS +4208, IRAM +792).
DIRAM used=230573; linker remaining=111187. Ingen extra reserverad PSRAM.
Runtime-heap/stack och ISR-beteende är NOT_VERIFIED. Binärdelta +3072 byte.
Binärstorlek 1694864 byte; factory 0x10000/0x600000, 4596592 byte kvar.
Firmware SHA256:
`0ff7fedb6e825dee0a91e2d3fe8a4ed38eb35e048082971f9b1bf2d3e8da734b`.
Evidens: `~/Downloads/PATCH076_V32_OFFLINE_2os9j0ue/RESULT.md`.
Kandidat: `build_final/esp32_homey_wall_panel.bin` i samma katalog.
Ingen flash, serial, reset, panelkontakt, NVS/Homey-operation eller publicering.

**PENDING – separat diagnostikflashgate:** verifiera kandidatidentitet och risker
innan separat uttryckligt godkännande för exakt ett app-only-försök. Därefter
krävs ytterligare separat auktorisering för högst en 90-sekunders RX-only fångst
med swipe/menyer, automatisk stängning och ingen avsiktlig reset eller retry.
Portöppningens DTR/RTS-risk måste ingå. Ingen enhetskontroll eller inställnings-
ändring. Diagnostiken kan störa timing; normal mätning utesluter inte PSRAM-
bandbredd, fysisk korruption, tearing eller oinstrumenterade samtidighetsfel.
V30:s oförändrade ombyggnad ska inte flashas.

## V35 – intermittent positionsstatus och verifierad handoff-rättning

**Aktuell prioritering: IN PROGRESS – färska markispositioner över tid.**
Denna sektion uppdaterar äldre väntande V33/RGB-gates; historiken ovan bevaras.

**COMPLETE – registrerad V33-flashevidens:** exakt V32 app-only flash och
hashverifiering PASS. Binär SHA256
`0ff7fedb6e825dee0a91e2d3fe8a4ed38eb35e048082971f9b1bf2d3e8da734b`.
ST7701 2.0.3 är accepterad med dokumenterat V33B-undantag; worktree2.0.2~2
normaliserades inte. Historiskt V33-källidentitetsfel kvarstår som historisk FAIL.
V33-auktoriseringen är förbrukad. Ingen V34-insamling är verifierat utförd.

**COMPLETE – registrerad operatörsobservation, inte instrumenterad mätning:**
Inga upprepade omstarter observerades. Homey visade först Ansluter, anslöt sedan;
alla fyra förstasideswidgets visade korrekt status. Solskydd och Balkongmarkis
blev senare båda Okänd, och båda har nu återfått korrekt positionsstatus.
Ytterbelysning och Dekorbelysning har enligt operatören visat korrekt status hela
tiden. Grafikflimret har upphört enligt operatören. Grafikorsaken är UNKNOWN;
V32-instrumenteringen får inte automatiskt tillskrivas en grafikfix. Återöppna
inte RGB-utredningen utan ny evidens. Lampornas observation bevisar inte färsk
GET-respons exakt vid markisernas statusövergång.

**COMPLETE – V35 käll-/hostevidens:** gemensam dashboard-snapshot har120000ms TTL,
färsk vid exakt gränsen, stale därefter. Lokal poll1s; periodisk API-kö vid60s,
busy/not_ready defer5s och queued/failed cooldown30s. Periodiska workerfel får
inte boot-workerretry, men schedulern fortsätter köa när prerequisites tillåter,
även efter ERROR. Bootretry5/10/20/30s,42960/120/240/300s. Misslyckad transport/
JSON-publicering behåller aktiv snapshot och dess gamla tidsstämpel. Efter TTL
blir båda positionerna Okänd; ny giltig publicering återställer dem. Strukturellt
lyckad respons med saknade/ogiltiga capabilities kan däremot göra positionerna
Okänd omedelbart trots fresh snapshot. AVAILABLE och giltig position är olika
villkor; explicit unavailable visas Otillgänglig. Favoritlampor har separat
store/modelägarskap utan denna TTL, även om inventoryhämtningen delar Devices-
responsen. Homey Ansluten är inte garanti för färska positioner.

**COMPLETE – reproducerad funktionell bug och minimal rättning:**
`poll_homey_dashboard_if_due()` skrev dashboardstate före displaylåset. Vid
låstimeout förlorades state→model-handoff: nästa samma snapshot klassades som
oförändrad, så UI kunde bli kvar på Okänd efter färsk data eller behålla gammal
position efter stale. Exakt originalfunktion fallerar riktat LVGL-regressionstest.
State-commit flyttades till efter lyckad befintlig50ms låsning. State och modell
uppdateras nu tillsammans; misslyckat lås lämnar övergången väntande till nästa
poll. Inga API-anrop/intervall, TTL, alias/NVS-format, stackar eller dataformat
ändrades. Denna bug är verifierad offline; dess medverkan i operatörens faktiska
händelse är INTE verifierad. Den ursprungliga runtime-utlösaren för Okänd är
fortfarande UNKNOWN; gemensam freshness/pollning är huvudmisstanke.

Validering:28 riktiga parser→alias→snapshot→model→LVGL-kontroller PASS, inklusive
alla fem positioner, transient fail, TTL-gräns, ny data, båda widgets, partial/
ogiltig capability, fel Homey-identitet, fel alias, readiness och låstimeout på
stale/färsk övergång. Ursprungliga fel bevarade. Exakta schedulerfunktioner PASS.
Snapshot/UI/bindningsregressioner, V28-layout och137 real-LVGL-bilder PASS.
HOST_TESTS=PARTIAL; de två tidigare Favorites/Patch059-felen är inte lösta.
Runtime-aliasidentitet/NVS-backup är fortfarande NOT_VERIFIED; ingen NVS lästes.

Ren isolerad ESP-IDF v6.0.1-build och ELF/image PASS. Explicit dependency-lock
OCH komplett managed-bundle från installerad V32 återanvändes, med ST7701 2.0.3
som dokumenterat val. Ingen oavsiktlig resolution eller worktree-normalisering.
7689 build-inputfiler manifesterade;969 projekt/managed-källor verifierade.
Intern statisk RAM-delta0 mot V32; DIRAM230573, linker remaining111187.
Binär1694864byte, storleksdelta0; app0x10000/0x600000,4596592byte kvar.
V23-stackar8192, snapshotägarskap, V28-layout, numerisk riktning, ljuscallbacks,
NVS auto-erase disabled, awning writes disabled och V32 RGB-instrumentering bevaras.
Den separata startup-displaylåsbristen är oförändrad, inte blandad i V35.

Kandidat: `~/Downloads/PATCH076_V35_OFFLINE_y9pb7_b4/build/esp32_homey_wall_panel.bin`.
SHA256: `3c04daad5a1083c398934f5031174a5fa58a11b7918711f2ddbdb9d71852b0a5`.
Evidens: `~/Downloads/PATCH076_V35_OFFLINE_y9pb7_b4/RESULT.md`.
Git diff --check PASS; båda SCRATCHPAD identiska. Handoff/Current State oförändrade.
Ingen panelkontakt, API-operation, serial/reset/flash, NVS, provisionering,
Homey-mutation, markisrörelse eller Git-publicering utfördes i V35.

**Historisk gate – därefter genomförd i V36:** separat app-only-flashgranskning av exakt V35-
kandidat före ny fysisk auktorisering. Runtime-rättningen är NOT_RUN. Verifiera
inte grafikorsak eller Homey READY med detta offline-PASS. Ingen automatisk flash.
Om runtime-symptomet kvarstår behövs senare en enda separat godkänd passiv
read-only-loggfångst av periodic queue/age, attempt stage/error/status och snapshot-
publicering, tidskorrelerad med Okänd→position; inga extra Homey-requests eller
reset. V34:s gamla tid eller auktorisering får inte återanvändas.


## V36 – installerad V35 och minst sex minuters operatörskontroll

**COMPLETE – flash-evidens:** app-only flash PASS, ett försök med verifierade
data vid0x10000. V36-auktoriseringen är förbrukad. Installerad V35 SHA256:
`3c04daad5a1083c398934f5031174a5fa58a11b7918711f2ddbdb9d71852b0a5`.
Evidens: `~/Downloads/PATCH076_V36_PREFLIGHT_a3rw3sgm/RESULT.md`.
Den första korta rapportens ofullständiga observation kompletteras av operatörens
V37-uppdrag: minst sex minuter med korrekt status för båda markiser och lampor,
inga upprepade omstarter, inga tidigare kraftiga grafikstörningar, swipe mellan
alla tre sidor och fungerande öppning/stängning av inställningar. Dessa sex
kontrollområden är PASS som operatörsobservation, inte instrumenterad mätning.
Långtidsstabilitet, exakt runtimeutlösare för V35 och grafikrotorsak är ej bevisade.
Ingen V34-insamling är verifierat utförd; gamla fysiska auktoriseringar gäller inte.

## V37 – atomisk Favorites och förändringsstyrd presentation

**COMPLETE – implementation/offlinevalidering; RESULT=PARTIAL.**
**PENDING – fysisk renderingsverifiering:** ingen flash eller panelkontakt i V37.

Ny operatörsfilm, granskad i ChatGPT och inte antagen tillgänglig lokalt, cirka
7,8s:0–4,0s korrekta namn och Släckt; cirka4,1s Belysning1/2, därefter Okänd;
cirka5,3s korrekta namn åter, cirka5,5s Släckt åter. Homey Ansluten, positioner
oförändrade, ingen observerad omstart/manövrering. Namn/status återkommer inte
nödvändigtvis i samma bildruta. Tider uppskattade, inte firmwaretidsstämplar.
Kvarstående kort flimmer vid bakgrundsuppdatering är operatörsobservation.

Verifierade käll-/hostmekanismer:

- Favorites-parsern clearade publikt tillstånd innan nästa snapshot byggdes.
  En läsare under cJSON-allokering ser UNVERIFIED och fallbacknamn/Okänd för båda
  ljuswidgets. Exakt originalkod fallerar deterministiskt interleavingtest;
  kandidaten passerar. Detta stämmer med filmen; faktisk runtimekausalitet LIKELY,
  ingen logg bevisar att just denna parserlucka fångades i videon.
- Parsern bygger nu separat och publicerar färdigt tillstånd under befintlig
  spinlock. Senaste kompletta presentation kvarstår under konstruktion, men
  ljuskommandobehörighet återkallas omedelbart. Volatil epoch spärrar äldre
  kandidater efter clear/revoke/ny parse. Riktig parse-/schema-/identitetsogiltighet
  visas fail-closed; inget debounce och ingen TTL-förändring.
- Homey-identitetsbyte tömmer gamla Favorites innan aliasinvalidation/transport.
  Samma identitet behåller read-only-presentation, men authority återkallas.
  Exakt pre-transportkod testas med syntetiska identiteter; ingen API-operation.
- UI jämför aktuell text, färg, typsnitt, alignment, bakgrund och sidpunkter
  innan motsvarande LVGL-skrivning. Refresh återställer inte redan rätt pager-
  offset och påverkar inte pågående swipe. LVGL:s egna flagg-/invalidationregler,
  direkt UNKNOWN, touch, verkliga förändringar och power/wake bevaras.
- Rotationens befintliga pending-setter synkroniserar klickbarhet även när
  pendingvärdet och presentationen är oförändrade. Detta återställer behöriga
  ljusinteraktioner efter atomisk ompublicering utan att begära omritning.
  Händelseguard kontrollerar auktoritet direkt; invalid data tillåter inga kommandon.
- Ingen visuell debounce är motiverad av de reproducerade mekanismerna:
  DEBOUNCE_IMPLEMENTED=NO, väntetid0ms. Ingen pending presentationskö tillkom.

Källändringar: `panel_homey_favorites.c`, `panel_ui.c`, `athom_cloud_client.c`
under `components/secure_bootstrap/`, två nya V37-hosttestfiler och båda SCRATCHPAD.
Inga andra tidigare produktionsändringar ändrades; V23-stackar8192, V35-handoff,
V28-layout/index, ljuscallbacks, alias/NVS, inverterade positioner, V32 RGB och
awning write disabled bevaras. Startup-displaylåsbristen från V30 är oförändrad.

Mätmetod: samma verkliga LVGL-display, fem refresh/readiness/connection/time-
sekvenser. Textskrivningar145→0, räknade stilskrivningar105→0, scroll_to_x5→0,
INVALIDATE_AREA-events40→0. Explicit panel_ui_refresh-anrop5→5, lv_refr_now0→0;
ingen fysisk flush-/RGB-kostnad mättes.20 identiska verkliga Favorites-publiceringar
har0 text-/stil-/scrollskrivningar och0 invalidationevents. En faktisk ljusändring
ger1 statustext och2 ikonfärger för enbart berörd widget.

Tester PASS: atomisk publikation, båda ljusnamn/status, omedelbar authority-
återkallelse, clear/revoke under parse, malformed/verklig UNKNOWN/återkomst,
Homey-identitetsgräns, positionsändring/stale, klocka, swipe, menyer och dim/off/
wake med brightnesscallback; V35:s28 checkpoints och scheduler;137 real-LVGL-
bilder; UI/layout/settings/widget/snapshot/inventory/readiness/light-dispatch.
Legacy Favorites-suiten fallerar fortsatt första dokumenterade assertionen;
vid separat karakterisering finns en senare dim/wake-assertion med samma gamla
förväntan på model-changed. Den fallerar även på baseline och ändrades inte.
Övriga Favorites-kontroller passerar; full legacy-suiten rapporteras inte PASS.
Patch059-suiten fallerar oförändrad. HOST_TESTS=PARTIAL,
PREEXISTING_HOST_FAILURES=2 (suitefel, inte två individuella assertions).
Äldre light-dispatch-runner saknade V32 include-path; körd med explicit host-
include-path utanför Git PASS. Ursprungliga fel/loggar bevaras.

Ren slutbuild ESP-IDFv6.0.1 PASS, ELF/image/checksum/hash PASS. Explicit komplett
V35/V32 dependency-lock och managed-bundle oförändrade: ST77012.0.3 med samma
V33B-undantag; worktreens2.0.2~2 oförändrad.7691 projekt/managed-inputposter,
969 kompilerade projekt/managed-källor och4177 faktiska kompileringsinputs/headers/
konfigurationsfiler manifesterade. Slutlig plantext kopierades efter release-
builden och särredovisas; ingen kompilerad produktionskälla ändrades efter build.
Original-IDF LCD136-filers manifest oförändrat; RGB-genererad källa samma som V32.
Intern statisk RAM230589byte, delta+16 mot V35 (8-byte epoch plus alignment).
Länkarkvar111171byte, PSRAM-delta0. Runtimeheap/stack/timing ej uppmätta.
Binär1695648byte, delta+784; app0x10000/0x600000,4595808byte kvar.
Kandidat:
`~/Downloads/PATCH076_V37_OFFLINE_cowas_g_/build_release/esp32_homey_wall_panel.bin`.
SHA256: `0af38719597cbb9020fcfda03149c21d945b555fa8475dea386b2a068ed5f628`.
Evidens: `~/Downloads/PATCH076_V37_OFFLINE_cowas_g_/RESULT.md`.
NVS-backup ej verifierad. Fysisk flimmerförbättring NOT_RUN; host-PASS bevisar inte
RGB-stabilitet. Ingen panelkontakt, flash, serial/reset, NVS/Homey, rörelse,
provisionering, Git-publicering eller handoffändring utförd i V37.

**Historisk gate – genomförd i V38:** separat V38 offline-app-only-flashgate för exakt
V37-kandidat, med återanvändning av validering och verifiering av käll-/byggidentitet.
Stoppa före fysisk operation och begär separat bounded auktorisering om gaten
passerar. Ingen flash eller tidigare fysisk auktorisering är nu tillåten.


## V38–V40 – installerad V37 och verifierat bootinventeringsfel

**COMPLETE – flash/evidens:** V38 installerade V37 app-only med PASS. Den
auktoriseringen är förbrukad. Runtime efteråt: Ansluter, fyra Okänd widgets,
korrekta namn, snabbt synkroniserad klocka. Detta verifierar inte Homey READY.
V39 fann ingen konkret V37-regression och utförde endast offlineanalys.
V40 utförde ett enda separat auktoriserat read-only diagnostiskt GET; auktoriseringen
är förbrukad. Runtime homey_connection_error/homey_data_state=error, boot_auto
sequence1/attempt1, devices, avslutad42592ms, error28684, finalHTTP0 men rawHTTP200,
elapsed23246ms; ingen snapshot eller verifierad Favorites-publication.
Aliaslagret stabilt, strukturellt giltigt och Selected-Homey-matchande; tre markisalias
bevarade. Ingen ny avslutad inventering registrerad. Operatören såg samtidigt Ansluter.

**Kompletterad uppstartsevidens:** enligt operatörens verifierade underlag är
runtime_id samma i första Live Status och V40. Runtimeinitialiseringen genererar
runtime_id; kontinuitet mellan observationerna är därför SUPPORTED. V40-rapportens
uppgift att tidigare runtime_id saknades är överspelad i denna analys; historisk
rapport ändras inte. Ingen ytterligare panelbegäran utförd.

## V41 – begränsad read-only recovery efter ofullständigt devices-svar

**COMPLETE – offlineimplementation och validering.** ROOT_CAUSE för retrygapet
VERIFIED; TRANSPORT_ROOT_CAUSE=UNKNOWN. 28684=0x700C=ESP_ERR_HTTP_INCOMPLETE_DATA.
HTTP200 betyder inte komplett respons; TCP_CONNECT_FAIL är inte handshakebevis.
Generell transientklassning saknar detta fel, första försöket slutar i ERROR och
utan initial snapshot startar inte den vanliga periodiska återhämtningen.
Ofullständig body lämnas aldrig vidare för parsing/publication. Runtime READY
kräver fortsatt ESP_OK och inventory_complete. Diagnostic_set återställer slutlig
HTTP-status till0, medan raw transport behåller mottagen200.

Minimal produktionsändring endast athom_oauth_runtime.c: en separat volatil budget
per boot-auto-jobb tillåter högst ETT extra försök när error=INCOMPLETE_DATA och
stage=inventory_devices;401/403 undantas. Budgeten återställs inte inom jobbet.
Efter första felet används befintlig backoff5s (senare5/10/20/30s utifrån redan
befintligt attemptnummer;429-policy oförändrad). Det specifika extra försöket går
via befintlig cached-session read-only-inventering, med Selected-Homey/session/URL-
och aliasidentitetskontroller, utan ny discovery, OAuth-login eller sessionpersistens.
Ett andra ofullständigt devices-svar avslutar i ERROR; ett komplett verifierat svar
återställer READY. Annan befintlig transientpolicy är oförändrad, inklusive dess
befintliga retrybeteende. Ingen generisk INCOMPLETE_DATA-transientklassning infördes.

HTTP-klientens befintliga lifecycle behålls: perform-fel stänger transporten,
requestens headers/user-data rensas, ofullständig body nollställs och frigörs.
IDF6.0.1 close återställer state till INIT före transport-close, och nästa perform
förbereder ny request. Ingen evidens motiverar ny handle/cleanup-policy. Faktisk
socket/heapåterhämtning på panelen är ännu NOT_VERIFIED.

Hostreproduktion före fix: en devicesförfrågan, ERROR,0 publicerade snapshots,
förväntad recovery-assertion FAIL. Efter fix passerar samma faktiska extraherade
worker/count_collection/inventory/readiness-kedja: första fel/andra framgång;
två fel/ERROR; väntetid före nytt försök; ingen snapshot under väntan; frigjorda
hostresurser; inget write; nytt jobb med ny budget; timeout mellan två incomplete-
fel återställer inte budget; manual/periodic policy oförändrad; identitetsbyte och
aliasfel förblir fail-closed. Policytest täcker övriga origins,401/403,fel stage/null,
OOM/schema/sizefel. Nätverk, klocka, queue och publication är hostdubblar; detta
verifierar inte fysisk HTTP/ISR/RGB-runtime.
V35/V37, inventory gate, scheduler-worker, transportpolicy, OAuth, snapshot,
Favorites readiness, Favorites transport/FIN, light dispatch och read-only/privacy/
429-invarianter PASS (15 relevanta regressionssviter plus riktade V41-testet).
HOST_TESTS=PARTIAL; två dokumenterade äldre Favorites/Patch059-fel kvarstår och
har inte uppgraderats till PASS. Oförändrade137 LVGL-bilder återanvänds som tidigare
offlineevidens; ingen UI/RGB-kod ändrad. Ingen debounce eller grafikfix i V41.

Rent isolerat ESP-IDF6.0.1-bygge PASS; esptool5.3.1 image/ELF PASS. Explicit lock
från V37/V35/V32 bevarad; ST7701 exakt2.0.3 och dess kompilerade managed-källor
matchar V37. Development managed2.0.2~2 är det redan dokumenterade undantaget;
ingen ny oavsiktlig resolution.969 kompilerade projekt-/managed-källor och4171
käll-/header-/genererade inputs manifesterade;7693 inputfiler oförändrade under
bygget. RGB-genererad källa och original-IDF LCD136-filers manifest oförändrade.
Main/rotation8192byte bevarade. Länkat internt RAM-delta0,PSRAM-delta0;111171byte
DIRAM kvar. Två nya små worker-lokaler tillkommer; faktisk stack high-water ej mätt.
Binär1695888byte (+240 mot V37), app0x10000/0x600000,4595568byte kvar.
Kandidat: ~/Downloads/PATCH076_V41_OFFLINE_rfeh_x0l/build/esp32_homey_wall_panel.bin
SHA256: a3a3a53d32b064cbe90a6d4d6cce649f154ccb913e4096b268a4ac019f44d439
Evidens: ~/Downloads/PATCH076_V41_OFFLINE_rfeh_x0l/RESULT.md

**PENDING – nästa enda steg:** separat V42 offline-app-only-flashgranskning av
exakt V41-kandidat. Ingen fysisk auktorisering finns. Runtime recovery och orsaken
till det ursprungliga ofullständiga svaret är inte verifierade. NVS-backup ej
verifierad. Ingen panelkontakt, Homey-request, flash, serial/reset, NVS-operation,
provisionering, rörelse, Git-publicering eller handoffändring utförd i V41.

<!-- SCRATCHPAD_UPDATE_2026_10_14_V52D_AWNING_BLINK_BEGIN -->
## Aktuell komplettering 2026-10-14 – V52D installerad; nästa UI-uppdrag planerat

**Denna daterade status kompletterar och har företräde framför äldre `NEXT_ACTION`- och
"aktuell status"-rader ovan, vilka behålls oförändrade som historik.** Den är
operatörs- och rapportbaserad; den ändrar inte Git- eller runtime-evidens i efterhand.

### V52D – genomfört och observerat

- **COMPLETE – offline implementation/build:** V52D byggd i isolerad V52B-kopia.
  R1 (kvarhållen meny-skrollposition) är reproducerad och korrigerad i tidigare
  LVGL-hostregression. R2 (accepterad sparbegäran utan aktiv resultatpublicerare)
  reproducerades på V52B och korrigerades i V52D. Readiness blir true först efter
  att `rotation_task` faktiskt nått completion-publiceraren med lyckat displaylås;
  annars avvisas begäran före köskrivning/generationsökning och stängbar ERROR visas.
  R2-fixen ändrade endast `components/secure_bootstrap/secure_bootstrap_esp.c`.
  Inga nya Homey-kommandon, NVS-policyer eller RGB-driverändringar ingår.
- **COMPLETE – V52/V52B UI offline:** all synlig användartext i granskade vyer minst
  22 px (font `homey_panel_font_22`); hamburger, X och tillbakapil har enhetliga
  52x48-tryckytor och 26x21-ikoner. Hjälp, Sparar..., Sparat och fel använder samma
  modala dialog med mörk overlay. Bakgrundstouch blockeras. SAVING stängs inte av
  användaren; SAVED/ERROR kräver bekräftat utfall respektive fel. Detta gäller de
  fem lokala panelinställningarna; Wi-Fi använder HELP/ERROR. Ingen ny markis-write.
- **COMPLETE – firmwarebuild/image:** ESP-IDF v6.0.1 och låsta beroenden, inklusive
  ST7701 2.0.3. V52D-app 1 687 360 byte; statisk DRAM/IRAM/PSRAM-delta 0 jämfört
  med V52B. Firmware SHA256:
  `5c54c5412174ec5035dc34eafc5125c4de7c14f9b9d13c65624a9eebfe73d027`.
  ELF SHA256:
  `600473dd4da955084a10367baa96a1e56eea63745c446179d64517cb3997471c`.
  Isolerad kandidat/evidens:
  `~/Downloads/PATCH076_V52D_OFFLINE_8gby5mi_/`.
- **COMPLETE – operatörsutförd app-only flash:** ESP32-S3, app vid offset 0x10000;
  esptool 5.3.1 rapporterade `Hash of data verified` efter 1 687 360 byte och
  initierade hard reset. Ingen avsiktlig bootloader-, partitionstabells- eller
  NVS-skrivning genom flashkommandot. Denna flashauktorisering är förbrukad.
- **COMPLETE – första fysiska operatörskontroll, 6/6:** panelen startar och visar
  dashboard; Homey visar READY/Ansluten; alla fyra förstasideswidgetar visar rätt
  värden; hamburgermenyn öppnas; textstorlek och navigationsikoner ser bra ut;
  Avancerat öppnas, skrollas och stängs. Detta är operatörsobservation, inte en
  instrumenterad runtime-/minnesmätning.
- **FORTFARANDE ÖPPET:** återkommande RGB-helskärmsflimmer finns kvar även i
  V52D. Rotorsak UNKNOWN. V51C/V51D-spåret är avslutat som BLOCKED under
  oförändrat diagnostikkontrakt. Flimret får inte tillskrivas V52D:s modaler
  eller automatiskt utlösa en ny diagnostik-/flashserie.
- `HOST_TESTS=PARTIAL`: två äldre Favorites/Patch059-svitfel kvarstår.
  `TARGET_LVGL_POOL_HEADROOM=NOT_VERIFIED` (konfigurerad pool 65 536 byte;
  64-bitars hostmätningar bevisar inte målplattformens dynamiska marginal).
  `NVS_BACKUP=NOT_VERIFIED`. Fysisk HELP-/SAVING-/SAVED-/ERROR-interaktion och
  faktisk inställningspersistens är ännu inte separat testade.

### Nästa önskade utveckling – markiswidgetarnas dubbla blink (PENDING)

**Produktbeslut:** behåll befintligt widgetutseende och fasta steg helt. De fyra
positionsstrecken ligger vertikalt staplade (fyra synliga horisontella streck;
modellens fyllnad 0–4 motsvarar fem lägesnivåer). Ingen slider, ny färgremsa,
extra knapp eller permanent grafisk ändring.

- [ ] **Först källgranska V52D:** identifiera exakt vilka markiswidgetar som har
  övre/nedre tryckzon samt hur tryck faktiskt behandlas. Solskydd/Balkongmarkis
  och den större Markis-widgeten kan ha olika geometri/knappar; anta inte samma
  klicksemantik utan källbevis. Bekräfta de fyra befintliga strecken och aktuell
  command write-status innan någon implementation.
- [ ] Vid giltigt tryck ska **endast den tryckta widgetens övre ELLER nedre halva
  blinka exakt två gånger**, med befintlig färgsättning/tillfällig ljushetsökning.
  Total tid cirka 400–500 ms; återgå därefter pixelidentiskt till normalläget.
  Den andra halvan och övriga widgets ska vara opåverkade.
- [ ] Blink är endast **omedelbar kvittens på registrerat tryck**, inte bevis att
  ett kommando skickats/accepterats eller att markisen rört sig. Visa aldrig
  önskad position som verklig Homey-readback.
- [ ] Inga nya markis-write-paths aktiveras. Befintliga disabled-/fail-closed-
  behörighetsgränser gäller. Ingen Homey-mutation eller fysisk markisrörelse får
  ske under implementation/hosttest. Blinkningen får inte sända kommando, skapa
  dubbletter eller kringgå alias/capability/Selected-Homey-verifiering.
- [ ] Implementera senare i NY isolerad kopia av exakt V52D, inte i original-
  worktree. Testa med verklig LVGL-hostrendering: normallägespixelidentitet,
  två pulser på rätt halva, återställning, snabba upprepade tryck, ingen läcka,
  bevarade steg/status, oförändrade V52D-modaler och inga nya periodiska
  helskärmsinvalidationer. Rent isolerat ESP-IDF-bygge först efter PASS.
- [ ] Stoppa före flash, serial, reset, NVS-write eller Homey-write. Varje fysisk
  operation och varje markisrörelse kräver separat uttrycklig auktorisering.

**Patchnummerkonflikt att lösa före Codex-arbetet:** äldre SCRATCHPAD reserverar
`Patch077` för framtida Verisure-integration. Den senaste ChatGPT-prompten kallade
markisblinkningen `PATCH077_AWNING_EXISTING_WIDGET_DOUBLE_BLINK`. **Återanvänd inte
Patch077 för två olika uppdrag.** Välj och verifiera ett ledigt patch-/task-ID i
repo/handoff innan implementation; bevara Verisure-uppgiften i kön.

### Andra väntande aktiviteter och arbetsordning

- [ ] Fysisk kontroll av HELP-modal/touch utan att starta oönskad Wi-Fi-
  provisionering. Sparar.../Sparat/fel för en lokal inställning innebär möjlig
  NVS-skrivning och kräver separat uttryckligt beslut innan ett sådant test.
- [ ] Längre stabilitetsobservation av V52D och notering av fortsatt flimmer;
  ingen automatisk RGB-diagnostik, panel-GET eller omflashning.
- [ ] Verifiera målplattformens verkliga LVGL-minnesmarginal genom separat
  godkänd metod när det är motiverat; extrapolera inte 64-bitars hostvärden.
- [ ] Markispositions-/capability- och kommando-write-granskning kvarstår som
  separat framtida säkerhetsuppgift. Statusvisning är inte manövreringsstöd.
- [ ] Lägg till kommande användarönskemål som NYA daterade tilläggssektioner.
  Ändra inte äldre historik och markera inget som klart utan ny evidens.

`NEXT_ACTION=AWNING_FIXED_STEPS_DOUBLE_BLINK_SOURCE_AUDIT_WHEN_CODEX_AVAILABLE`
`ACTIVE_UI_IMPLEMENTATION=NONE`
`INSTALLED_APP=PATCH076_V52D_OPERATOR_VERIFIED_PARTIAL_RUNTIME`
`AWNING_WRITE_PATH=DISABLED_UNTIL_VERIFIED_AND_SEPARATELY_AUTHORIZED`

**Arbetsgräns tills nytt uttryckligt uppdrag:** inga Git-commits/pushar, inga
ändringar av handoff eller andra SCRATCHPAD-kopior, inga nya firmwareflashningar,
Homey-requests, NVS-ändringar eller fysiska markiskommandon. ChatGPT beslutar
nästa avgränsade steg; Codex implementerar när användningen är tillgänglig igen.
<!-- SCRATCHPAD_UPDATE_2026_10_14_V52D_AWNING_BLINK_END -->

<!-- SCRATCHPAD_UPDATE_2026_10_14_WIFI_REFRESH_CANCEL_MODAL_PLAN_BEGIN -->
## Komplettering 2026-10-14 (2) - Wi-Fi-status, avbrott och inställningsdialoger

**Status: PLANERAD / INTE IMPLEMENTERAD.** Denna nyare plan har företräde
framför tidigare `NEXT_ACTION`-prioritering, men ändrar inte historiken,
markisblinkbeslutet, Patch077:s befintliga Verisure-reservation eller tidigare
verifierad Git-/runtime-evidens. Ingen ny firmware har byggts eller installerats.

### Fysisk operatörsevidens efter installerad V52D

- **OBSERVED:** Operatören ändrade släckningstid och ljusstyrka. Den
  efterföljande modalen "Sparat" upplevs som onödig när det nya värdet
  redan visas. Detta bevisar inte fullständig NVS-persistens efter omstart.
- **OBSERVED:** Tryck på "Ändra Wi-Fi" öppnade provisioneringen.
  Dialogen saknade QR-kod och korrekt tydlig information för anslutning.
  Operatören kunde inte avbryta; ett senare tryck gav i stället fel om
  att Wi-Fi-inställningen redan var öppen.
- **OBSERVED:** Under förloppet visade panelen klocka, "Ansluten" och
  ljusstatus, men Solskydd och Balkongmarkis visade "Okänd". Live Status
  visade `state=ready`, `homey_data_state=ready`, samtidigt som
  `awning_snapshot.result=stale`, `fresh=false`, `age_ms=2930389`
  (ungefär 48 min 50 s), generation 53. Senast registrerade inventory-
  försöket hade `origin=periodic`, `sequence=53`, `completed=true`,
  `stage=inventory_complete` och `snapshot_published=true`. Samtliga tre
  markisalias var matchade med befintliga device/capability-bindningar;
  `alias_activation=ok`. Denna status är inte bevis att senare försök
  köades eller genomfördes.
- **OBSERVED:** Efter en vanlig USB-strömcykling återkom korrekt status
  för samtliga widgets. Ingen ny aliasprovisionering, fabriksåterställning
  eller firmwareflash rapporterades. En lyckad omstart är inte ett
  kausalitetsbevis för grundorsaken.
- **KVARSTÅR:** Periodiskt RGB-skärmflimmer observeras fortfarande i V52D;
  skilj detta problem från Wi-Fi-/Homey-färskhetsfelet.

### Prioritet 0 - Wi-Fi-provisionering får inte spärra Homey i onödan

**Status: SOURCE-INDICATED DEFECT / EXACT V52D SOURCE IDENTITY NOT VERIFIED.**
Read-only-granskning 2026-10-14 av publika GitHub `main`, INTE ännu av
exakt byggd V52D-kopia, visar denna konkreta orsakskedja:

1. `components/secure_bootstrap/secure_bootstrap_esp.c`:
   `open_provisioning()` anropar ovillkorligt
   `phone_provisioning_on_wifi_offline()` och startar därefter
   `WIFI_MODE_APSTA`. APSTA kan behålla en fungerande STA/IP-anslutning.
2. `components/secure_bootstrap/phone_provisioning_store.c`:
   `phone_provisioning_on_wifi_offline()` sätter den logiska
   `s_wifi_online=false` och återkallar live-readiness.
3. `components/secure_bootstrap/athom_oauth_runtime.c`:
   `queue_inventory_refresh_if_ready()` returnerar `NOT_READY` när
   `phone_provisioning_wifi_online()` är false. Periodiska inventeringar
   kan därför blockeras trots kvarvarande faktisk STA-anslutning.
4. `secure_bootstrap_esp.c`/`wifi_event()` accepterar inte alla normala
   STA-disconnect-/GOT_IP-händelser när Wi-Fi-state är PROVISIONING.
   Detta måste också granskas för korrekt återanslutning.

**Kausalitetsgräns:** Koden visar en plausibel och konkret spärr, som
stämmer med operatörens Live Status, men en tidskorrelerad mätning av
fysisk STA/IP och refresh-queue under just händelsen finns inte. Säg
inte att fysisk V52D-grundorsak är slutbevisad förrän källidentitet
och deterministisk reproduktion är verifierade.

- [ ] Kontrollera exakt V52D-källidentitet och om ovanstående
  funktionskedja finns i installerad firmware. Blanda inte `main`, äldre
  arbetskopior och V52D.
- [ ] Reproducera felet offline med relevanta tillståndsmaskiner/testdubblar:
  online STA -> start APSTA-provisionering -> periodisk inventering -> stale
  snapshot; efter korrigering ska samma fall fortsätta refresh.
- [ ] Separera tre tillstånd: faktisk STA/IP-anslutning, aktiv lokal
  AP-provisionering och Homey-inventeringens aktuella färskhet/behörighet.
  START av provisionering får INTE i sig sätta STA offline.
- [ ] Om STA faktiskt förlorar IP/anslutning ska nätverksberoende
  Homey-operationer spärras fail-closed. Efter verifierad återanslutning
  ska ordinarie inventering återupptas, utan ny OAuth-login, aliasmutation
  eller extra Homey-anrop utanför befintlig begränsad refresh-policy.
- [ ] Hantera disconnect, reconnect och GOT_IP korrekt även i
  PROVISIONING. Var explicit om hur gammalt `ready` kan vara; när
  positionssnapshot är stale får panelen inte antyda att aktuella
  positioner är verifierade. Håll transport- och datafärskhetsstatus
  åtskilda. Behåll 120 s TTL tills separat krav beslutas.
- [ ] Testa första installation utan sparat Wi-Fi, APSTA med kvarvarande
  STA/IP, faktisk STA-förlust, återanslutning, föråldrat/färskt
  snapshot och Homey-transportfel. Inga riktiga Homey-requests i hosttest.

### Prioritet 1 - Ändra Wi-Fi: korrekt modal, lösenord och avbryt

**Status: PENDING - efter ovanstående state-/säkerhetsgranskning.**

- [ ] Behåll en modal som är läsbar på 480x480 (minst 22 px för
  användartext, ingen kapning av SSID/lösenord). Den ska visa korrekt
  installations-SSID, **hela AP-lösenordet i klartext** och den riktiga
  QR-koden. QR, utskrivet lösenord och aktiv AP-konfiguration ska alltid
  härröra från samma aktuella provisioneringssession.
- [ ] Återanvänd första uppstartens verifierade Wi-Fi-QR-format
  (`[private Wi-Fi QR payload redacted]` enligt granskad `main`).
  Beskriv rätt sekvens: anslut telefonen till panelens installations-
  Wi-Fi (via QR eller SSID/lösenord); öppna sedan portaladressen
  `[private local endpoint redacted]` i webbläsaren. IP-adressen är inte SSID.
- [ ] Hantera rotation/utgång av tillfällig AP-kod utan att QR,
  lösenord, portal och AP tappar synk. Visa aldrig lösenord i
  SCRATCHPAD, loggar, rapporter, testartefakter eller Homey.
- [ ] Modalens **Avbryt Wi-Fi-ändring** ska avsluta den pågående
  provisioneringen på riktigt: stäng AP/portal på rätt sätt,
  bevara tidigare kända STA-uppgifter och återgå till normal drift.
  Att bara dölja modalens UI räcker inte.
- [ ] Vid nästa tryck på Wi-Fi-inställningsknappen under pågående
  provisionering ska användaren kunna **avbryta**, inte bara få
  "redan öppen". Knappen kan visa "Avbryt Wi-Fi-ändring" när
  detta är tillåtet. Ett avbrott ska inte skapa ny session, ny
  inställningsskrivning eller dubbla callbacks.
- [ ] Säkerställ säker cancel/commit-race: stoppa inte en pågående
  permanent Wi-Fi-skrivning mitt i transaktionen. Om avbrott inte längre
  är säkert, visa begripligt tillstånd och bevara verifierad
  persist-/rollback-logik. Ingen fabriksåterställning eller radering.
- [ ] Testa att upprepade öppna/avbryt-cykler och normala Homey-uppdateringar
  inte läcker tasks, timers, minne, serverhandlers eller AP-resurser.

### Prioritet 2 - Ta bort lyckad-sparad-dialogen (SAVED)

**Status: PENDING.** Operatörens val: ingen modal "Sparat" efter lyckad
inställningsändring; den synliga inställningsraden räcker.

- [ ] Gäller samtliga fem lokala skärminställningar: normal ljusstyrka,
  dimmad ljusstyrka, dimningstid, släckningstid och bakgrund.
- [ ] Behåll den befintliga icke-stängbara modalen "Sparar..." tills
  en verklig matchad resultatgeneration anger lyckad sparning. Då stängs
  den automatiskt och direkt, **utan** att visa "Sparat" eller en timer.
- [ ] Behåll tydlig stängbar felmodal vid misslyckad sparning samt HELP-
  dialogers beteende i övrigt. Ingen optimistisk NVS-bekräftelse
  utifrån bara UI-värdet. Bevara readback, generation, kö, persistens-
  policy och V52D:s R2-guard mot fastlåst "Sparar...".
- [ ] Verifiera snabba upprepade ändringar, no-op, fel, generationsrace,
  normalläge och avsaknad av extra omritning i riktade hosttester.

### Prioritet 3 - Bevara markisblink-planen

- [ ] Tidigare beslut om exakt två diskreta blinkningar i tryckt övre
  eller nedre halva och oförändrade fyra vertikalt staplade positions-
  streck gäller. Ingen slider, ny färgremsa eller aktiverad write-path.
- [ ] Granska aktuell V52D-layout: de två mindre markiswidgetarna och
  det större Markis-kortet kan ha olika tryckzoner/funktioner. Gissa inte.
- [ ] Lös patch-ID-konflikten: `Patch077` är historiskt reserverat för
  Verisure; återanvänd inte beteckningen för flera olika uppdrag.

### Arbetsordning, validering och uttrycklig stoppgräns

1. När Codex fungerar igen: läs repots `AGENTS.md`,
   `PROJECT_INSTRUCTIONS.md` och handoff-index i dess föreskrivna ordning.
   Kontrollera exakt installerad V52D-källa och utgångsidentiteter innan
   nya patch-/task-ID väljs. Inga gamla `NEXT_ACTION` ska köras blint.
2. Separera säker Wi-Fi-state-/refresh-rättning från rent kosmetiska
   UI-förbättringar i testbara avgränsade patchar. Återanvänd
   relevant verifierad evidens; hävda inte full host-PASS när två
   dokumenterade legacy-sviter fortfarande fallerar.
3. Testa offline i en NY isolerad kopia: state-machine-fall, regression för
   Homey snapshot/freshness, Wi-Fi AP/STA, QR och cancel, LVGL-modal, R2 och
   markis-/ljusbindningar. Rent ESP-IDF v6.0.1-bygge först efter gates.
4. Bevara V52D installerad tills annan separat beslutspunkt. Inget i denna
   plan är flash-/Wi-Fi-/Homey-/NVS-auktorisering.

`NEXT_ACTION=OFFLINE_EXACT_V52D_WIFI_PROVISIONING_HOMEY_REFRESH_SOURCE_AND_STATE_AUDIT`
`INSTALLED_APP=PATCH076_V52D_OPERATOR_VERIFIED_WITH_WIFI_RECOVERY_AFTER_REBOOT`
`WIFI_PROVISIONING_HOMEY_REFRESH=SOURCE_INDICATED_DEFECT_NOT_IMPLEMENTED`
`WIFI_CANCEL_QR_PASSWORD_UI=PENDING`
`SETTINGS_SAVED_MODAL_REMOVAL=PENDING`
`AWNING_DOUBLE_BLINK=PENDING`
`AWNING_WRITE_PATH=DISABLED_UNTIL_SEPARATELY_AUTHORIZED`

**Tillåten åtgärd för detta SCRATCHPAD-skript:** dokumentuppdatering av
exakt en lokal fil med backup. Ingen källkod, handoff, andra worktrees,
Git-commit/push/PR, panelkontakt, serial, flash, reset, NVS-operation eller
Homey-mutation. Framtida fysisk test eller förändring kräver en egen
uttrycklig och avgränsad auktorisering.
<!-- SCRATCHPAD_UPDATE_2026_10_14_WIFI_REFRESH_CANCEL_MODAL_PLAN_END -->

## SCRATCHPAD_UPDATE_2026_10_14_PATCH078_WIFI_STA_REFRESH_STATE

- V52D source identity verified against its 7,693-file manifest. Work was
  performed only in the isolated copy
  `~/Downloads/PATCH076_WIFI_REFRESH_STAGE_20261014/`.
- `open_provisioning()` had cleared STA online readiness before APSTA mode.
  The old APSTA->offline->periodic-refresh-rejected sequence was reproduced
  deterministically using the unchanged production queue gate.
- Patch078 offline correction separates STA/IP availability from AP portal
  state, handles GOT_IP/DISCONNECTED during PROVISIONING, uses synchronized
  link-state reads, gates subsequent Homey network phases while STA is down,
  and presents CONNECTED only with online STA, READY runtime and a fresh,
  generation-valid snapshot. Snapshot TTL and stale fail-closed behavior remain
  unchanged; no new Homey write, OAuth provisioning, alias binding, NVS format,
  retry, or polling policy was added.
- Focused Wi-Fi/APSTA, periodic scheduler, worker, read-only safety, dashboard
  stale-state, clock and V35 parser/alias/snapshot/LVGL tests passed. Aggregate
  host status remains `PARTIAL` for the documented legacy suites.
- Clean ESP-IDF v6.0.1 build passed. Candidate:
  `~/Downloads/PATCH076_WIFI_REFRESH_STAGE_20261014_validation/build_release_final2/esp32_homey_wall_panel.bin`
  SHA-256 `f4b0c0404d10961854e0d86feeb7204240230348a5760a62df1d60df78a1ed3e`.
  App size 1,688,208 bytes; static DRAM +32 bytes, IRAM 0, static PSRAM 0;
  dependency lock and partition table match V52D.
- The prior V52D build directory was inadvertently invoked by a size-inspection
  command. Its source remained unchanged, and its final ELF and firmware hashes
  still match the recorded V52D hashes; the build tool did rerun within that
  old build directory. No physical operation occurred.
- Runtime verification is not run. RGB flicker remains unresolved and separate;
  `NVS_BACKUP=NOT_VERIFIED`.
- Next: Patch079 Wi-Fi change modal/cancel flow, built as a separate candidate
  on top of Patch078. Keep Patch080 SAVED-modal removal and Patch081 awning
  feedback independently isolated. Patch077 remains reserved for Verisure.


## SCRATCHPAD_UPDATE_2026_10_14_PATCH080_SAVED_MODAL_REMOVAL_AND_STAGE_GATES

- Patch078 offline Wi-Fi STA/Homey refresh correction completed in an isolated
  candidate and clean-built. Candidate SHA-256:
  `f4b0c0404d10961854e0d86feeb7204240230348a5760a62df1d60df78a1ed3e`.
  It remains uninstalled and has no physical runtime verification.
- Patch079 Wi-Fi session modal/cancel is BLOCKED before implementation. Current
  provisioning UI does not expose the active QR/password inside the modal;
  successful reconfigure leaves `s_reconfigure_pending` set; and safe cancel
  versus permanent Wi-Fi commit requires a synchronized state-machine boundary
  outside the initial file scope. No Patch079 code was changed. See
  `~/Downloads/PATCH079_WIFI_DIALOG_STAGE_20261014_validation/RESULT_B.md`.
- Patch080 removes the SAVED modal after a correctly paired local settings-save
  completion; SAVING closes immediately. Focused UI/settings tests and real LVGL
  rendering passed (139 frame comparisons); clean ESP-IDF v6.0.1 build passed.
  Candidate SHA-256:
  `ed6cc1cb7cdd031ce33496ed1b74bad0db9339f0ca9e89af628bc52ef091d1d0`.
  This is an isolated V52D-based candidate, not combined with Patch078.
- Patch081 awning double-blink is BLOCKED before implementation: current
  awning cards are non-clickable and their visible direction zones are disabled.
  Introducing a new press zone would change input/swipe behavior, so no assumed
  touch semantics or code were added. See
  `~/Downloads/PATCH081_AWNING_DOUBLE_BLINK_STAGE_20261014_validation/RESULT_D.md`.
- Runtime UI, Wi-Fi cancellation, target LVGL pool headroom, physical layout,
  and RGB flicker are not verified by these offline stages. V46 remains the
  installed baseline per task; no physical operation occurred.
- Aggregate host status remains PARTIAL due two documented legacy failures.
  `NVS_BACKUP=NOT_VERIFIED`; `AWNING_WRITE_PATH=DISABLED`.
