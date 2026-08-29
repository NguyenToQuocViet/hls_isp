<!--
Project: Adaptive Directional BPC and BLC
Module: Repository Guidelines
Description: Define repository-wide contribution and agent behavior.
Author: Viet Nguyen To Quoc
-->

# Repository Guidelines

## Scope and Sources of Truth

This personal R&D workspace covers Adaptive Directional Bad Pixel Correction (BPC), the primary IP, and Black Level Correction (BLC), a secondary HLS block. It is not the complete Viettel ISP. Never add unrelated ISP modules, team infrastructure, or confidential Viettel source or data.

`README.md` owns scope; `docs/` owns contracts and decisions; source and tests show implemented behavior; Git owns history. Treat formulas, architectures, and performance targets as hypotheses until documented and verified. Do not duplicate specifications here.

## Project Structure & Module Organization

- `reference/`: readable algorithms and golden models.
- `hls/bpc/`: synthesizable BPC HLS source.
- `hls/blc/`: synthesizable BLC HLS source.
- `tests/`: reference, HLS, and cross-model equivalence tests.
- `scripts/`: focused data and experiment utilities.
- `docs/`: engineering contracts, decisions, and evidence summaries.

Keep the hierarchy shallow. Add `include/`, `data/`, `results/`, or similar directories only for real artifacts. Do not create files merely to preserve empty directories.

## Build, Test, and Development Commands

No build system, source, or test framework is committed. Do not invent commands or generated Vitis/Vivado layouts. Document reproducible commands here when the toolchain exists. Checks:

```sh
git status --short
git diff --check
```

Keep handwritten source separate from generated output under ignored `build/` or `artifacts/` directories.

## Coding Style & Naming Conventions

Use lowercase directories and descriptive filenames. For C++, use four-space indentation, `snake_case` for functions and variables, `PascalCase` for types, and `UPPER_SNAKE_CASE` for constants. Keep reference code clear and independent of HLS optimization. Add hardware-specific types or optimizations only when an accepted design justifies them.

## File Header Convention

Every new human-authored code or documentation file must begin with these four fields in this order. Bring an existing file into compliance when next editing it.

```text
Project: Adaptive Directional BPC and BLC
Module: <logical name of this file or module>
Description: <one concise sentence describing its responsibility>
Author: Viet Nguyen To Quoc
```

`Project` and `Author` are fixed verbatim. `Module` names the file's logical unit, such as `Defect Injector` or `Defect Injection Contract`; `Description` is file-specific. Use native comments: `//` for C/C++/Verilog/SystemVerilog, `#` after any required shebang for Python or shell, and an HTML comment for Markdown. Exclude `.gitignore`, data, binaries, generated output, and third-party files. Do not add dates, email, version, license, or copyright fields unless this contract is explicitly revised.

## Testing Guidelines

Name tests `test_<behavior>.<ext>` and identify their target: reference, HLS, or equivalence. Cover normal data, defects, boundaries, saturation, and arithmetic corner cases. Fix random seeds and record parameters. Claims such as `II=1`, resource use, or bit equivalence require reproducible evidence. No coverage threshold exists yet.

## Commit and Review Guidelines

No commit history exists to infer a convention. Use focused, imperative subjects, for example `docs: define defect injection contract`. Do not commit generated builds, downloaded DNG files, or unrelated changes. Reviews state scope, governing contract, commands run, results, and limitations.

## Agent Boundaries

Inspect before changing files. Explanation, review, diagnosis, and planning are read-only. Modify only explicitly authorized artifacts; never silently implement, refactor, commit, or expand scope. For educational source work, guide Việt unless implementation is explicitly delegated. Raise decision-relevant ambiguity before acting and preserve existing work.
