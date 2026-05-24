# Release Notes Guide

## Goal
Provide machine-readable release notes for every module under `lib/`.

## File Name
Each module must contain:
- `release_notes.md`

## Required Format
Use this exact section structure for each version.

```markdown
# Release Notes

## 1.2.3
### Breaking Changes
- Renamed API ... to ...

### Feature
- Added support for ...

### Bug
- Fixed crash when ...
```

## Rules
- Version heading `## <version>` must match module `library.json`.
- Categories must be exactly:
  - `### Breaking Changes`
  - `### Feature`
  - `### Bug`
- Category order inside each version must be exactly:
  1. `### Breaking Changes`
  2. `### Feature`
  3. `### Bug`
- Keep bullet points short, one change per line.
- If no entries are available in a category, use `- none`.
- Put incompatible changes only in `Breaking Changes`.

## Migration from Existing CHANGELOG.md
1. Keep existing changelog files unchanged.
2. Create or update `release_notes.md`.
3. For each version, map entries into required categories.
4. When category cannot be determined, default to `Bug` and refine later.

## Reusable AI Prompt

For migration in additional projects, use:

- `doc/Release-Notes-Conversion.prompt.md`

This prompt covers extraction from `CHANGELOG.md`, `README.md`, and application docs, then merges entries into the required machine-readable format.

## Examples

Example with no breaking changes:

```markdown
## 0.6.0
### Breaking Changes
- none

### Feature
- Added KNX reconnect diagnostics.

### Bug
- Fixed timezone offset handling for sunrise calculation.
```

Example with breaking changes:

```markdown
## 1.4.0
### Breaking Changes
- Changed KO numbering for core objects 2-19.

### Feature
- Added new GPIO abstraction for expanders.

### Bug
- Fixed memory layout overlap detection output.
```
