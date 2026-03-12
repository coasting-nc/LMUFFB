Investigate how to export the .bin logs to Motec i2Pro.

Determine the correct format read by Motec i2Pro.

See other open source projects that export to Motec i2Pro to see the format, tools and convention for exporting to Motec i2Pro.

ld format: This structure has been meticulously mapped by various open-source reverse engineering efforts, most notably the ldparser written in Python, the Aim_2_MoTeC converter written in C#, and the motec-i2 library written in Rust.

* https://github.com/alelosbrigia/LMU-telemetry

DAMPlugin for rF2 ("This plugin generates Motec i2Pro compatible log files.")
* https://forum.studio-397.com/index.php?threads/damplugin-for-rf2.49363/


Unrelated to rF2 / LMU, but it is opensource and converts to Motec i2Pro format:
"We invested a significant amount of effort in reverse engineering to decipher Motec's proprietary file format. This was necessary to enable the file to be both written and opened by i2 Pro. If i2 detects even a minor formatting error in the file, it will either refuse to read it or recognize it as a Standard file rather than a Pro file."

* https://github.com/ludovicb1239/Aim_2_MoTeC


* https://forum.motec.com.au/viewtopic.php?f=26&t=4317


* https://github.com/patrickmoore/Mu

* https://github.com/stevendaniluk/MotecLogGenerator
* https://github.com/stevendaniluk/MotecLogGenerator/blob/master/motec_log.py

ldpartser:
* https://github.com/gotzl/ldparser/tree/f05d2f8e51059a287ac39807552ce57adb9e3e25
* https://github.com/GeekyDeaks/sim-to-motec
* https://forum.kw-studios.com/index.php?threads/slipsteer-ldx-motec-telemetry-file-generator.17978/#:~:text=KamilDabkowski%20Member&text=Hello%2C,outside%20of%20motec%20licensed%20tools).&text=Just%20need%20to%20click%20start,or%20%22borderles%20window%22%20mode.



## Alternatives to Motec

Also investigate alternatives to Motect that have feature parity with it for most typical use in sim racing.

* https://www.simracingtelemetry.com/

"An application to display telemetry data recorded by Assetto Corsa Competizione.

It is build around bokeh and displays various figures that are inspired by the ACC MoTec i2 workspace. See also this article." https://www.overtake.gg/threads/acc-blog-motec-telemetry-and-dedicated-acc-workspace.165714/
* https://github.com/gotzl/acctelemetry




## Building a new toold

SimTelemetryViewer
