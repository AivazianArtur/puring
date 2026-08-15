#!/bin/bash

set -e

mkdir -p docs_site/api
mkdir -p docs_site/docs

ln -sf ../README.md               docs_site/index.md
ln -sf ../USER_GUIDE.md           docs_site/USER_GUIDE.md
ln -sf ../USER_GUIDE.md           docs_site/user-guide.md
ln -sf ../../docs/ARCHITECTURE.md docs_site/docs/ARCHITECTURE.md
ln -sf ../docs/ARCHITECTURE.md    docs_site/architecture.md
ln -sf ../../docs/DEVELOPMENT.md  docs_site/docs/DEVELOPMENT.md
ln -sf ../docs/DEVELOPMENT.md     docs_site/development.md
ln -sf ../../docs/IO_URING.md     docs_site/docs/IO_URING.md
ln -sf ../docs/IO_URING.md        docs_site/io-uring.md
ln -sf ../docs/IO_URING.md        docs_site/IO_URING.md

# --- API Reference (docs/api/*.md -> docs_site/api/*.md, and root-level for
#     relative links inside API.md which resolve relative to api.md's own
#     location in docs_site/, i.e. the root) -------------------------------
ln -sf ../docs/API.md docs_site/api.md 2>/dev/null || true
ln -sf ../../docs/API.md docs_site/docs/API.md 2>/dev/null || true
for name in loop file socket context-managers enums module-functions; do
  ln -sf ../../docs/api/${name}.md docs_site/api/${name}.md 2>/dev/null || true
  ln -sf ../docs/api/${name}.md    docs_site/${name}.md      2>/dev/null || true
done

ln -sf ../../docs/ROADMAP.md             docs_site/docs/ROADMAP.md             2>/dev/null || true
ln -sf ../../docs/BENCHMARKS_ANALYSIS.md docs_site/docs/BENCHMARKS_ANALYSIS.md 2>/dev/null || true
ln -sf ../docs/BENCHMARKS_ANALYSIS.md    docs_site/benchmarks-analysis.md      2>/dev/null || true
ln -sfn ../../docs/assets                docs_site/docs/assets                 2>/dev/null || true
ln -sfn ../docs/assets                   docs_site/assets                      2>/dev/null || true
ln -sfn ../../docs/examples              docs_site/docs/examples               2>/dev/null || true

ln -sf ../CONTRIBUTING.md      docs_site/CONTRIBUTING.md      2>/dev/null || true
ln -sf ../CODE_OF_CONDUCT.md   docs_site/CODE_OF_CONDUCT.md   2>/dev/null || true
ln -sf ../SECURITY.md          docs_site/SECURITY.md          2>/dev/null || true

echo "Symlinks are created at docs_site/"
