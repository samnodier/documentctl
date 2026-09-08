"""Launch the documentctl GUI, or the CLI indexer/search loop."""

import argparse
import os
import sys

ROOT = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(ROOT, "scripts"))


def main() -> int:
    parser = argparse.ArgumentParser(
        description="PDF search engine and reader"
    )
    parser.add_argument(
        "directory",
        nargs="?",
        help="Folder of PDFs to index (starts the command-line search loop)",
    )
    parser.add_argument(
        "--gui",
        action="store_true",
        help="Open the desktop reader (default when no directory is given)",
    )
    parser.add_argument(
        "--reindex",
        action="store_true",
        help="Force reindexing even if an index already exists (CLI mode)",
    )
    parser.add_argument(
        "--data-dir",
        type=str,
        default=None,
        help="Directory to store the index (default: ./data)",
    )
    args = parser.parse_args()

    if args.directory and not args.gui:
        from cli import main as cli_main

        argv = [sys.argv[0], args.directory]
        if args.reindex:
            argv.append("--reindex")
        if args.data_dir:
            argv.extend(["--data-dir", args.data_dir])
        sys.argv = argv
        result = cli_main()
        return result if result is not None else 0

    from PyQt6.QtWidgets import QApplication
    from gui import PDFReaderWindow

    app = QApplication(sys.argv)
    app.setApplicationName("documentctl")
    window = PDFReaderWindow()
    window.show()
    return app.exec()


if __name__ == "__main__":
    raise SystemExit(main())
