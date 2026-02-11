"""
Command-line interface for the search engine
"""

import os
import re
import argparse
from search_engine import SearchEngine


def highlight_text(text: str, query: str) -> str:
    """Highlight query term in text using ANSI colors"""

    # Use regex for case-insensitive highlighting
    pattern = re.compile(re.escape(query), re.IGNORECASE)
    return pattern.sub(lambda m: f"\033[38;5;33m{m.group(0)}\033[0m", text)


def interactive_search(engine: SearchEngine):
    """Interactive search loop"""
    print("\n" + "=" * 60)
    print("Interactive Search Mode")
    print("Type 'exit' or 'quit' to stop")
    print("=" * 60)

    # Loop to test the search results
    while True:
        try:
            query = input("\nSearch: ").strip()
            if query.lower() in ("exit", "quit", "q"):
                break

            if not query:
                continue

            # Search
            results = engine.search(query)

            if not results:
                print("No results found.")
                continue

            print(
                f"Found {len(results)} occurrence{'s' if len(results) != 1 else ''}:\n"
            )

            # Display results
            for i, result in enumerate(results, 1):
                filename = os.path.basename(result.doc_path)
                print(f"{i}. {filename} - Page {result.page_num + 1}")

                # Get and display snippet
                snippet = engine.get_snippet(result)
                if snippet:
                    highlighted = highlight_text(snippet, query)
                    print(f"    ...{highlighted}...")

                print()

        except KeyboardInterrupt:
            print("\n\nExiting...")
            break
        except Exception as e:
            print(f"Error: {e}")


def main():

    # Call the function
    parser = argparse.ArgumentParser(
        description="PDF Search Engine with AI capabilities"
    )
    parser.add_argument(
        "directory", type=str, nargs="?", help="Directory containing PDFs to index"
    )
    parser.add_argument(
        "--reindex", action="store_true", help="Force reindexing even if index exists"
    )
    parser.add_argument(
        "--data-dir",
        type=str,
        default="data",
        help="Directory to store index (default: data)",
    )

    args = parser.parse_args()

    # Create engine
    try:
        engine = SearchEngine(data_dir=args.data_dir)
    except FileNotFoundError as e:
        print(f"Error: {e}")
        return 1

    # Load existing index or create a new one
    needs_indexing = args.reindex or not engine.load()

    if needs_indexing:
        if not args.directory:
            print("Error: Directory required for indexing")
            print("Usage: python cli.py <directory>")
            return 1
        if not os.path.isdir(args.directory):
            print(f"Error: '{args.directory} is not a valid directory")
            return 1

        if args.reindex:
            engine.create_new()

        # Index directory
        if not engine.index_directory(args.directory):
            print("Indexing failed!")
            return 1

        # Save the index
        engine.save()

    # Start interactive search
    if engine.is_indexed():
        interactive_search(engine)
    else:
        print("No index available. Please specify a directory to index.")
        return 1

    return 0


if __name__ == "__main__":
    main()
