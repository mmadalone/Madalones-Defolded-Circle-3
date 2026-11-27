#!/usr/bin/env python3
# Copyright (c) 2026 madalone. Check a UCR3 install bundle before it is published or installed.
# SPDX-License-Identifier: GPL-3.0-or-later
"""Check a UCR3 install bundle: the layout the firmware expects, the right binary, every file of config/ (v1.6.6).

History
-------
Three times a release shipped a bundle that the pipeline had built without complaint:
  - up to v1.4.19 the tarball held the raw build folder, and the firmware refused it ("Missing file: remote-ui");
  - v1.4.41 to v1.5.3 held 2 of the files of deploy/config/ (12 at v1.4.41, 18 from v1.4.42), so a remote
    installed from GitHub had no sounds;
  - v1.5.2's binary carried an empty version string (CHANGELOG v1.5.3). Local builds have kept an older one
    (an incremental build).
Since v1.6.2 the maintainer ran a copy of this check by hand on the published assets. Now build.yml runs it on the
tarball, before it can be uploaded, and the same script checks a local test build.

What it checks
--------------
  - The bundle holds release.json, bin/remote-ui and config/, optionally version.txt, and nothing else.
  - release.json is valid JSON and byte-identical to deploy/release.json; its version equals VERSION in
    remote-ui.pro; requirements.firmware_version is there (the firmware reads it at install).
  - bin/remote-ui is an AArch64 ELF file. Its exec bit is not checked: test bundles packed on Windows carry
    mode 0644, and such bundles have been installed and run on a Remote 3.
  - The binary carries the version it should, matched as a whole NUL-terminated string. A substring test passes for
    every binary of this toolchain: libpng's "1.6.39" contains "1.6.3".
      * with --tag vX.Y.Z (a release): the binary carries X.Y.Z, release.json says X.Y.Z, version.txt (if there)
        says vX.Y.Z, and the binary carries no "X.Y.Z-..." string of a build between releases;
      * without --tag: the binary carries the string of version.txt (or --built-version) without its "v", which is
        how remote-ui.pro derives APP_VERSION from `git describe`. release.json is not compared with it: between
        releases it still names the last release.
  - The binary names the Qt version of the pinned toolchain image (QT_VERSION below; change it with the image).
  - config/ holds exactly the files of deploy/config/, byte for byte, and no sub-directory (the firmware refuses a
    bundle with one: "Could not extract archive").

Exit codes:
  0 - every check passed (with --self-test: every doctored bundle was reported, and the good ones passed).
  1 - a check failed (each one is listed), or the self-test failed.
  2 - a file or folder was not found, or the arguments are wrong.

Usage:
  python3 tools/check_bundle.py <bundle folder or .tar.gz> [--root DIR] [--tag vX.Y.Z] [--built-version STR]
  python3 tools/check_bundle.py --self-test
"""

import argparse
import io
import json
import os
import re
import sys
import tarfile
import tempfile
from pathlib import Path

QT_VERSION = "5.15.19"          # the Qt of the pinned toolchain image (BUILD.md, "Toolchain pinning rationale")
ALLOWED_TOP = {"release.json", "version.txt", "bin", "config"}
ELF_MAGIC = b"\x7fELF"
EM_AARCH64 = b"\xb7\x00"        # e_machine 183, little endian, at offset 18


class Report:
    def __init__(self, quiet: bool = False):
        self.failed = []
        self.quiet = quiet

    def check(self, ok: bool, text: str) -> bool:
        if not ok:
            self.failed.append(text)
        if not self.quiet:
            print(("ok   " if ok else "FAIL ") + text)
        return ok


def pro_version(root: Path):
    """VERSION from remote-ui.pro, or None."""
    m = re.search(r"^VERSION[ \t]*=[ \t]*(\S+)", (root / "remote-ui.pro").read_text(encoding="utf-8"), re.MULTILINE)
    return m.group(1) if m else None


def whole(data: bytes, version: str) -> int:
    """How often `version` stands in `data` as a whole NUL-terminated string."""
    return data.count(b"\x00" + version.encode() + b"\x00")


def unpack(tarball: Path, dest: Path, report: Report) -> None:
    """Extract `tarball` into `dest`. Refuses a member with a path outside dest."""
    with tarfile.open(tarball, "r:*") as tar:
        for member in tar.getmembers():
            name = member.name
            while name.startswith("./"):
                name = name[2:]
            if name in ("", "."):
                continue
            parts = Path(name).parts
            # anchor: "/" on POSIX; "C:\", "C:" and a leading "\" on Windows, where joinpath() would leave dest
            if (member.name.startswith("/") or Path(name).anchor or ".." in parts
                    or member.issym() or member.islnk()):
                report.check(False, f"tarball member {member.name!r}: an absolute path, '..' or a link")
                continue
            target = dest.joinpath(*parts)
            if member.isdir():
                target.mkdir(parents=True, exist_ok=True)
            elif member.isfile():
                target.parent.mkdir(parents=True, exist_ok=True)
                with tar.extractfile(member) as src, open(target, "wb") as out:
                    out.write(src.read())
            else:
                report.check(False, f"tarball member {member.name!r}: not a file or a folder")


def check_bundle(bundle: Path, root: Path, tag, built_version, report: Report) -> None:
    tmp = None
    if bundle.is_file():
        tmp = tempfile.TemporaryDirectory()
        folder = Path(tmp.name)
        unpack(bundle, folder, report)
    else:
        folder = bundle
    try:
        _check_folder(folder, root, tag, built_version, report)
    finally:
        if tmp:
            tmp.cleanup()


def _check_folder(folder: Path, root: Path, tag, built_version, report: Report) -> None:
    check = report.check
    top = sorted(p.name for p in folder.iterdir())
    extra = [n for n in top if n not in ALLOWED_TOP]
    check(not extra, "the bundle holds only release.json, version.txt, bin/ and config/"
          + (f" (also: {', '.join(extra)})" if extra else ""))

    # release.json
    repo_json = root / "deploy" / "release.json"
    rel_path = folder / "release.json"
    version_pro = pro_version(root)
    rel = None
    if check(rel_path.is_file(), "release.json is there"):
        rel_bytes = rel_path.read_bytes()
        try:
            rel = json.loads(rel_bytes.decode("utf-8"))
        except (UnicodeDecodeError, ValueError) as err:
            check(False, f"release.json is valid JSON ({err})")
        if isinstance(rel, dict):
            check(True, "release.json is valid JSON")
            check(rel.get("version") == version_pro,
                  f"release.json version {rel.get('version')!r} equals remote-ui.pro VERSION {version_pro!r}")
            req = rel.get("requirements")
            fw = req.get("firmware_version") if isinstance(req, dict) else None
            check(isinstance(fw, str) and fw.strip() != "", f"requirements.firmware_version is set ({fw!r})")
            check(rel_bytes == repo_json.read_bytes(), "release.json is byte-identical to deploy/release.json")
        elif rel is not None:
            check(False, "release.json holds a JSON object")

    # which version the binary has to carry
    vt_path = folder / "version.txt"
    version_txt = vt_path.read_text(encoding="utf-8").strip() if vt_path.is_file() else None
    if tag:
        tag_version = tag[1:] if tag.startswith("v") else tag
        expected = tag_version
        if version_txt is not None:
            check(version_txt == "v" + tag_version, f"version.txt {version_txt!r} equals the tag v{tag_version}")
        if isinstance(rel, dict):
            check(rel.get("version") == tag_version,
                  f"release.json version {rel.get('version')!r} equals the tag's {tag_version!r}")
    else:
        source = built_version or version_txt
        # remote-ui.pro: REMOTE_VERSION = $$replace(GIT_VERSION, v, "")  (every "v" goes, not only the first)
        expected = source.replace("v", "") if source else None
        check(expected is not None and expected != "",
              "the built version is known (version.txt in the bundle, or --built-version)")

    # the binary
    binary = folder / "bin" / "remote-ui"
    bin_dir = folder / "bin"
    if bin_dir.is_dir():
        others = sorted(p.name for p in bin_dir.iterdir() if p.name != "remote-ui")
        check(not others, "bin/ holds only remote-ui" + (f" (also: {', '.join(others)})" if others else ""))
    if check(binary.is_file(), f"bin/remote-ui is there ({binary.stat().st_size if binary.is_file() else 0} bytes)"):
        data = binary.read_bytes()
        check(data[:4] == ELF_MAGIC, "bin/remote-ui is an ELF file")
        check(data[18:20] == EM_AARCH64, "its ELF machine is AArch64")
        if expected:
            count = whole(data, expected)
            check(count >= 1, f"the binary carries the version {expected!r} as a whole string ({count}x)")
            if tag:
                between = (expected + "-").encode()
                check(between not in data, f"the binary carries no string of a build between releases ({expected}-...)")
        check(QT_VERSION.encode() in data, f"the binary names Qt {QT_VERSION} ({data.count(QT_VERSION.encode())}x)")

    # config/
    repo_cfg = root / "deploy" / "config"
    cfg = folder / "config"
    if check(cfg.is_dir(), "config/ is there"):
        want = sorted(p.name for p in repo_cfg.iterdir() if p.is_file())
        have = sorted(p.name for p in cfg.iterdir() if p.is_file())
        check(have == want, f"config/ holds the {len(want)} files of deploy/config/ ({len(have)} there)")
        if not report.quiet:
            for name in sorted(set(want) - set(have)):
                print("     missing:", name)
            for name in sorted(set(have) - set(want)):
                print("     extra:  ", name)
        check(not any(p.is_dir() for p in cfg.iterdir()), "config/ has no sub-directory")
        same = [n for n in want if n in have and (cfg / n).read_bytes() == (repo_cfg / n).read_bytes()]
        check(len(same) == len(want), f"the files of config/ are byte-identical to the repo's ({len(same)}/{len(want)})")
        if not report.quiet:
            print("     sound files (.wav):", sum(1 for n in have if n.endswith(".wav")))


# ---------------------------------------------------------------------------------------------------------------
# self-test: a throwaway repo root and bundles built from it, each broken in one way
# ---------------------------------------------------------------------------------------------------------------

def _fake_binary(version: str, machine: bytes = EM_AARCH64, magic: bytes = ELF_MAGIC, extra: bytes = b"") -> bytes:
    header = magic + b"\x02\x01\x01" + b"\x00" * 9 + b"\x03\x00" + machine      # e_machine at offset 18
    return (header + b"\x00" * 44 + b"libpng 1.6.39\x00Qt " + QT_VERSION.encode() + b"\x00"
            + b"\x00" + version.encode() + b"\x00" + extra)


def _make_root(root: Path, version: str) -> None:
    (root / "deploy" / "config").mkdir(parents=True)
    (root / "remote-ui.pro").write_text(f"TEMPLATE = app\nVERSION = {version}\n", encoding="utf-8")
    (root / "deploy" / "release.json").write_text(
        json.dumps({"version": version, "requirements": {"firmware_version": ">=2.12.0"}}, indent=2) + "\n",
        encoding="utf-8")
    for name, body in (("font.otf", b"font"), ("click.wav", b"wav1"), ("zap.wav", b"wav2"), ("LICENSE.txt", b"ofl")):
        (root / "deploy" / "config" / name).write_bytes(body)


def _make_bundle(dest: Path, root: Path, binary_version: str, version_txt) -> None:
    (dest / "bin").mkdir(parents=True)
    (dest / "config").mkdir()
    (dest / "release.json").write_bytes((root / "deploy" / "release.json").read_bytes())
    for p in (root / "deploy" / "config").iterdir():
        (dest / "config" / p.name).write_bytes(p.read_bytes())
    (dest / "bin" / "remote-ui").write_bytes(_fake_binary(binary_version))
    os.chmod(dest / "bin" / "remote-ui", 0o755)
    if version_txt is not None:
        (dest / "version.txt").write_text(version_txt + "\n", encoding="utf-8")


def self_test() -> int:
    failures = []
    cases = [0]

    def run(name, expect_ok, prepare, tag="v1.6.3", binary_version="1.6.3", version_txt="v1.6.3",
            built_version=None, as_tarball=False, evil_member=None):
        cases[0] += 1
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp) / "root"
            bundle = Path(tmp) / "bundle"
            _make_root(root, "1.6.3")
            _make_bundle(bundle, root, binary_version, version_txt)
            if prepare:
                prepare(bundle, root)
            target = bundle
            if as_tarball:
                target = Path(tmp) / "bundle.tar.gz"
                with tarfile.open(target, "w:gz") as tar:
                    for p in sorted(bundle.rglob("*")):
                        info = tar.gettarinfo(str(p), arcname="./" + p.relative_to(bundle).as_posix())
                        if p.is_file():
                            with open(p, "rb") as fh:
                                tar.addfile(info, fh)
                        else:
                            tar.addfile(info)
                    if evil_member:
                        outside = tarfile.TarInfo(evil_member(tmp))
                        outside.size = 1
                        tar.addfile(outside, io.BytesIO(b"x"))
            report = Report(quiet=True)
            check_bundle(target, root, tag, built_version, report)
            ok = not report.failed
            if (Path(tmp) / "outside.txt").exists():
                failures.append(f"{name}: the member was written outside the bundle folder")
            if ok != expect_ok:
                failures.append(f"{name}: expected {'a pass' if expect_ok else 'a failure'}, got "
                                f"{'a pass' if ok else 'failures: ' + '; '.join(report.failed)}")

    def write(rel, body):
        return lambda b, r: (b / rel).write_bytes(body)

    # good bundles
    run("a release bundle", True, None)
    run("a release bundle as a tarball", True, None, as_tarball=True)
    run("a build between releases", True, None, tag=None, binary_version="1.6.3-4-g0a1b2c3",
        version_txt="v1.6.3-4-g0a1b2c3")
    run("a local test build, version given", True, None, tag=None, binary_version="1.6.3-autorotate1",
        version_txt=None, built_version="v1.6.3-autorotate1")
    # release.json
    run("release.json with a trailing comma", False, write("release.json", b'{"version": "1.6.3",}\n'))
    run("release.json of another version", False,
        write("release.json", b'{"version": "1.6.2", "requirements": {"firmware_version": ">=2.12.0"}}\n'))
    run("release.json without the firmware requirement", False, write("release.json", b'{"version": "1.6.3"}\n'))
    run("release.json that is not the repo's", False, lambda b, r: (b / "release.json").write_bytes(
        (b / "release.json").read_bytes().replace(b">=2.12.0", b">=2.13.0")))
    # the binary
    run("no binary", False, lambda b, r: (b / "bin" / "remote-ui").unlink())
    run("a binary that is not ELF", False, write("bin/remote-ui", b"#!/bin/sh\n" + b"\x00" * 80))
    run("an x86-64 binary", False, write("bin/remote-ui", _fake_binary("1.6.3", machine=b"\x3e\x00")))
    run("the version only inside a longer string (libpng's 1.6.39)", False, None, binary_version="9.9.9")
    run("the previous release's binary", False, None, binary_version="1.6.2")
    run("a release whose binary is a build between releases", False,
        write("bin/remote-ui", _fake_binary("1.6.3", extra=b"1.6.3-2-gabcdef0\x00")))
    # a tarball member that would land outside the bundle folder: next to it, in the test's own temporary folder
    run("a tarball member with '..'", False, None, as_tarball=True, evil_member=lambda tmp: "../outside.txt")
    run("a tarball member with an absolute path", False, None, as_tarball=True,
        evil_member=lambda tmp: tmp + "/outside.txt")
    if os.name == "nt":
        # rooted, without the drive letter: neither absolute nor with a drive for pathlib, and still outside
        run("a tarball member with a rooted Windows path", False, None, as_tarball=True,
            evil_member=lambda tmp: Path(tmp).as_posix()[2:].replace("/", "\\") + "\\outside.txt")
    run("a second file in bin/", False, write("bin/core", b"x"))
    # the tag
    run("version.txt of another build than the tag", False, None, version_txt="v1.6.3-1-gabcdef0")
    run("a tag that is not the version", False, None, tag="v1.6.4")
    run("a build between releases without any version", False, None, tag=None, version_txt=None)
    run("a build between releases with another binary", False, None, tag=None, binary_version="1.6.3",
        version_txt="v1.6.3-4-g0a1b2c3")
    # config/
    run("a file of config/ missing", False, lambda b, r: (b / "config" / "zap.wav").unlink())
    run("an extra file in config/", False, write("config/extra.wav", b"x"))
    run("a changed file in config/", False, write("config/click.wav", b"other"))
    run("a sub-directory in config/", False, lambda b, r: (b / "config" / "sub").mkdir())
    run("no config/", False, lambda b, r: [p.unlink() for p in (b / "config").iterdir()] and (b / "config").rmdir())
    # the layout
    run("an extra file at the top", False, write("README.txt", b"x"))

    for line in failures:
        print("SELF-TEST FAIL " + line)
    if not failures:
        print(f"self-test ok: {cases[0]} bundles")
    return 1 if failures else 0


def main() -> int:
    parser = argparse.ArgumentParser(description="Check a UCR3 install bundle (a folder or a .tar.gz).")
    parser.add_argument("bundle", nargs="?", help="the staged bundle folder, or the tarball")
    parser.add_argument("--root", default=None, help="the repository root (default: the parent of tools/)")
    parser.add_argument("--tag", default=None, help="the release tag this bundle is built for (vX.Y.Z)")
    parser.add_argument("--built-version", default=None,
                        help="the version string the build was given, for a bundle without version.txt")
    parser.add_argument("--self-test", action="store_true", help="run the built-in cases and exit")
    args = parser.parse_args()

    if args.self_test:
        return self_test()
    if not args.bundle:
        parser.print_usage(sys.stderr)
        return 2
    bundle = Path(args.bundle)
    root = Path(args.root) if args.root else Path(__file__).resolve().parent.parent
    for needed in (bundle, root / "remote-ui.pro", root / "deploy" / "release.json", root / "deploy" / "config"):
        if not needed.exists():
            print(f"check_bundle: {needed} not found", file=sys.stderr)
            return 2
    report = Report()
    check_bundle(bundle, root, args.tag, args.built_version, report)
    if report.failed:
        print(f"\n{len(report.failed)} check(s) FAILED")
        return 1
    print("\nALL CHECKS PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main())
