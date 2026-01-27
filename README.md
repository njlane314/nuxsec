# Nuxsec

ROOT-based utilities for a neutrino cross-section analysis pipeline built around explicit analysis entities
(Fragment → Sample → Dataset/RDF → Channel/Category → Selection → Template/Plot).

## Core concepts (pipeline map)

- **LArSoft job outputs** (partitions / shards)
- **Art provenance aggregation** (input-level manifests)
- **Logical samples** (collection of shards + merged view)
- **Exposure / POT accounting** (per shard, per logical sample, target POT scaling)
- **RDataFrame construction** (source trees, friend trees, derived columns, weights)
- **Channels / categories** (analysis topology, truth categories, control regions)
- **Selections and template definitions** (cutflow, histograms, response matrices, xsec outputs)
- **Template production and plotting** (binned templates, stacked plots)

## Repository structure

This is a COLLIE-like module layout. Each module is built as its own shared library.

```
nuxsec/
  io/      # LArSoft output discovery, file manifests, provenance extraction
  ana/     # analysis-level definitions and ROOT::RDataFrame sources + derived columns
  plot/    # plotting utilities for stacked histograms and diagnostic outputs
  apps/    # unified CLI (aggregators, template makers)
  scripts/ # environment helpers
```

## Macros and executables

### What the macros are used for

Macro folders (for example, `io/macro/` and `ana/macro/`) are reserved for interactive ROOT
scripts that perform post-processing and visualisation of outputs produced by the compiled
applications. Empty `.keep` placeholders are committed so the macro directories stay tracked
in the repository. These macros are intended for quick inspection and plotting workflows, such
as reading output ROOT files, dumping diagnostic values, and generating plots for systematic
variations or fit-response checks.

### How they differ from applications

Applications are compiled executables built by the `Makefile` that perform the core Nuxsec
workflows: aggregating inputs, building RDF datasets, and producing the ROOT outputs used by
downstream analysis. Macros, in contrast, are not standalone executables—they are run
interactively inside ROOT (for example, `root -l` followed by `.L myMacro.C`) to inspect or
visualise those outputs. In short, applications do the heavy calculations, while macros focus
on analysis and plotting of the results.

## Requirements

- C++17 compiler (e.g. `g++`)
- ROOT (for `root-config` and runtime I/O)
- sqlite3 development headers/libs

## Build

```bash
make
```

This produces:

- `build/lib/libNuxsecIO.so`
- `build/lib/libNuxsecAna.so`
- `build/lib/libNuxsecPlot.so`
- `build/bin/nuxsec`
- `build/bin/nuxsecArtFileIOdriver`
- `build/bin/nuxsecSampleIOdriver`
- `build/bin/nuxsecEventIOdriver`
- `./nuxsec` (wrapper script that runs `build/bin/nuxsec`)

## Analysis processing

The `libNuXsecAna` library provides `nuxsec::ColumnDerivationService` and RDF construction helpers
for defining analysis-level columns (weights, fiducial checks, channel classifications) on `ROOT::RDF::RNode`.

The `nuxsec` CLI drives the art/sample/event aggregation stages and the `plot` module provides
helper utilities for ROOT-based plotting and macro workflows.

## CLI overview

```bash
nuxsec -h
```

```text
Usage: nuxsec <command> [args]

Commands:
  art         Aggregate art provenance for an input
  sample      Aggregate Sample ROOT files from art provenance
  event       Build event-level output from aggregated samples
  macro       Run plot macros

Run 'nuxsec <command> --help' for command-specific usage.
```

## Runtime environment

```bash
source .setup.sh
```

This adds `build/bin` to your `PATH` and `build/lib` to your `LD_LIBRARY_PATH` so the
executables can locate the shared libraries at runtime.

If you prefer to run from the repository root without modifying your `PATH`, use the
wrapper script:

```bash
./nuxsec <command> [args...]
```

## Prepare file lists

File lists are newline-delimited paths to ROOT files (blank lines and `#` comments are ignored).

```bash
cat > data.list <<'LIST'
# input file list
/path/to/input1.root
/path/to/input2.root
LIST
```

## Example command-line workflow (template-style)

Assume you run from the repo top-level (`nuxsec/`) and you already have per-input filelists
produced by your partitioning step.

### 1) Input → Art provenance ROOT (per partition/input)

Each input filelist is a plain text file containing the input art/selection ROOT files for that input.

```bash
nuxsec art "sample_a:inputs/filelists/sample_a.txt:Data:Beam"
nuxsec art "sample_b:inputs/filelists/sample_b.txt:EXT:Beam"
nuxsec art "sample_c:inputs/filelists/sample_c.txt:Overlay:Beam"
nuxsec art "sample_d:inputs/filelists/sample_d.txt:Dirt:Beam"
```

Outputs (by code convention):

```
build/out/art/art_prov_sample_a.root
build/out/art/art_prov_sample_b.root
build/out/art/art_prov_sample_c.root
build/out/art/art_prov_sample_d.root
```

Repeat `nuxsec art ...` for each partition/input you need.

### 2) Art provenance ROOT → Sample ROOT (group inputs into samples)

Create per-sample filelists that contain the art provenance ROOT outputs from step (1).
If you want these lists under the build tree, prefer a dedicated directory (for example
`build/out/lists/`) to avoid confusion with the output directory `build/out/sample/`:

```bash
mkdir -p build/out/lists
ls build/out/art/art_prov_sample_a*.root > build/out/lists/sample_a.txt
ls build/out/art/art_prov_sample_b*.root > build/out/lists/sample_b.txt
ls build/out/art/art_prov_sample_c*.root > build/out/lists/sample_c.txt
ls build/out/art/art_prov_sample_d*.root > build/out/lists/sample_d.txt
```

Then aggregate each sample:

```bash
nuxsec sample "sample_a:build/out/lists/sample_a.txt"
nuxsec sample "sample_b:build/out/lists/sample_b.txt"
nuxsec sample "sample_c:build/out/lists/sample_c.txt"
nuxsec sample "sample_d:build/out/lists/sample_d.txt"
```

Outputs:

```
build/out/sample/sample_root_<sample>.root
build/out/sample/samples.tsv
```

The TSV is the only input list you pass downstream.

#### Separate sample lists for training versus templates/plots

If you want full separation between training and histogram/plot production, keep two
independent sample aggregations and `samples.tsv` handoffs. This keeps the inputs disjoint
and makes the handoff explicit.

**Step A: build two sets of input lists**

```bash
mkdir -p build/out/lists_train build/out/lists_plot

# Training lists (use only the inputs you want for training)
for kind in sample_a sample_b sample_c sample_d; do
  ls build/out/art/art_prov_${kind}_train*.root > build/out/lists_train/${kind}_train.txt
done

# Plot/template lists (disjoint from training)
for kind in sample_a sample_b sample_c sample_d; do
  ls build/out/art/art_prov_${kind}_plot*.root > build/out/lists_plot/${kind}_plot.txt
done
```

**Step B: aggregate each set and stage outputs**

The `sample` CLI always writes to `build/out/sample/`, so run one set at a time and move
the outputs into per-purpose directories:

```bash
# Training samples/TSV
for kind in sample_a sample_b sample_c sample_d; do
  nuxsec sample "${kind}_train:build/out/lists_train/${kind}_train.txt"
done

mkdir -p build/out/sample_train
mv build/out/sample/sample_root_* build/out/sample/samples.tsv build/out/sample_train/

# Plot/template samples/TSV
for kind in sample_a sample_b sample_c sample_d; do
  nuxsec sample "${kind}_plot:build/out/lists_plot/${kind}_plot.txt"
done

mkdir -p build/out/sample_plot
mv build/out/sample/sample_root_* build/out/sample/samples.tsv build/out/sample_plot/
```

**Handoffs**

- `build/out/sample_train/samples.tsv` for training.
- `build/out/sample_plot/samples.tsv` for templates, histograms, and plots.

### 3) Samples → Event-level output (compiled analysis)

The compiled analysis definition in this repository is `nuxsec_default` with tree
name `Events` by default. Override the input tree name by exporting `NUXSEC_TREE_NAME`
before running the CLI. The event builder writes a single ROOT file containing the
event-level tree plus metadata for the aggregated samples.

```bash
nuxsec event build/out/sample/samples.tsv build/out/event/events.root
```

### 4) Plotting via macros

Plotting is macro-driven. Use the `nuxsec macro` helper to run a plot macro
(and optionally a specific function inside it).

```bash
nuxsec macro plotPotSimple.C
```

Shell completion for these commands is available in `scripts/nuxsec-completion.bash` (source it
in your shell profile or session).

### What the user “touches” in this UX

- Filelists for inputs (already produced by your partitioning step).
- Per-sample grouping filelists (or a small helper script).
- `samples.tsv` is produced automatically and becomes the stable handoff.
- After that, users only run:
  - `nuxsec event …` (required)
  - `nuxsec macro …` (optional, macro-driven plotting outputs).
