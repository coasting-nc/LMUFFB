# Disable by default "Safety Duration" time window #350

Opened by **coasting-nc** on Mar 13, 2026

## Description

Under "FFB Safety Features -> Safety Duration", set the default value to 0 seconds, effectively disabling it.
Also set it to 0 in any of the built-in presets if it has a different value set for it.
And make sure that whenever we load a preset that does not have an explicit setting for " Safety Duration", we set it to 0 at load time.

The motivation for this change comes from the fact that multiple users have reported that this "safety duration" only results in losing FFB during driving scenarios, when one is in an online server and people join or leave the server, which leads to stuttering and trigger the disabling of FFB for a time of "Safety Duration" length.
This setting seems to have no safety benefit, since the stuttering due to people joining or leaving the server does not seem to cause dangerous spikes in force of the FFB.

See also:
https://community.lemansultimate.com/index.php?threads/lmuffb-app.10440/post-84482
