# C-Trie PDF Reader & Search Engine

A high-performance, cross-language desktop search engine. This project uses a **C-based Trie data structure** for lightning-fast indexing and **Poppler** for PDF text extraction, all wrapped in a Python interface for ease of use.

## 🚀 Features

- **Blazing Fast Indexing**: Custom C implementation of a Trie (prefix tree) for $O(L)$ word lookups, where $L$ is the length of the word.
- **Automated Discovery**: Recursive directory crawling to find and map PDF documents.
- **Poppler Integration**: Robust PDF parsing using the industry-standard Poppler GLib library.
- **Memory Efficient**: Manual memory management in C with a Python `ctypes` bridge to ensure a low footprint.
- **Case-Insensitive**: Normalized search queries via the Python manager.

## 🛠 Prerequisites

Since this project builds against native C libraries, you will need:

- Compiler: `gcc`
- Build System: `make`
- Libraries: `poppler-glib` and `glib-2.0`
- Language: `python3`

### Install Dependencies (Arch Linux)

```
sudo pacman -S poppler-glib glib2 gcc make
```

## 🏗 Installation & Build

1. **Clone the repository**:

```
git clone https://github.com/samnodier/documentctl.git
cd pdf-search-engine
```

1. **Compile the C Shared Library**: The project uses a `Makefile` to compile the C source files into a shared object (`.so`) that Python can load.

```
make
```

## 🖥 Usage

Run the engine by pointing it to a directory containing PDF files:

```
python main.py /path/to/your/pdfs
```

### Commands

- **Indexing**: Upon launch, the engine will crawl and index all PDFs found.
- **Search**: Type any word to see a list of documents where it occurs.
- **Exit**: Type `exit` to shut down the engine and free allocated memory.

## 📂 Project Structure

- `main.py``: The Python entry point and`ctypes` bridge.
- `engine.c/h`: The core search engine manager and document mapping.
- `trie.c/h`: High-performance Trie data structure implementation.
- `crawler.c/h`: Filesystem navigation logic.pdf_processor.
- `c/h`: PDF text extraction logic using Poppler.
- `Makefile`: Handles compilation and linking of the C shared library.

## 🗺 Roadmap

- [x] **Phase 1**: High-performance C-Trie indexer.
- [x] **Phase 2**: Automated filesystem crawling and Poppler integration.
- [x] **Phase 3**: Page-level granularity and hit-context snippets.
- [ ] **Phase 4**: AI-assisted summarization (Gemini API integration).
- [ ] **Phase 5**: Persistent binary index storage to disk.

## 🤝 Contributing

This is an MVP (Minimum Viable Product). Future iterations will include:

- Page number tracking for search results.
- Persistent disk-based indexing.
- Multi-threaded crawling
