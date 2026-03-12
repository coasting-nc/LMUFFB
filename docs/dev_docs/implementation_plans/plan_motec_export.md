# Implementation Plan - MoTeC i2 Pro Export Feature

## Context
This plan outlines the implementation of a MoTeC i2 Pro compatible export feature for the LMUFFB log analyzer. This will allow developers and users to analyze high-frequency force feedback and physics telemetry using industry-standard tools.

## Design Rationale
### Architectural Integrity
The export logic will be encapsulated in a new `MotecExporter` class within the `tools/lmuffb_log_analyzer/exporters` module. This maintains a clean separation between data loading, analysis, and export formats.

### Physics-Based Reasoning for Resampling
MoTeC i2 Pro strictly requires constant sampling frequencies for all channels. However, the LMU physics engine and the LMUFFB logger operate at variable intervals (target 400Hz, but subject to frame time jitter).
- **Approach**: We will resample all telemetry to a fixed frequency (default 100Hz).
- **Interpolation**: Linear interpolation will be used for continuous physics values (accelerations, speeds, loads) to prevent aliasing and "stair-stepping" in graphs.
- **Precision**: We will use 16-bit signed integers for data storage to mimic real-world hardware loggers, utilizing scaling factors (e.g., 0.01 or 0.1) to preserve necessary decimal precision.

### Software Compatibility (Pro Features)
To ensure the generated files are recognized as "Pro" files by MoTeC i2, we will include the reverse-engineered "Pro Enabled" magic number (`0xc81a4`) at the specific byte offset (`0x5DE`) in the `.ld` file header.

## Reference Documents
- `docs/dev_docs/investigations/Exporting LMU Telemetry to MoTeC.md`
- `docs/dev_docs/specifications/motec_file_format.md` (to be created)

## Codebase Analysis Summary
### Current Architecture
The `lmuffb_log_analyzer` uses a `loader.py` to ingest `.bin` (binary LZ4) or `.csv` files into a `pandas.DataFrame`. Analyzers and plots then operate on this DataFrame.

### Impacted Functionalities
- **CLI (`cli.py`)**: A new `export-motec` command will be added.
- **Models (`models.py`)**: No changes expected, will use existing `SessionMetadata`.
- **Loader (`loader.py`)**: No changes expected, will use existing loading logic.

## Proposed Changes

### 1. Specification Document
- Create `docs/dev_docs/specifications/motec_file_format.md`.
- Detail the binary structure of `.ld` (Headers, Channel Metadata linked-list, Data blocks).
- Detail the XML structure of `.ldx`.

### 2. New Module: `tools/lmuffb_log_analyzer/exporters/motec_exporter.py`
- **Class `MotecExporter`**:
  - `export(metadata, df, output_path)`: Main entry point.
  - `_resample(df, target_freq)`: Resample variable-rate data to fixed-rate.
  - `_write_ld(metadata, data_dict, output_path)`: Binary serialization for `.ld`.
  - `_write_ldx(metadata, lap_markers, output_path)`: XML generation for `.ldx`.

### 3. CLI Integration: `tools/lmuffb_log_analyzer/cli.py`
- Add `@cli.command() export-motec`.
- Parameters: `logfile`, `--output`, `--freq`.

## Test Plan (TDD-Ready)

### Design Rationale
Tests will focus on byte-level correctness of the binary output. Since the MoTeC parser is unforgiving, we must ensure magic numbers and offsets are exact.

### Test Cases (`tools/lmuffb_log_analyzer/tests/test_motec_exporter.py`)
1. **test_header_generation**: Verify the first 0x664 bytes of the `.ld` file contain the correct magic numbers, Pro flag, and initial pointers.
2. **test_channel_linked_list**: Verify that the channel metadata headers correctly point to each other and that the last one terminates with a zero pointer.
3. **test_data_scaling**: Verify that a known floating-point value (e.g., 50.0 km/h) is correctly scaled and packed into its 16-bit integer representation.
4. **test_resampling_alignment**: Verify that the resampled data has the correct number of samples for a given duration and frequency.
5. **test_ldx_structure**: Verify the generated XML contains valid lap beacon nodes and matches the binary filename.

## Deliverables
- [ ] `docs/dev_docs/specifications/motec_file_format.md`
- [ ] `tools/lmuffb_log_analyzer/exporters/motec_exporter.py`
- [ ] `tools/lmuffb_log_analyzer/tests/test_motec_exporter.py`
- [ ] Updated `tools/lmuffb_log_analyzer/cli.py`
- [ ] `docs/dev_docs/investigations/Motec Export Limitations.md`
- [ ] Incremented version in `VERSION` and `src/Version.h.in`
