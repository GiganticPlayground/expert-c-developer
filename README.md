# expert-c-developer

**A C engineering knowledge base for AI coding agents — where every rule is
cited, every example compiles, and every claim survived adversarial review.**

![C17](https://img.shields.io/badge/C-C17%20baseline%2C%20C23%20aware-blue)
![Examples](https://img.shields.io/badge/examples-ASan%20%2B%20UBSan%20clean-brightgreen)
![Links](https://img.shields.io/badge/citations-136%20URLs%20verified-brightgreen)
![License](https://img.shields.io/badge/license-CC%20BY--SA%204.0%20%2B%20MIT-orange)

LLMs write C the way the internet taught them to — and the internet is full of
`strncpy` "safety", `assert(fopen(...))`, unchecked `malloc(count * size)`,
and overflow checks the optimizer silently deletes. This skill replaces that
folklore with the sources that actually govern C: **the ISO standard, SEI CERT,
the Linux kernel, NASA/JPL, OpenSSF** — distilled into ~75 rules an agent can
apply and a human can verify.

## The three guarantees

Most coding guidelines ask you to trust them. This one shows receipts:

1. **Every rule is cited — with a full URL to the exact source.** Not "best
   practice says", but `CERT INT32-C`, `N3220 §6.5`, `Power of Ten, Rule 5` —
   each linked to the authoritative page, each source vetted for credibility
   and licensing in [references/SOURCES.md](references/SOURCES.md).
   `scripts/check-links.sh` proves all 136 URLs resolve.

2. **Every behavioral rule has a complete, runnable proof.** The docs teach
   with minimal fragments, but each rule links a `demo_*()` function in
   [`examples/`](examples/) — a real program that asserts the claimed
   behavior. `scripts/validate-examples.sh` compiles all of them with
   `-Wall -Wextra -Werror -Wconversion -Wsign-conversion …` plus
   AddressSanitizer and UBSan, and runs them. If an example is wrong, **the
   repo fails its own build.**

3. **It was adversarially reviewed** — by an expert-C-developer pass that
   verified section numbers against the actual N3220 PDF and CERT rule IDs
   against the live standard (and caught real errors, which were fixed), and a
   technical-documentation pass that verified every internal link, every
   cross-reference, and every format contract.

## What's inside

| File | What it covers |
|---|---|
| [SKILL.md](SKILL.md) | The agent entry point: defaults, non-negotiable workflow, routing table |
| [references/gotchas.md](references/gotchas.md) | 17 classic pitfalls — precedence traps, `sizeof` decay, signed/unsigned, integer promotion, `strncpy`, `assert` misuse |
| [references/memory.md](references/memory.md) | Ownership discipline, overflow-checked allocation, `realloc` done right, lifetime pitfalls |
| [references/error-handling.md](references/error-handling.md) | Return-value discipline, `errno`, the goto-cleanup idiom, `EINTR`/partial I/O, signal safety |
| [references/security.md](references/security.md) | CERT-derived: bounded writes, format strings, injection, TOCTOU, secrets, CSPRNGs |
| [references/undefined-behavior.md](references/undefined-behavior.md) | The optimizer's contract: aliasing, alignment, pointer arithmetic, data races — and the defined alternative to each |
| [references/patterns.md](references/patterns.md) | Opaque types, designated initializers, lifecycle pairs, flexible array members, intrusive lists, arenas |
| [references/style.md](references/style.md) | Naming, layout, headers, comments — with formatting delegated to clang-format |
| [references/tooling.md](references/tooling.md) | Warning baseline, sanitizers, hardening flags (OpenSSF-current), static analysis, fuzzing, build integration |
| [references/SOURCES.md](references/SOURCES.md) | The annotated bibliography: every source, its credibility tier, its license, and what it may be cited for |
| [examples/](examples/) | Six complete programs, 58 demos, every one asserting its rule's behavior |
| [scripts/](scripts/) | The two self-tests: link verification and example validation |

Defaults: **write C17-compatible code, note C23 niceties where toolchains
support them; general-purpose (POSIX/desktop) conventions first, with the
stricter embedded rules (BARR-C, Power of Ten) applied when the target is
firmware or safety-critical.**

## What this is NOT

Honesty about scope is part of the credibility:

- **Not a linter or a tool.** It's knowledge. Pair it with the tools it
  recommends (clang-tidy, sanitizers, clang-format) — it tells you exactly
  which and how.
- **Not a C tutorial.** It assumes you (or your agent) can already write C;
  it exists to stop the specific, well-documented ways C goes wrong.
- **Not MISRA compliance.** MISRA is paywalled and therefore excluded by this
  project's ground rules; BARR-C and the Power of Ten (both MISRA-aligned and
  freely available) carry the safety-critical perspective instead.
- **Not scraped from Stack Overflow.** SO is banned here as a source — by
  hard rule — along with blogs, tutorial mills, and anything unvetted.
  Guides that failed vetting are documented *as rejected, with reasons* in
  [SOURCES.md](references/SOURCES.md).
- **Not for C++, C#, or Objective-C.** C only.

## How to use it

### With Claude Code (or any SKILL.md-compatible agent)

Clone into your skills directory — per-project or global:

```bash
git clone https://github.com/GiganticPlayground/expert-c-developer.git .claude/skills/expert-c-developer
```

That's it. The agent loads [SKILL.md](SKILL.md) when C work triggers it, and
pulls in the specific reference file the task needs (the routing table in
SKILL.md maps task → file). The skill's workflow rules make the agent compile
at high warning levels, run sanitizers before claiming correctness, and cite
its sources when it makes style or correctness rulings — so you can check its
reasoning instead of trusting it.

### As a human reference

The reference files stand alone as a sourced C review checklist. Start with
[gotchas.md](references/gotchas.md) — if you review C, it's the list you're
already carrying in your head, now with citations and proofs.

### Verify everything yourself

```bash
scripts/validate-examples.sh
```

```bash
scripts/check-links.sh
```

The first compiles and runs all examples under the skill's own warning
baseline plus ASan+UBSan; it exits nonzero on any failure and runs as a hard
CI gate (see [.github/workflows/ci.yml](.github/workflows/ci.yml)). The
second checks every cited URL in parallel — it's authoritative when run
locally, but **advisory in CI**: several authoritative sources
(cppreference.com, Barr Group, c-faq.com) bot-block datacenter IPs with 403s
while serving browsers fine, so the CI leg reports rather than gates.

**Platforms and dependencies.** The scripts are **bash, for Linux and macOS**:

- `validate-examples.sh` needs a **C compiler with sanitizer support** —
  clang (the default `cc` on macOS) or gcc; override with
  `CC=gcc-14 scripts/validate-examples.sh`. Everything else is standard Unix
  tooling.
- `check-links.sh` needs **curl** (preinstalled on macOS and most Linux
  distros).

**On Windows:** run everything under **WSL** for full parity — the example
programs deliberately exercise POSIX APIs (pipes, `sigaction`, `mkdtemp`), so
they don't build with native Windows toolchains. The link checker, though, has
a **native PowerShell port** with no dependencies at all:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\check-links.ps1
```

## The rule format

Behavioral rules follow one uniform shape, so both agents and humans can
consume them predictably:

> **Rule** → **Why** (the mechanism, not just the commandment) → **Bad**
> (fragment) → **Good** (fragment) → **Runnable** (link to the proving
> `demo_*()`) → **Source** (full URLs)

BAD forms that are undefined behavior exist only as fragments — they must
never execute. The runnable examples prove the GOOD forms, with a few
*defined-but-wrong* forms executed deliberately to show them misbehave.

## Contributing

Contributions are welcome — the bar is high on purpose, and it's mechanical:

1. **Every new rule needs a Tier 1–3 source** from
   [SOURCES.md](references/SOURCES.md), cited with a full URL to the exact
   rule/section. Want to use a new source? Add it to SOURCES.md first with
   provenance, credibility assessment, and license terms — that's a
   contribution in itself. Personal blogs and Stack Overflow will be declined
   regardless of how correct they are; find the claim in a real source or it
   doesn't ship.
2. **Behavioral rules need a runnable demo** — a `demo_*()` in the matching
   `examples/*.c` that asserts the claimed behavior, passing
   `scripts/validate-examples.sh` (strict warnings + ASan/UBSan, assertions
   required, no exceptions).
3. **Both scripts must pass** before any PR:
   `scripts/validate-examples.sh && scripts/check-links.sh`.
4. **Write in your own words.** Several sources (BARR-C, Modern C, c-faq,
   CERT) prohibit copying — paraphrase and cite, never paste. All code you
   contribute must be original and MIT-licensable.
5. **Corrections outrank additions.** A PR proving a rule wrong against the
   standard, with the N3220 section to back it, is the most valuable
   contribution this repo can receive.

Good first contributions: the sourced entries we've explicitly deferred —
`restrict` semantics, struct padding vs. `memcmp`/serialization — or a GitHub
Actions workflow running both scripts on Linux + macOS with gcc + clang.

## Licensing

Dual-licensed by content type — see [LICENSE.md](LICENSE.md) for the full
terms and the upstream-compatibility table:

- **Documentation/prose:** [CC BY-SA 4.0](https://creativecommons.org/licenses/by-sa/4.0/)
  (the most restrictive license among the upstreams we may lawfully adapt).
- **All code** (examples and scripts): MIT — paste it into anything.

## Acknowledgments

This project stands on the shoulders of the sources it cites: the ISO WG14
committee's freely published drafts, CMU SEI's CERT C Coding Standard, the
Linux kernel community, Gerard Holzmann (NASA/JPL), the Barr Group, the
OpenSSF Best Practices WG, Jens Gustedt's *Modern C*, Steve Summit's
comp.lang.c FAQ, and the maintainers of cppreference. Full credits, with
licenses and provenance, in [references/SOURCES.md](references/SOURCES.md).
