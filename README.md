# C-Trie PDF Reader & Search Engine

A desktop PDF search engine. Indexing and lookup run in C (a trie plus Poppler), and Python provides a command-line loop and a PyQt reader.

## Prerequisites

- `gcc` (or Apple clang), `make`, `pkg-config`
- `poppler-glib` and `glib`
- Python 3.10+ (3.14 on Fedora 44 is fine)

### Fedora

```bash
sudo dnf install gcc make pkgconf poppler-glib-devel glib2-devel python3
```

### macOS

```bash
brew install gcc make pkg-config poppler python
```

Homebrew’s `poppler` package includes the GLib bindings the C library links against.

Python packages (PyQt6, PyMuPDF) are installed with `uv` or `pip` below. You do not need to install Poppler via Python.

## Build

From the repo root:

```bash
make
```

That compiles `lib/libengine.so` on Linux or `lib/libengine.dylib` on macOS.

Install the Python dependencies:

```bash
uv sync
```

Or without uv:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install pymupdf pyqt6
```

## How to run

Always run from the repo root after `make`.

**Desktop reader (default):**

```bash
uv run python main.py
```

In the window:

1. **Index Folder** (toolbar, or `Ctrl+Shift+I`) — pick a directory of PDFs. Search needs this once.
2. **Open** (`Ctrl+O`) — view a PDF.
3. Type a word in the left search box and press Go. Click a hit to jump to that page.

**Command-line index + search:**

```bash
uv run python main.py /path/to/your/pdfs
```

Type a word to search, or `exit` to quit. Re-index with:

```bash
uv run python main.py /path/to/your/pdfs --reindex
```

If `uv` is not installed, use the venv instead:

```bash
source .venv/bin/activate
python main.py
```

If you copied this repo from another machine or distro and `uv` complains about the interpreter, recreate the environment:

```bash
uv venv --clear --python python3
uv sync
```

## Tests

```bash
make test
make test-python
```

`make test-python` needs the shared library (`make`) and the Python packages above.

## Project layout

- `main.py` — launches the GUI, or the CLI when you pass a directory
- `scripts/gui.py` — PyQt PDF reader and search panel
- `scripts/cli.py` — interactive search loop
- `scripts/search_engine.py` — `ctypes` bridge to the C library
- `src/` / `include/` — crawler, trie, Poppler indexer, query API
- `Makefile` — shared library, tests, and `compile_commands.json` for the editor

The index is stored only on your machine at `data/index.db` (created on first index). It is gitignored and is not part of the GitHub repo. If indexed PDFs are deleted, those entries are dropped the next time the app starts.
