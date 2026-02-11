#!/usr/bin/env python3

"""
End-to-end integration tests
"""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "scripts"))

from search_engine import SearchEngine


def test_snippet_integration():
    print("\n=== Test: End-to-End Snippet Verification ===")

    base_dir = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
    test_data_dir = os.path.join(base_dir, "tests", "test_data")

    engine = SearchEngine(data_dir=test_data_dir)

    # 1. FORCE INDEXING (Don't just load, actually test the indexer!)
    print(f"[Test] Indexing directory: {test_data_dir}")
    engine.index_directory(test_data_dir)

    # 2. VERIFY TRIE CONTENT
    test_word = "sam"  # Or "the", but "sam" is better if it's in your resume
    results = engine.search(test_word)

    if not results:
        print(f"✗ FAILED: Search for '{test_word}' returned no results after indexing.")
        return False

    # 3. VERIFY SNIPPET ACCURACY
    # Check if the byte_offset correctly lands on the word
    result = results[0]
    snippet = engine.get_snippet(result)

    if not snippet:
        print("✗ FAILED: Snippet returned empty.")
        return False

    print(f"✓ Found: '{snippet}'")

    if test_word.lower() in snippet.lower():
        print(
            f"✅ SUCCESS: Term '{test_word}' found in snippet at offset {result.byte_offset}"
        )
        return True
    else:
        print("✗ FAILED: Snippet mismatch. Check C byte_offset logic.")
        return False


def main():
    print("\n" + "=" * 50)
    print("Integration Test Suite")
    print("=" * 50)

    result = test_snippet_integration()

    return 0 if result else 1


if __name__ == "__main__":
    sys.exit(main())
