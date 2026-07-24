# Patch 005 - ESP32-native Athom Cloud Authentication and Inventory

## Purpose

Establish the offline-buildable ESP-IDF authentication, credential lifecycle,
Homey selection and read-only inventory boundaries required when Homey and the
panel are on different networks.

## Evidence class

This phase provides architecture, source, host-test and ESP-IDF build evidence.
It does not provide live OAuth, protocol, cloud-session, hardware or inventory
runtime evidence.

## Non-goals

- real credentials;
- live token exchange;
- guessed HTTPS endpoints;
- PKCE or device grant;
- generic Homey API forwarding;
- mutations;
- merge of PR #6.
