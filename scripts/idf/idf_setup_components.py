# -*- coding: utf-8 -*-
"""
Open ■
┬────┴  setup_idf_components
■ KNX   2026 OpenKNX - Erkan Çolak

Creates the IDF component scaffold required for OAM-Nuki:
  components/
    certs/                          -- cert files (managed by pre_generate_crt_asm.py)
    esp_https_server/               -- HTTPS server cert placeholder
    espressif__esp_diagnostics/     -- empty stub to disable managed component
    espressif__esp_insights/        -- empty stub to disable managed component
  idf_component.yml                 -- IDF component manifest in the project root

Usage:
  Standalone : python lib/OGM-Common/scripts/idf/idf_setup_components.py [project_root]
  PlatformIO : called by pre_generate_crt_asm.py via subprocess (not as extra_script)
"""

from __future__ import print_function
import os
import shutil

# --------------------------------------------------------------------------- #
# Resolve project root: works both standalone and as PlatformIO pre-script
# --------------------------------------------------------------------------- #
import sys
try:
    Import("env")  # noqa: F821
    PROJECT_DIR = env.subst("$PROJECT_DIR")
except Exception:
    if len(sys.argv) > 1 and os.path.isdir(sys.argv[1]):
        PROJECT_DIR = sys.argv[1]
    else:
        # Fallback: 4 levels up from this script's location
        # (scripts/pio -> scripts -> OGM-Common -> lib -> project_root)
        PROJECT_DIR = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))

COMPONENTS_DIR = os.path.join(PROJECT_DIR, "components")

# --------------------------------------------------------------------------- #
# Text files  {relative_path_under_components: content}
# --------------------------------------------------------------------------- #
FILES = {
    os.path.join("esp_https_server", ".gitkeep"): "",
    os.path.join("espressif__esp_diagnostics", "CMakeLists.txt"): "idf_component_register()\n",
    os.path.join("espressif__esp_diagnostics", "idf_component.yml"): 'version: "0.0.1"\n',
    os.path.join("espressif__esp_insights",    "CMakeLists.txt"): "idf_component_register()\n",
    os.path.join("espressif__esp_insights",    "idf_component.yml"): 'version: "1.2.2"\ndescription: "Stub - disabled"\n',
    os.path.join("certs", ".gitkeep"): "",
}

# --------------------------------------------------------------------------- #
# idf_component.yml in project root
# --------------------------------------------------------------------------- #
IDF_COMPONENT_YML = os.path.join(PROJECT_DIR, "idf_component.yml")
IDF_COMPONENT_YML_CONTENT = """\
dependencies:
  idf:
    version: ">=5.1"
  espressif__esp_insights:
    override_path: components/espressif__esp_insights
    rules:
      - if: "target in [esp32p4]"
  espressif__esp_diagnostics:
    override_path: components/espressif__esp_diagnostics
    rules:
      - if: "target in [esp32p4]"
"""

# --------------------------------------------------------------------------- #
# Helpers
# --------------------------------------------------------------------------- #
def _makedirs(path):
    if not os.path.isdir(path):
        os.makedirs(path)


def _create_text(path, content):
    _makedirs(os.path.dirname(path))
    rel = os.path.relpath(path, PROJECT_DIR)
    if os.path.exists(path) and os.path.basename(path) != ".gitkeep":
        print("[setup_idf] Already exists: " + rel)
        return
    with open(path, "w") as f:
        f.write(content)
    print("[setup_idf] Created: " + rel)


# --------------------------------------------------------------------------- #
# Main
# --------------------------------------------------------------------------- #
print("\n[setup_idf] Setting up IDF component scaffold in 'components/' ...")

for rel, content in FILES.items():
    _create_text(os.path.join(COMPONENTS_DIR, rel), content)

certs_dir = os.path.join(COMPONENTS_DIR, "certs")
_makedirs(certs_dir)
# Copy default cert files from scripts/idf/certs/ -- never overwrite developer's own certs
SCRIPT_CERTS_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "certs")
for fname in sorted(os.listdir(SCRIPT_CERTS_DIR)):
    src_cert = os.path.join(SCRIPT_CERTS_DIR, fname)
    dst_cert = os.path.join(certs_dir, fname)
    if os.path.exists(dst_cert):
        print("[setup_idf] Already exists (not overwriting): " + os.path.relpath(dst_cert, PROJECT_DIR))
    else:
        shutil.copy2(src_cert, dst_cert)
        print("[setup_idf] Copied: " + os.path.relpath(dst_cert, PROJECT_DIR))

if not os.path.exists(IDF_COMPONENT_YML):
    with open(IDF_COMPONENT_YML, "w") as f:
        f.write(IDF_COMPONENT_YML_CONTENT)
    print("[setup_idf] Created: idf_component.yml")
else:
    print("[setup_idf] Already exists: idf_component.yml")

print("[setup_idf] Done.\n")
