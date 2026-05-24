# Prompt: Convert Legacy Release Notes To release_notes.md

Use this prompt with an AI coding assistant to migrate historical release information into the machine-readable `release_notes.md` format.

## Copy/Paste Prompt

```text
You are working in a repository with multiple module projects under lib/.

Goal:
Convert all legacy release information to the new machine-readable release notes format.

Target format for each module file `release_notes.md`:

# Release Notes

## <version>
### Feature
- ...
### Bug
- ...
### Breaking Changes
- ...

Requirements:
1. Search all possible legacy sources for release history, including:
   - CHANGELOG.md
   - README.md sections like "Release Notes", "Changelog", "Änderungshistorie"
   - application documentation files, e.g. doc/Applikationsbeschreibung*.md
2. Merge all found history entries into each module's `release_notes.md`.
3. Keep existing entries and only add missing information.
4. Deduplicate identical entries.
5. Map entries into categories:
   - Feature: new functionality, improvements, refactorings, docs (if no bug context)
   - Bug: fixes, hotfixes, error corrections
   - Breaking Changes: incompatible behavior or migration-impacting changes
6. If a category has no entries for a version, set `- none`.
7. Preserve version ordering from newest to oldest where possible.
8. Keep wording close to original source text; do not invent new release facts.
9. After migration, update README sections that still carry old release lists to a short pointer:
   - "Die vollständige Historie liegt in [release_notes.md](release_notes.md)."
10. Validate:
   - every module under lib has `release_notes.md`
   - each version has all three headings: Feature, Bug, Breaking Changes
   - scripts/parsers consuming release_notes still work

Output:
- Apply all file edits directly.
- Provide a final summary with:
  - which modules were changed
  - which legacy sources were consumed per module
  - any ambiguities or entries that could not be classified safely
```

## Notes

- Recommended usage scope: one repository at a time.
- If module history is very long, migrate in phases (recent releases first, then older history).
- Keep this prompt in sync with `Release-Notes-Guide.md`.
