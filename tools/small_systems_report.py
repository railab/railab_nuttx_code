#!/usr/bin/env python3
############################################################################
# tools/small_systems_report.py
#
# Build all "Apache NuttX and small systems" configurations and write
# a memory usage report (Markdown).
#
# Usage:
#
#   ./tools/small_systems_report.py [-j JOBS] [-o report.md]
#
# Requires: cmake, ninja, arm-none-eabi-gcc, kconfiglib
############################################################################

import argparse
import concurrent.futures
import datetime
import os
import re
import shutil
import statistics
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
BOARD = "boards/arm/stm32/nucleo-f302r8-mini"
CONFIGS = os.path.join(ROOT, BOARD, "config")
BASELINE = os.path.join(ROOT, "tools", "small_systems_baseline.txt")
REFERENCE = os.path.join(ROOT, "tools", "small_systems_reference.txt")

# Allowed growth against the reference before a config is reported

THRESHOLD = 64
VARPREFIX = "_report_"

# Config used to measure the effect of single options

OPTION_BASE = "mini_hello_opt"

# Defconfig options that are expected to be dropped by Kconfig:
#   - the custom board selection, the in-tree nucleo-f302r8 board is used,
#   - signal action pools, they exist only with CONFIG_ENABLE_ALL_SIGNALS.

EXPECTED_DROPS = re.compile(
    r"CONFIG_ARCH_BOARD_CUSTOM|CONFIG_SIG_(PRE)?ALLOC_ACTIONS"
)

# Options added after the 2024 series (applied one by one on OPTION_BASE)

OPTIONS = [
    ["CONFIG_DISABLE_ALL_SIGNALS=y"],
    ["CONFIG_LIBC_PRINTF_MINIMAL=y"],
    ["CONFIG_SYSLOG_NONE=y"],
    ["CONFIG_TLS_EMUTLS=y"],
    ["# CONFIG_TLS_EMUTLS is not set"],
]


def run(cmd, cwd=None):
    return subprocess.run(
        cmd, cwd=cwd, capture_output=True, text=True, shell=isinstance(cmd, str)
    )


def git_head(path):
    r = run(["git", "-C", path, "log", "-1", "--format=%H %cd", "--date=short"])
    return r.stdout.strip() or "unknown"


def symbol(line):
    m = re.search(r"CONFIG_[A-Z0-9_]+", line)
    return m.group(0) if m else None


def config_lines(path):
    with open(path) as f:
        return [
            line.rstrip("\n")
            for line in f
            if re.match(r"(# )?CONFIG_[A-Z0-9_]+(=| is not set)", line)
        ]


def mem_usage(log):
    res = {}
    for name in ("flash", "sram"):
        m = re.findall(r"^\s*%s:\s+(\d+)\s+(B|KB)" % name, log, re.M)
        if m:
            val, unit = m[-1]
            res[name] = int(val) * (1024 if unit == "KB" else 1)
    return res


def build(nuttx, name, builddir):
    """Build one config, return dict with sizes, errors and dropped options"""

    b = os.path.join(builddir, name)
    shutil.rmtree(b, ignore_errors=True)
    cfg = os.path.join(CONFIGS, name)
    r1 = run(["cmake", "-B", b, "-S", nuttx, "-DBOARD_CONFIG=" + cfg, "-GNinja"])
    log = r1.stdout + r1.stderr
    ok = r1.returncode == 0
    if ok:
        r2 = run(["cmake", "--build", b])
        log += r2.stdout + r2.stderr
        ok = r2.returncode == 0

    res = {"name": name, "ok": ok}
    res.update(mem_usage(log))
    if not ok or "flash" not in res:
        res["ok"] = False
        errs = [
            line.strip()
            for line in log.splitlines()
            if re.search(r"error:|undefined reference|Error:", line)
        ]
        res["errors"] = list(dict.fromkeys(errs))[:5]

    # Options from defconfig that are not in the resolved .config

    dot = os.path.join(b, ".config")
    if os.path.exists(dot):
        got = set(config_lines(dot))
        res["dropped"] = [
            line
            for line in config_lines(os.path.join(cfg, "defconfig"))
            if line not in got and "BASE_DEFCONFIG" not in line
            and not EXPECTED_DROPS.search(line)
        ]
        res["config"] = got

    with open(os.path.join(builddir, name + ".log"), "w") as f:
        f.write(log)
    shutil.rmtree(b, ignore_errors=True)
    return res


def make_variant(base, name, extra):
    d = os.path.join(CONFIGS, name)
    os.makedirs(d, exist_ok=True)
    syms = {symbol(x) for x in extra}
    lines = [
        x for x in config_lines(os.path.join(CONFIGS, base, "defconfig"))
        if symbol(x) not in syms
    ]
    with open(os.path.join(d, "defconfig"), "w") as f:
        f.write("\n".join(lines + extra) + "\n")
    return name


def load_baseline(path=BASELINE):
    base = {}
    if os.path.exists(path):
        with open(path) as f:
            for line in f:
                p = line.split()
                if len(p) >= 3 and not line.startswith("#"):
                    base[p[0]] = (int(p[1]), int(p[2]))
    return base


def fmt(r, ref=None):
    if not r or not r["ok"]:
        return "FAIL"
    s = "%d / %d" % (r["flash"], r["sram"])
    if ref:
        s += " (%+d / %+d)" % (r["flash"] - ref[0], r["sram"] - ref[1])
    return s


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--nuttx", default=os.path.join(ROOT, "external/nuttx"))
    ap.add_argument("--apps", default=os.path.join(ROOT, "external/nuttx-apps"))
    ap.add_argument("-j", "--jobs", type=int, default=max(1, os.cpu_count() // 4))
    ap.add_argument("-o", "--output", default="small_systems_report.md")
    ap.add_argument("--builddir", default=os.path.join(ROOT, "build/report"))
    ap.add_argument("--update-reference", action="store_true",
                    help="save the results as the new reference")
    args = ap.parse_args()

    # Link our applications into nuttx-apps

    link = os.path.join(args.apps, "external")
    if not os.path.exists(link):
        os.symlink(os.path.relpath(os.path.join(ROOT, "apps"), args.apps), link)

    os.makedirs(args.builddir, exist_ok=True)

    names = sorted(
        d for d in os.listdir(CONFIGS)
        if not d.startswith(VARPREFIX) and os.path.isdir(os.path.join(CONFIGS, d))
    )

    variants = {}
    for opt in OPTIONS:
        n = VARPREFIX + re.sub(r"[^a-z0-9]+", "_", " ".join(opt).lower()).strip("_")
        variants[n] = (opt, make_variant(OPTION_BASE, n, opt))

    try:
        todo = names + list(variants)
        results = {}
        with concurrent.futures.ThreadPoolExecutor(args.jobs) as ex:
            futs = {ex.submit(build, args.nuttx, n, args.builddir): n for n in todo}
            for i, f in enumerate(concurrent.futures.as_completed(futs), 1):
                r = f.result()
                results[r["name"]] = r
                print("[%d/%d] %s %s" % (i, len(todo), r["name"], fmt(r)),
                      file=sys.stderr)
    finally:
        for n in variants:
            shutil.rmtree(os.path.join(CONFIGS, n), ignore_errors=True)

    write_report(args, names, variants, results)


def write_report(args, names, variants, results):
    base = load_baseline()
    ported = [n for n in names if not n.endswith("_tuned")]
    out = []
    w = out.append

    w("# NuttX small systems report\n")
    w("* date: %s" % datetime.date.today())
    w("* nuttx: `%s`" % git_head(args.nuttx))
    w("* nuttx-apps: `%s`" % git_head(args.apps))
    w("* railab_nuttx_code: `%s`" % git_head(ROOT))
    w("* toolchain: `%s`" % run(["arm-none-eabi-gcc", "--version"])
      .stdout.splitlines()[0])
    w("")

    # Checks

    failed = [n for n in names if not results[n]["ok"]]
    dropped = {n: results[n].get("dropped") for n in names
               if results[n].get("dropped")}
    checks = []
    checks.append(("All configurations build", not failed,
                   ", ".join(failed) or "%d configs" % len(names)))
    ref = load_baseline(REFERENCE)
    grown = ["%s %+d B" % (n, results[n]["flash"] - ref[n][0])
             for n in names
             if n in ref and results[n]["ok"]
             and results[n]["flash"] - ref[n][0] > THRESHOLD]
    checks.append(("No FLASH growth over %d B vs reference" % THRESHOLD,
                   not grown, ", ".join(grown) or
                   ("%d configs" % len(ref) if ref else "no reference")))
    checks.append(("No defconfig options silently dropped", not dropped,
                   "; ".join("%s: %s" % (k, ", ".join(v))
                             for k, v in dropped.items()) or "-"))

    h = results.get(OPTION_BASE)
    if h and "config" in h:
        tls = "# CONFIG_TLS_EMUTLS is not set" in h["config"] or not any(
            x.startswith("CONFIG_TLS_EMUTLS") for x in h["config"])
        checks.append(("TLS_EMUTLS off by default with DEFAULT_SMALL", tls,
                       OPTION_BASE))

    for name, desc in (("mini_core2", "STM32 image without U(S)ART links"),
                       ("mini_components4_tim",
                        "Full signals with SIG_PREALLOC_ACTIONS=0 link")):
        if name in results:
            checks.append((desc, results[name]["ok"], name))

    w("## Checks\n")
    w("| Check | Result | Details |")
    w("|---|---|---|")
    for desc, ok, det in checks:
        w("| %s | %s | %s |" % (desc, "PASS" if ok else "**FAIL**", det))
    w("")

    # Results

    w("## Results\n")
    w("FLASH / SRAM in bytes. Differences to the 2024 baseline "
      "(`tools/small_systems_baseline.txt`) in parentheses.\n")
    w("| Config | 2024 | ported | tuned |")
    w("|---|---|---|---|")
    for n in ported:
        ref = base.get(n)
        w("| `%s` | %s | %s | %s |" % (
            n, "%d / %d" % ref if ref else "-", fmt(results[n], ref),
            fmt(results.get(n + "_tuned"), ref)))
    w("")

    for label, suffix in (("ported", ""), ("tuned", "_tuned")):
        df = [results[n + suffix]["flash"] - base[n][0] for n in ported
              if n in base and results.get(n + suffix, {}).get("ok")]
        dr = [results[n + suffix]["sram"] - base[n][1] for n in ported
              if n in base and results.get(n + suffix, {}).get("ok")]
        if df:
            w("* %s vs 2024: FLASH median %+d B (min %+d, max %+d), "
              "SRAM median %+d B" % (label, statistics.median(df), min(df),
                                     max(df), statistics.median(dr)))
    w("")

    # Options

    w("## Options on `%s`\n" % OPTION_BASE)
    w("| Option | FLASH | SRAM |")
    w("|---|---|---|")
    for n, (opt, _) in variants.items():
        r = results[n]
        if r["ok"] and h and h["ok"]:
            w("| `%s` | %+d B | %+d B |" % (", ".join(opt),
                                            r["flash"] - h["flash"],
                                            r["sram"] - h["sram"]))
        else:
            w("| `%s` | FAIL | |" % ", ".join(opt))
    w("")

    # Errors

    errs = [n for n in results if not results[n]["ok"]]
    if errs:
        w("## Build errors\n")
        for n in errs:
            w("### %s\n" % n)
            w("```")
            w("\n".join(results[n].get("errors") or ["(see log)"]))
            w("```")
            w("Log: `%s`\n" % os.path.join(args.builddir, n + ".log"))

    with open(args.output, "w") as f:
        f.write("\n".join(out) + "\n")

    if args.update_reference:
        with open(REFERENCE, "w") as f:
            f.write("# nuttx %s\n# nuttx-apps %s\n# config flash sram\n" %
                    (git_head(args.nuttx), git_head(args.apps)))
            for n in names:
                if results[n]["ok"]:
                    f.write("%s %d %d\n" % (n, results[n]["flash"],
                                             results[n]["sram"]))
        print("reference updated: %s" % REFERENCE, file=sys.stderr)
    print("report: %s" % os.path.abspath(args.output), file=sys.stderr)
    sys.exit(0 if all(c[1] for c in checks) else 1)


if __name__ == "__main__":
    main()
