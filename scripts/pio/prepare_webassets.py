# ---------------------------------------------------------------------------
#  Pre-build: embed web/assets/ into include/webassets.h.
#
#  CSS/JS/SVG/images from every built module + the project are minified, gzipped
#  and written as C arrays. Only runs for products with a webserver
#  (OPENKNX_WEBSERVER). A post-link nm report shows what actually shipped.
#  See OFM-Network/README.md.
# ---------------------------------------------------------------------------

Import("env")
import os
import sys
import re
import gzip
import hashlib
import inspect
import shutil
import pathlib
import subprocess
from SCons.Script import Action

sys.path.insert(0, next((p for p in ("lib/OGM-Common/scripts/pio", "scripts/pio")
                         if os.path.exists(os.path.join(p, "_pio_common.py"))), "."))
from _pio_common import C, step, resolve_project, section, human_bytes, quiet_action, G_RULE

# Gate: only for products that actually run a webserver.
_defines = set(str(d[0] if isinstance(d, (list, tuple)) else d) for d in env.get("CPPDEFINES", []))
if "OPENKNX_WEBSERVER" in _defines:
    project, lib_builders, basepath = resolve_project(env)

    WEBASSET_MIME = {".css": "text/css", ".js": "text/javascript", ".svg": "image/svg+xml",
                     ".jpg": "image/jpeg", ".jpeg": "image/jpeg", ".png": "image/png"}
    WEBASSET_FORMAT = "4"
    WEBASSET_HEADER = pathlib.Path("include/webassets.h")

    def _webasset_ident(rel):
        i = re.sub(r"[^0-9a-zA-Z_]", "_", rel)
        return "_" + i if i[:1].isdigit() else i

    def _webasset_min_css(t):
        t = re.sub(r"/\*.*?\*/", "", t, flags=re.DOTALL)
        t = re.sub(r"\s+", " ", t)
        t = re.sub(r"\s*([{}:;,])\s*", r"\1", t)
        return re.sub(r";}", "}", t).strip()

    def _webasset_min_js(t):
        # Whole-line // comments go too: they explain the source, and every device would carry them
        # in flash forever. Conservative on purpose -- a trailing // (which may sit inside a string
        # or a regex) and block comments are left alone, so no expression can change meaning.
        out = []
        for ln in t.splitlines():
            s = ln.strip()
            if not s or s.startswith("//"):
                continue
            out.append(s)
        return "\n".join(out)

    def _webasset_min_svg(t):
        t = re.sub(r"<!--.*?-->", "", t, flags=re.DOTALL)
        return re.sub(r">\s+<", "><", t).strip()

    WEBASSET_MINIFY = {".css": _webasset_min_css, ".js": _webasset_min_js, ".svg": _webasset_min_svg}

    def _webasset_collect():
        # Precise: walk the actual dependency tree (only modules really in the build) + the project.
        roots = []

        def walk(node):
            for lb in node.depbuilders:
                roots.append(pathlib.Path(lb.path))
                if lb.depbuilders:
                    walk(lb)

        walk(project)
        roots.append(pathlib.Path(env.subst("$PROJECT_DIR")))

        seen, entries = set(), {}
        for base in roots:
            d = (base / "web" / "assets").resolve()
            if not d.is_dir() or d in seen:
                continue
            seen.add(d)
            for f in sorted(d.rglob("*")):
                ext = f.suffix.lower()
                if not f.is_file() or ext not in WEBASSET_MIME:
                    continue
                ident = _webasset_ident(f.relative_to(d).as_posix())
                if ident in entries:
                    raise SystemExit("{}webassets: duplicate identifier '{}': {} vs {}{}".format(
                        C.RED, ident, entries[ident], f, C.END))
                entries[ident] = (ext, f)
        return entries

    def _webasset_generate():
        entries = _webasset_collect()
        if not entries:
            if WEBASSET_HEADER.is_file():
                WEBASSET_HEADER.unlink()
            return

        # The minifiers belong in the signature: changing one produces different bytes from unchanged
        # sources, and without this the header would keep the old ones forever. Hashing their source
        # (not a version constant) means nobody has to remember to bump anything.
        sig = hashlib.sha256(WEBASSET_FORMAT.encode())
        for fn in (_webasset_min_css, _webasset_min_js, _webasset_min_svg):
            try:
                sig.update(inspect.getsource(fn).encode())
            except (OSError, TypeError):
                pass
        for ident, (ext, f) in entries.items():
            sig.update("{}\0{}\0".format(ident, ext).encode())
            sig.update(hashlib.sha256(f.read_bytes()).digest())
        marker = "// webassets-sig: " + sig.hexdigest()

        if WEBASSET_HEADER.is_file():
            try:
                if marker in WEBASSET_HEADER.read_text().splitlines()[:2]:
                    return
            except OSError:
                pass

        step("Generate include/webassets.h")

        head = ["#pragma once", marker, "// webassets-list:"]
        body = []
        for ident, (ext, f) in entries.items():
            raw = f.read_bytes()
            data = WEBASSET_MINIFY[ext](raw.decode()).encode() if ext in WEBASSET_MINIFY else raw
            gz = gzip.compress(data, compresslevel=9, mtime=0)
            head.append("//   E {} {} {}".format(ident, len(gz), len(data)))
            rows = ["0x{:02x}".format(b) for b in gz]
            arr = "\n".join("    " + ",".join(rows[i:i + 20]) + "," for i in range(0, len(rows), 20))
            body += ["    inline const uint8_t {}_gz[] = {{".format(ident), arr, "    };",
                     '    inline const char* const {}_mime = "{}";'.format(ident, WEBASSET_MIME[ext]), ""]

        head += ["// Auto-generated by OGM-Common/scripts/pio/prepare_webassets.py from web/assets/ -- do not edit.",
                 "#include <cstddef>", "#include <cstdint>", "", "namespace WebAssets", "{"]
        WEBASSET_HEADER.parent.mkdir(parents=True, exist_ok=True)
        WEBASSET_HEADER.write_text("\n".join(head + body + ["} // namespace WebAssets", ""]))

    def _webasset_nm():
        # Windows carries a .exe suffix, so a substitution anchored at the end of the string misses --
        # and the untouched candidate is then the COMPILER itself, which exists and would be run with
        # nm's arguments. It answers nothing, every asset looks linker-removed, and the report lies.
        # Split the suffix off, substitute on the stem, put it back.
        cc = env.subst("$CC")
        stem, ext = os.path.splitext(cc)
        cands = [stem + "-nm" + ext,
                 re.sub(r"(gcc|g\+\+|cc)$", "nm", stem) + ext,
                 "nm" + ext, "nm"]
        for c in cands:
            if os.path.basename(c) == os.path.basename(cc):
                continue  # the substitution did not bite -- that is the compiler, not nm
            hit = c if (os.path.isabs(c) and os.path.exists(c)) else shutil.which(c)
            if hit:
                return hit
        return None

    def _webasset_report(target, source, env):  # SCons calls with keyword args -> names are fixed
        elf = next((str(n) for n in (*target, *source) if str(n).endswith(".elf")), None)
        manifest = {}
        try:
            for line in WEBASSET_HEADER.read_text().splitlines():
                if line.startswith("//   E "):
                    parts = line[7:].split()
                    ident, gz = parts[0], int(parts[1])
                    manifest[ident] = (gz, int(parts[2]) if len(parts) > 2 else 0)
        except OSError:
            pass
        nm = _webasset_nm()
        if not elf or not manifest or not nm:
            return
        try:
            out = subprocess.run([nm, "-C", elf], capture_output=True, text=True).stdout
        except (OSError, subprocess.SubprocessError):
            return
        present = set(re.findall(r"WebAssets::(\w+)_gz\b", out))
        section("Web assets", "(gzipped in flash - `raw` is the size after minifying, before gzip)")
        print("{}    {:<30}{:>10}{:>11}{}".format(C.GRAY, "", "flash", "raw", C.END))

        # Biggest first: the report exists to answer "what costs the space", and that answer is
        # useless in alphabetical order.
        kept = sorted(((gz, raw, i) for i, (gz, raw) in manifest.items() if i in present), reverse=True)
        in_bin = sum(gz for gz, _r, _i in kept)
        shown = kept[:14]
        for gz, raw, ident in shown:
            nm_ = "{:<30}".format(ident[:30])
            fl = "{:>10}".format(human_bytes(gz))
            rw = "{:>11}".format(human_bytes(raw) if raw else "-")
            big = gz >= 8 * 1024                       # a web asset this size is worth a second look
            print("    {}{}{}{}{}{}{}{}".format(
                C.CYAN, nm_, C.END,
                C.AMBER if big else "", fl, C.END if big else "",
                C.GRAY + rw, C.END))
        rest = kept[len(shown):]
        if rest:
            print("{}    {:<30}{:>10}{:>11}   {} more{}".format(
                C.GRAY, "(smaller ones)", human_bytes(sum(x[0] for x in rest)),
                human_bytes(sum(x[1] for x in rest)), len(rest), C.END))

        dropped = sorted(i for i in manifest if i not in present)
        if dropped:
            names = ", ".join(dropped)
            print("{}    {:<30}{:>10}{:>11}   {}{}".format(
                C.GRAY, "dropped (linker-removed)", "-", "-",
                names if len(names) <= 40 else "{} assets".format(len(dropped)), C.END))
        # Die Summe ist die Aussage des Blocks -- sie darf nicht zwischen den Zeilen untergehen.
        print("{}{:<30}{:>10}{:>11}{}{}   {} of {} assets{}".format(
            "    " + C.GRAY + G_RULE * 47 + C.END + "\n    " + C.BOLD, "total in firmware", human_bytes(in_bin),
            human_bytes(sum(r for _g, r, _i in kept)), C.END, C.GRAY, len(kept), len(manifest), C.END))

    _webasset_generate()
    env.AddPostAction("checkprogsize", quiet_action(_webasset_report))
