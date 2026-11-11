# Patch117Q1 close-failure cleanup

Patch117Q1 adds a bounded cleanup to the Patch117Q fresh-transport experiment.
If closing the Homey transport after successful Zones fails, the retained
Favorites response buffer is now securely zeroed and freed before the existing
fail-closed return. Devices is not requested on that path. No transport,
authentication, TLS, retry, timeout, identity, snapshot, or scheduler behavior
was changed.

The regression test first failed against Patch117Q because this path left the
Favorites buffer allocated. It passes against Q1 and verifies zero-before-free,
one free, no double-free, no Devices request after close failure, and no
snapshot publication. The inventory gate passes with the fresh-transport flag
both OFF and ON; relevant transport and Homey recovery host tests also pass.
ESP-IDF v6.0.1 ESP32-S3 offline app build and factory partition fit pass.

Patch117Q remains the installed physical firmware and remains the only version
with physical validation. Patch117Q1 is offline validated only. The Q experiment
flag defaults to OFF; the physically tested Q build used ON. The separate Q
experiment defaults file sets ON for comparison builds. Do not infer that a
production or later build enables the experiment without checking its effective
configuration.
