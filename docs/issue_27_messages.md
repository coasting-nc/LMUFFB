# GitHub Issue #27: When game crashes or exits the driving session the FFB signal remains active with strong forces

**Opened by:** @coasting-nc on Feb 1, 2026

## Description
We need to detect more reliably when a driving session is no longer active and stop all FFB signal.

### More details:
* "If the client gets disconnected from server (end of practice time) ffb remains active, eg if i was going full speed through corner, that force will remain on the wheel."
  [Link to forum post](https://community.lemansultimate.com/index.php?threads/lmuffb-app.10440/post-77378)
* "If I pause the game and am in a corner with the FFB loaded up, the FFB effects continue after pausing."
  [Link to forum post](https://community.lemansultimate.com/index.php?threads/lmuffb-app.10440/post-77722)
* "when the server disconnects during gameplay, the wheel suddenly snaps hard to one side and stays locked there. I’m using an R21 Ultra, and the force when it happens is extremely strong, which was quite alarming."
  [Link to forum post](https://community.lemansultimate.com/index.php?threads/lmuffb-app.10440/post-79481)

### Current workaround and clue for fix:
* "It doesnt always happen, but when it does alt-tab twice (to LMUFFB and back to LMU) should sort it out"
  [Link to forum post](https://community.lemansultimate.com/index.php?threads/lmuffb-app.10440/post-79573)

### Related issue:
* [Wheel pulls on one end with max force when starting driving session #23](https://github.com/coasting-nc/LMUFFB/issues/23)
