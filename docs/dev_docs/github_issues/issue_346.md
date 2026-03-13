# Fix: car class detected as "Unknown" for hypercars #346

**State:** Open
**Opened by:** coasting-nc on Mar 13, 2026

## Description

The *.bin logs have "Unknown" as car class for hypercars.
Eg. a file name like this gets generated:
"lmuffb_log_2026-03-13_11-47-54_Cadillac_Unknown_Silverstone_Grand_Prix_-_ELMS.bin"

The debug log shows the following:

[11:47:13] [FFB] Vehicle Identification -> Detected Class: Unknown | Seed Load: 4500.00N (Raw -> Class: Hyper, Name: Cadillac WTR 2025 #101:LM)
