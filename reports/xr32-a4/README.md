# XR32 A4: 32-bit water on sloped coasts (testing-4)

> **Publication note (2026-11-02).** These reports certify the code of the pre-release
> `xr32-prissi-night-image-prototype-01-r12377-testing-4`, published with the maintainer's authorisation from the same
> frozen files: the game archives carry exactly the certified executables (Windows SDL2 `42e9089c…`, SDL3 `43420d63…`,
> Linux SDL2 `c0cec57e…`, SDL3 `7a556636…`, and testing-3's `simutrans-16bit`), and `source/xr32-r12377/` is the
> candidate's tree `96f3d410`. "Publication: NOT_AUTHORISED" in them is the state at the end of each certification.
> Pictures of third-party water art used in the tests are not included.
> Local machine paths are replaced by placeholders (`<a4-lab>`, `<a4c-lab>` and similar); `deliver/`, `evidence/`,
> `logs/` and `out/` refer to the labs, and selected evidence files are in `evidence/xr32-a4/`.

Two certifications, in order:

- **`a4/`: XR32-COAST-RENDERING-01**, the first A4 candidate (`84b3225`). Verdict B, certified with limitations.
  - Root cause: the sloped-coast water is generated from 16-bit words.
  - Design: a per-tile recipe, composed when a cache is built.
  - Composition oracle against an independent model; night judged relative to the sea image; zoom geometry; the
    regression battery; sanitizers; clean shutdown and memory.
  - Benchmarks: P99 +0.4–3 ms on frames that rebuild coast caches.
  - Some recommendations in `A4-MAINTAINER-REVIEW.md` (keep `release_above`, keep the counters while testing) were
    decided otherwise afterwards; see below.
- **`a4c/`: XR32-A4-PHASE-CACHE-OPTIMIZATION-01**, the testing-4 code (`a4/cache` `0afdfbd`). Verdict B, accepted.
  - `84b3225` without the measurement counters and the unreachable `release_above()`.
  - A per-stage texture cache: 4 entries, keyed by everything the first step of a composition reads.
  - Byte-identical pictures to `84b3225` on 11,247 coast-tile draws and 87,760 battery pictures.
  - Each composition costs 4–5 times less; P95/P99 fall against `84b3225` on every lane; P50 is unchanged.
  - Memory is equal or lower.
  - New finding: about 7 one-pixel sand gaps in the water at zoom 2/1 (accepted for testing-4 as known issue A11).
