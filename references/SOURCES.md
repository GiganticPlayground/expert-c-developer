# Source Library — Vetted References for the expert-c-developer Skill

Every rule, pattern, or gotcha documented in this skill must cite one of the
sources below. Sources are tiered by authority. Licensing notes govern what we
may copy vs. what we must paraphrase-and-cite.

**House rule:** we write all guidance and all code examples in our own words
(examples are original, MIT-licensed). Sources are cited per-rule, e.g.
`(CERT INT32-C)` or `(Power of Ten, Rule 5)`. We never bulk-copy source text.

---

## Tier 1 — Normative / canonical references

### ISO C Standard working drafts (WG14)
- **C23**: N3220 working draft — <https://www.open-std.org/jtc1/sc22/wg14/www/docs/n3220.pdf>
  (equivalent to ISO/IEC 9899:2024 except editorial fixes; **the draft all
  rules cite** — section numbers follow N3220, which renumbered some C17
  clauses)
- **C11**: N1570 working draft — <https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1570.pdf>
  (background/fallback; rules cite N3220)
- WG14 project page (background): <https://www.open-std.org/jtc1/sc22/wg14/>
- **Credibility:** the language definition itself. Final authority on syntax and
  defined/undefined behavior.
- **License:** working drafts are freely distributed by WG14 for review; cite by
  document number and section (e.g. "N3220 §6.5.6"). Do not reproduce large runs of text.

### SEI CERT C Coding Standard (CMU Software Engineering Institute)
- Canonical home: <https://cmu-sei.github.io/secure-coding-standards/sei-cert-c-coding-standard/>
  (the standard migrated here from the legacy Confluence wiki at
  wiki.sei.cmu.edu, which now redirects and is often unreachable directly —
  cite the new site)
- Per-rule URL patterns:
  `…/sei-cert-c-coding-standard/rules/<section-slug>/<id>/` (IDs 30+) and
  `…/sei-cert-c-coding-standard/recommendations/<section-slug>/<id>/` (IDs 00–29)
- 2016 edition PDF (background; rules cite the live per-rule pages):
  <https://resources.sei.cmu.edu/library/asset-view.cfm?assetid=494934>
- **Credibility:** the de-facto public secure-coding standard for C; maintained
  by CMU SEI with community review; each rule has risk assessment, compliant and
  noncompliant examples, and CVE linkage.
- **License:** © Carnegie Mellon University; free to read online. Cite rule IDs
  (e.g. ARR30-C, INT32-C, STR31-C) and paraphrase; link to the rule page.

### POSIX.1-2017 (The Open Group Base Specifications, Issue 7, 2018 edition)
- <https://pubs.opengroup.org/onlinepubs/9699919799/>
- **Credibility:** the normative POSIX standard itself, published free-to-read
  by The Open Group. Authority for OS-interface semantics (`read`/`write`
  partial transfers, `EINTR`, `errno` guarantees ISO C doesn't make, signal
  behavior).
- **License:** free online access; cite function pages by URL, paraphrase.

### cppreference.com (C section)
- <https://en.cppreference.com/w/c>
- **Credibility:** community-maintained but rigorously standard-tracking; the
  working programmer's canonical library/semantics reference.
- **License:** CC-BY-SA 3.0 — attribution + share-alike required if we copy;
  prefer paraphrase + link.

---

## Tier 2 — Institutional style & safety standards

### Recommended C Style and Coding Standards ("Indian Hill" standard) ⭐ seed source
- <https://www.doc.ic.ac.uk/lab/cplus/cstyle.html>
- **Provenance:** originated at AT&T Indian Hill labs; updated by Henry Spencer
  (U. Toronto), David Keppel (UCB/UW), and Mark Brader (SoftQuad). Hosted by
  Imperial College London.
- **Credibility:** classic, widely-cited Bell Labs lineage. **Caveat: dated**
  (late 1980s — pre-C99). Excellent for timeless structure/commenting/naming
  principles; do not use for language-feature guidance (K&R prototypes, lint-era
  tooling). Where it conflicts with C99+ practice, modern sources win and we say so.
- **License:** no explicit license; treat as cite-and-paraphrase only.

### Linux Kernel Coding Style
- <https://www.kernel.org/doc/html/latest/process/coding-style.html>
- **Credibility:** governs the largest long-lived C codebase in the world;
  opinionated but battle-tested (naming, function size, indentation, goto-cleanup).
- **License:** GPL-2.0 (kernel documentation). Paraphrase + cite; note where its
  conventions are kernel-specific (e.g. 8-space tabs, no typedefs for structs).

### The Power of Ten — Rules for Developing Safety-Critical Code (NASA/JPL)
- Gerard J. Holzmann, NASA/JPL Laboratory for Reliable Software, 2006.
- **Author's PDF (the URL cited by all rules):** <https://spinroot.com/gerard/pdf/P10.pdf>
  (spinroot.com is Holzmann's own site)
- Background overview only (not cited by rules):
  <https://en.wikipedia.org/wiki/The_Power_of_10:_Rules_for_Developing_Safety-Critical_Code>
- Original publication: "The Power of 10: Rules for Developing Safety-Critical
  Code", IEEE Computer 39(6), 2006.
- **Credibility:** authored by the head of JPL's software reliability lab;
  incorporated into JPL flight-software standards. Ten memorable, checkable rules.
- **License:** published paper; the ten rules themselves are facts/ideas — restate
  with attribution.

### BARR-C:2018 Embedded C Coding Standard (Barr Group)
- <https://barrgroup.com/embedded-c-coding-standard>
- **Credibility:** the most widely adopted embedded-C style standard; each rule
  is justified by defect-prevention data; deliberately MISRA-compatible.
- **License:** free PDF but **redistribution prohibited** (© Barr Group, all
  rights reserved). Never copy text or embed the PDF; paraphrase + cite rule
  numbers and link to Barr Group's page.

### University of Maryland C Style Guide (CS course standard)
- <https://www.cs.umd.edu/~nelson/classes/resources/cstyleguide/>
- **Provenance:** University of Maryland CS department course style guide,
  referencing Dr. Neil Spring.
- **Credibility:** solid institutional teaching standard; highly prescriptive
  and beginner-safe (mandatory braces, mandatory comments, naming rules, code
  organization order). **Caveats:** it is a *course* guide — some rules exist
  for gradability rather than engineering merit, and it assumes C89/C99-era
  practice. Use for baseline style/organization rules; defer to Tier 1 and
  kernel/BARR-C where they conflict.
- **License:** no explicit license; cite-and-paraphrase only.

### GNU Coding Standards (C sections)
- <https://www.gnu.org/prep/standards/>
- **Credibility:** FSF-maintained, governs GNU project C. Useful counterpoint on
  formatting/portability; some conventions are GNU-idiosyncratic.
- **License:** GFDL — paraphrase + cite.

### OpenSSF Compiler Options Hardening Guide for C and C++
- <https://best.openssf.org/Compiler-Hardening-Guides/Compiler-Options-Hardening-Guide-for-C-and-C++.html>
- **Provenance:** OpenSSF (Linux Foundation) Best Practices Working Group.
- **Credibility:** the current authority on GCC/Clang hardening flags —
  actively maintained (updated 2026-08), tracks new compiler releases, explains
  the *why* behind every flag, distinguishes dev vs. production builds.
  Primary source for `tooling.md`.
- **License:** OpenSSF best-practices docs are published openly (repo:
  `ossf/wg-best-practices-os-developers`, CC-BY-4.0); attribute when adapting.

### MISRA C — *excluded, referenced by name only*
- The automotive/safety standard. **Paywalled**, so per our ground rules it is
  not a source; we may mention that BARR-C and Power of Ten are aligned with it.

---

## Tier 3 — Expert-authored, freely available books & FAQs

### Modern C (Jens Gustedt) — C23 edition
- <https://gustedt.gitlabpages.inria.fr/modern-c/> (free PDF via Inria/HAL)
- **Credibility:** Gustedt is an INRIA senior scientist and an active **ISO WG14
  committee member**; the book is the best free treatment of modern (C17/C23)
  idiomatic C. Publisher: Manning.
- **License:** book is CC BY-NC-ND 4.0 (no derivatives — paraphrase + cite, don't
  excerpt); companion code is MIT.

### MaJerle/c-code-style (Tilen Majerle) — *scoped: style & format enforcement only*
- <https://github.com/MaJerle/c-code-style>
- **Provenance:** Tilen Majerle, prolific embedded-C OSS author (LwESP/LwGPS
  and other widely used STM32-ecosystem libraries). ~1.3k stars, actively
  maintained; ships a `.clang-format` (v20), CI format-check workflow, and
  file templates.
- **Credibility:** an individual's guide, so it does NOT outrank Tier 1–2 —
  but unlike rejected personal guides it is active, author-endorsed, RFC
  2119-style normative, and backed by enforcement tooling. Several positions
  are opinionated/embedded-idiosyncratic and are **overruled** by our higher
  tiers where they conflict (e.g. it rejects `bool`/`<stdbool.h>`, which
  contradicts C11+ practice and Modern C; its VLA ban we keep, agreeing with
  Power of Ten Rule 3).
- **Scope of use:** `style.md` conventions and `tooling.md` clang-format
  enforcement only. Never a source for correctness/security/UB claims.
- **License:** MIT — we may adapt (e.g. seed our `.clang-format` from his)
  with attribution.

### comp.lang.c FAQ (Steve Summit)
- <https://c-faq.com/>
- **Credibility:** the canonical Usenet-era FAQ; unmatched catalog of classic
  gotchas (null pointers, arrays vs. pointers, sequence points, malloc misuse).
  Predates C99, so verify against N3220/cppreference for modern semantics.
- **License:** free for personal use only — cite question numbers (e.g. "c-faq
  §6.2") and paraphrase.

---

## Tier 4 — First-party tool documentation (authoritative for the tool itself)

Cited for what a tool does and how to run it — never for language-semantics
claims.

### LLVM/Clang docs (Apache-2.0 w/ LLVM exceptions)
- clang-format: <https://clang.llvm.org/docs/ClangFormat.html>
  (style options: <https://clang.llvm.org/docs/ClangFormatStyleOptions.html>)
- clang-tidy: <https://clang.llvm.org/extra/clang-tidy/>
- AddressSanitizer: <https://clang.llvm.org/docs/AddressSanitizer.html>
- UndefinedBehaviorSanitizer: <https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html>
- libFuzzer: <https://llvm.org/docs/LibFuzzer.html>

### GNU docs (GFDL)
- GCC warning options: <https://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html>
- GCC static analyzer: <https://gcc.gnu.org/onlinedocs/gcc/Static-Analyzer-Options.html>
- GCC preprocessor options (dependency generation):
  <https://gcc.gnu.org/onlinedocs/gcc/Preprocessor-Options.html>
- GNU Make manual: <https://www.gnu.org/software/make/manual/make.html>

### Other tools
- Valgrind manual: <https://valgrind.org/docs/manual/quick-start.html>
- AFL++: <https://aflplus.plus/>
- Bear (compile_commands.json for Make): <https://github.com/rizsotto/Bear>

---

## Evaluated and rejected

### mcinglis/c-style (GitHub)
- <https://github.com/mcinglis/c-style> — CC-BY-SA 4.0, ~2.2k stars.
- **Verdict: not a source**, despite the friendly license. Three strikes:
  (1) the author's own README disclaimer — "This document is very old, and I no
  longer endorse it"; (2) the repo was archived read-only in Jan 2026; (3) it
  is a self-described personal/subjective guide with several positions contrary
  to mainstream practice and to our Tier 1–2 sources (avoid `switch` entirely,
  reject `unsigned` types, avoid opaque pointers/incomplete types, no VLAs even
  in C99 code). An author-disavowed personal guide fails the "no random devs"
  bar regardless of popularity.
- **Salvage:** its bibliography independently corroborates sources we already
  use (SEI CERT, OpenSSF hardening guide, Rob Pike's style notes).

---

## Explicitly not sources

- Random personal blogs, Medium posts, and unattributed tutorial sites.
- **Stack Overflow / Stack Exchange — HARD RULE, never use.** Not as a cited
  source, not as a code-example source, not laundered through a secondary
  source that merely repackages SO answers. Primary reason: answers are
  unvetted and frequently wrong or outdated. Secondary reason: SO content is
  CC-BY-SA, whose share-alike term is incompatible with our MIT-licensed code
  examples (our BY-SA prose could technically absorb it, but the quality bar
  is the rule regardless). If a claim can only be traced to Stack Overflow, it
  does not go in this skill — find it in a Tier 1–3 source or drop it.
- W3Schools-style beginner references.
- Paywalled standards (MISRA C, official ISO PDFs) — cited by name only.
- LLM-generated content without a verifiable source behind it.
