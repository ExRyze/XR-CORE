# Contributing

## Changes

- Keep firmware responsibilities in the corresponding module files; use `XR-CORE.ino` for composition and task orchestration.
- Keep source code and documentation in English.
- Keep frontend source in `local web/`; generated production assets belong in `data/`.
- Do not commit credentials, private keys, or deployment-specific certificates.
- Preserve the partition layout assumptions shared by firmware and filesystem uploads.

## Before submitting

- Compile with the documented board profile and record any dependency version changes.
- For frontend changes, run `npm run build` in `local web/` and inspect the generated `data/` output.
- For logo changes, regenerate and inspect `data/logo.raw` and `tools/logo_preview.png`.
- Describe hardware or runtime validation that was actually performed; do not imply device validation from a successful compile alone.
