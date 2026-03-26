from __future__ import annotations

import json
import tempfile
import unittest
from pathlib import Path

from tools.notices.notices_support import generate_notices_outputs


def _write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


class NoticesSupportTests(unittest.TestCase):
    def test_generate_windows_notices_merges_owners(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            repo_root = Path(temp_dir)
            self._write_minimal_repo(repo_root)
            output_root = repo_root / "dist" / "notices"

            outputs = generate_notices_outputs(repo_root, output_root, ["windows"])

            windows_payload = json.loads((outputs["windows"] / "notices.json").read_text(encoding="utf-8"))
            packages = {item["package_id"]: item for item in windows_payload["packages"]}

            self.assertEqual(["bills_cli", "bills_core"], packages["pkg_alpha"]["owners"])
            self.assertEqual(["alpha::target"], packages["pkg_alpha"]["resolved_artifacts"])
            self.assertEqual(["bills_io"], packages["pkg_beta"]["owners"])
            self.assertEqual(["beta::target"], packages["pkg_beta"]["resolved_artifacts"])

    def test_missing_license_text_fails(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            repo_root = Path(temp_dir)
            self._write_minimal_repo(repo_root)
            catalog_path = repo_root / "third_party" / "catalog" / "pkg_alpha.toml"
            catalog_path.write_text(
                "\n".join(
                    [
                        'package_id = "pkg_alpha"',
                        'display_name = "Alpha Package"',
                        'homepage = "https://example.com/alpha"',
                        'license_spdx = "MIT"',
                        'license_text_relpath = "third_party/texts/missing/LICENSE.txt"',
                        "",
                    ]
                ),
                encoding="utf-8",
            )

            with self.assertRaisesRegex(FileNotFoundError, "missing"):
                generate_notices_outputs(repo_root, repo_root / "dist" / "notices", ["windows"])

    def test_android_runtime_artifact_must_be_covered(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            repo_root = Path(temp_dir)
            self._write_minimal_repo(repo_root)
            output_root = repo_root / "dist" / "notices"
            release_path = repo_root / "release.json"
            debug_path = repo_root / "debug.json"
            release_path.write_text(
                json.dumps(
                    {
                        "configuration": "releaseRuntimeClasspath",
                        "artifacts": [
                            {
                                "group": "example.unmatched",
                                "name": "runtime",
                                "version": "1.0.0",
                            }
                        ],
                    }
                ),
                encoding="utf-8",
            )
            debug_path.write_text(
                json.dumps(
                    {
                        "configuration": "debugRuntimeClasspath",
                        "artifacts": [
                            {
                                "group": "example.android",
                                "name": "debug-only",
                                "version": "1.0.0",
                            }
                        ],
                    }
                ),
                encoding="utf-8",
            )

            with self.assertRaisesRegex(ValueError, "is not covered"):
                generate_notices_outputs(
                    repo_root,
                    output_root,
                    ["android"],
                    android_release_artifacts_path=release_path,
                    android_debug_artifacts_path=debug_path,
                )

    def test_repo_windows_notices_generate(self) -> None:
        repo_root = Path(__file__).resolve().parents[3]
        with tempfile.TemporaryDirectory() as temp_dir:
            outputs = generate_notices_outputs(
                repo_root,
                Path(temp_dir) / "notices",
                ["windows"],
            )

            payload = json.loads((outputs["windows"] / "notices.json").read_text(encoding="utf-8"))
            package_ids = {item["package_id"] for item in payload["packages"]}

            self.assertIn("nlohmann_json", package_ids)
            self.assertIn("tomlplusplus", package_ids)
            self.assertIn("sqlite_amalgamation", package_ids)

    def _write_minimal_repo(self, repo_root: Path) -> None:
        _write_text(
            repo_root / "third_party" / "texts" / "mit" / "LICENSE.txt",
            "MIT License\n",
        )
        _write_text(
            repo_root / "third_party" / "catalog" / "pkg_alpha.toml",
            "\n".join(
                [
                    'package_id = "pkg_alpha"',
                    'display_name = "Alpha Package"',
                    'homepage = "https://example.com/alpha"',
                    'license_spdx = "MIT"',
                    'license_text_relpath = "third_party/texts/mit/LICENSE.txt"',
                    "",
                ]
            ),
        )
        _write_text(
            repo_root / "third_party" / "catalog" / "pkg_beta.toml",
            "\n".join(
                [
                    'package_id = "pkg_beta"',
                    'display_name = "Beta Package"',
                    'homepage = "https://example.com/beta"',
                    'license_spdx = "MIT"',
                    'license_text_relpath = "third_party/texts/mit/LICENSE.txt"',
                    "",
                ]
            ),
        )
        _write_text(
            repo_root / "libs" / "bills_core" / "notices.toml",
            "\n".join(
                [
                    'schema_version = "1"',
                    'layer_id = "bills_core"',
                    "",
                    "[[deps]]",
                    'package_id = "pkg_alpha"',
                    'cmake_targets = ["alpha::target"]',
                    "",
                ]
            ),
        )
        _write_text(
            repo_root / "libs" / "io" / "notices.toml",
            "\n".join(
                [
                    'schema_version = "1"',
                    'layer_id = "bills_io"',
                    "",
                    "[[deps]]",
                    'package_id = "pkg_beta"',
                    'cmake_targets = ["beta::target"]',
                    "",
                ]
            ),
        )
        _write_text(
            repo_root / "apps" / "bills_cli" / "notices.toml",
            "\n".join(
                [
                    'schema_version = "1"',
                    'layer_id = "bills_cli"',
                    "",
                    "[[deps]]",
                    'package_id = "pkg_alpha"',
                    'cmake_targets = ["alpha::target"]',
                    "",
                ]
            ),
        )
        _write_text(
            repo_root / "apps" / "bills_android" / "notices.toml",
            "\n".join(
                [
                    'schema_version = "1"',
                    'layer_id = "bills_android"',
                    "",
                    "[[deps]]",
                    'package_id = "pkg_beta"',
                    'maven_groups = ["example.android"]',
                    "",
                ]
            ),
        )


if __name__ == "__main__":
    unittest.main()
