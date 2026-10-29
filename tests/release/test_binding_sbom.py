from __future__ import annotations

import hashlib
import json
import pathlib
import tempfile
import unittest
import xml.etree.ElementTree as element_tree
import zipfile

from binding_sbom import PACKAGE_SBOM
from kotlin_release import GROUP_PATH, publication_sboms
from sbom import component, document


class MavenSbomTest(unittest.TestCase):
    def test_classified_inventories_and_snapshot_metadata_follow_the_publication(self):
        with tempfile.TemporaryDirectory() as temporary:
            repository = pathlib.Path(temporary)
            directory = repository / GROUP_PATH / "fixture" / "0.202610.3-SNAPSHOT"
            directory.mkdir(parents=True)
            stem = "fixture-0.202610.3-20261029.010101-1"
            (directory / f"{stem}.pom").write_text("""
<project xmlns="http://maven.apache.org/POM/4.0.0">
  <groupId>org.maplibre.nativeffi</groupId><artifactId>fixture</artifactId>
  <version>0.202610.3-SNAPSHOT</version>
  <dependencies><dependency><groupId>org.jetbrains.kotlin</groupId>
    <artifactId>kotlin-stdlib</artifactId><version>2.4.0</version>
  </dependency></dependencies>
</project>""")
            metadata = directory / "maven-metadata.xml"
            metadata.write_text("""<metadata><versioning><snapshotVersions><snapshotVersion>
<extension>pom</extension><value>0.202610.3-20261029.010101-1</value>
<updated>20261029010101</updated></snapshotVersion></snapshotVersions></versioning></metadata>""")
            for platform in ("linux-gnu-x64", "macos-arm64"):
                root = component(
                    "maplibre-native-c", "0.202610.3-SNAPSHOT", "BSD-2-Clause"
                )
                root["bom-ref"] = f"native:{platform}:vulkan"
                vendor = component("vendor", "1.0.0", "MIT")
                inventory = document(
                    root, [vendor], {root["bom-ref"]: {vendor["bom-ref"]}}
                )
                with zipfile.ZipFile(
                    directory / f"{stem}-natives-{platform}.jar", "w"
                ) as archive:
                    archive.writestr(
                        f"META-INF/sbom/{PACKAGE_SBOM}", json.dumps(inventory)
                    )
            publication_sboms(repository)
            sidecar = directory / f"{stem}-cyclonedx.json"
            result = json.loads(sidecar.read_text())
            refs = {item["bom-ref"] for item in result["components"]}
            self.assertEqual(len(refs), 5)
            self.assertTrue(any("linux-gnu-x64" in ref for ref in refs))
            self.assertTrue(any("macos-arm64" in ref for ref in refs))
            for entry in result["dependencies"]:
                self.assertTrue(set(entry["dependsOn"]) <= refs)
            self.assertEqual(
                sidecar.with_name(sidecar.name + ".sha256").read_text().strip(),
                hashlib.sha256(sidecar.read_bytes()).hexdigest(),
            )
            publication_sboms(repository)
            entries = element_tree.parse(metadata).findall(
                "versioning/snapshotVersions/snapshotVersion"
            )
            classified = [
                entry
                for entry in entries
                if entry.findtext("classifier") == "cyclonedx"
            ]
            self.assertEqual(len(classified), 1)
            self.assertEqual(classified[0].findtext("extension"), "json")
            self.assertEqual(
                classified[0].findtext("value"), "0.202610.3-20261029.010101-1"
            )
