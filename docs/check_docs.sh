#!/bin/bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
DOCS_DIR="${REPO_ROOT}/docs"
BUILD_DIR="${DOCS_DIR}/_build"

if [ ! -f "${DOCS_DIR}/conf.py" ]; then
    echo "Can't find ${DOCS_DIR}/conf.py"
    exit 1
fi

echo "Checking Sphinx documentation for issues in ${DOCS_DIR}"

cd "${REPO_ROOT}"

sphinx-build -W --keep-going "${DOCS_DIR}" "${BUILD_DIR}"

echo
echo "Sphinx build completed without warnings."
