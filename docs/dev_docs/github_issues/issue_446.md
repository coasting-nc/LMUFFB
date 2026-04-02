# Show warning in main GUI if user has selected In-Game FFB but there is no signal coming from that. #446

**Status: Open**
**Opened by: coasting-nc on Mar 20, 2026**

## Description

Show warning in main GUI if user has selected In-Game FFB but there is no signal coming from that.
That is likely because the user has disabled FFB from the Game settings.
The warning could be the slider / setting text becoming red and flashing, with an explanation either in the tooltip and / or added to the widget text.

It seems many users are still influenced from earlier versions of the app, whose readme said you had to disable FFB from the game. This is no longer the case.

Also update the readmes to recommend setting 100% FFB strength in LMU, and adjusting gain Steering Torque Gain from lmuFFB.

See also:
* [In-Game FFB Gain slider value makes no difference (0.7.207) #463](https://github.com/coasting-nc/LMUFFB/issues/463)
* [v0.7.207 feels like has lots of damping and/or smoothing #459](https://github.com/coasting-nc/LMUFFB/issues/459)
