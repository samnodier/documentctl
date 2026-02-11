"""
Search Engine wrapper for libengine.so
Handles all C library interactions and state management
"""

import os
import ctypes
from typing import List, Optional


class RawOccurence(ctypes.Structure):
    _fields_ = [
        ("doc_id", ctypes.c_int),
        ("page_num", ctypes.c_int),
        ("byte_offset", ctypes.c_long),
    ]


class SearchResult:
    """Represents a single search result occurrence"""

    def __init__(
        self, doc_id: int, page_num: int, byte_offset: int, doc_path: str = ""
    ):
        self.doc_id = doc_id
        self.page_num = page_num
        self.byte_offset = byte_offset
        self.doc_path = doc_path

    def __repr__(self):
        return f"SearchResult(doc={self.doc_id}, page={self.page_num}, offset={self.byte_offset})"


class SearchEngine:
    """
    High-level interface to the C search engine library.
    Manages index creation, persistence, and querying.
    """

    def __init__(self, data_dir: str = "data", lib_path: str = "lib/libengine.so"):
        """
        Initialize the search engine.
        Args:
            data_dir: Directory to store index files
            lib_path: Path to the compiled C library
        """

        # Engine state
        self.engine = None
        self._is_indexed = False

        # Setup paths
        self.data_dir = data_dir
        self.index_path = os.path.join(data_dir, "index.db")
        self.lib_path = os.path.abspath(lib_path)

        # Ensure data directory exists
        os.makedirs(self.data_dir, exist_ok=True)

        # Load C Library
        if not os.path.exists(self.lib_path):
            raise FileNotFoundError(
                f"Library not found at {self.lib_path}. Run 'make' first."
            )

        self.lib = ctypes.CDLL(self.lib_path)
        self._setup_ctypes()

    def _setup_ctypes(self):
        # Engine lifecycle
        self.lib.engine_create.argtypes = []
        self.lib.engine_create.restype = ctypes.c_void_p

        self.lib.engine_free.argtypes = [ctypes.c_void_p]
        self.lib.engine_free.restype = None

        # Serialization
        self.lib.engine_serialize.argtypes = [ctypes.c_void_p, ctypes.c_char_p]
        self.lib.engine_serialize.restype = ctypes.c_int

        self.lib.engine_deserialize.argtypes = [ctypes.c_char_p]
        self.lib.engine_deserialize.restype = ctypes.c_void_p

        # Indexing
        CALLBACK_TYPE = ctypes.CFUNCTYPE(None, ctypes.c_char_p)
        self.lib.crawl_directory.argtypes = [
            ctypes.c_char_p,
            ctypes.c_void_p,
            CALLBACK_TYPE,
        ]
        self.lib.crawl_directory.restype = ctypes.c_int
        self.lib.engine_index_all.argtypes = [ctypes.c_void_p]
        self.lib.engine_index_all.restype = None

        # Search
        self.lib.get_search_results.argtypes = [
            ctypes.c_void_p,
            ctypes.c_char_p,
            ctypes.POINTER(ctypes.c_int),
        ]
        self.lib.get_search_results.restype = ctypes.POINTER(RawOccurence)

        self.lib.free_results.argtypes = [ctypes.POINTER(RawOccurence)]
        self.lib.free_results.restype = None

        # Document info
        self.lib.engine_get_document_path.argtypes = [ctypes.c_void_p, ctypes.c_int]
        self.lib.engine_get_document_path.restype = ctypes.c_char_p

        # Snippets
        self.lib.get_snippet.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_long]
        self.lib.get_snippet.restype = ctypes.POINTER(ctypes.c_char)

        self.lib.free_snippet.argtypes = [ctypes.POINTER(ctypes.c_char)]
        self.lib.free_snippet.restype = None

        # Store callback type for later use
        self.CALLBACK_TYPE = ctypes.CFUNCTYPE(None, ctypes.c_char_p)

    def create_new(self) -> bool:
        """Create a new empty search engine"""
        if self.engine:
            self.lib.engine_free(self.engine)

        self.engine = self.lib.engine_create()
        self._is_indexed = False
        return self.engine is not None

    def load(self) -> bool:
        """
        Load index from disk

        Returns:
            True if loaded successfully, False otherwise
        """
        if not os.path.exists(self.index_path):
            return False

        print(f"[Engine] Loading index from {self.index_path}...")
        self.engine = self.lib.engine_deserialize(self.index_path.encode("utf-8"))

        if self.engine:
            print("[Engine] Index loaded successfully")
            self._is_indexed = True
            return True
        else:
            print("[Engine] Failed to load index")
            return False

    def save(self) -> bool:
        """
        Save index to disk.

        Returns:
            True if saved successfully, False otherwise
        """
        if not self.engine:
            print("[Engine] No engine to save")
            return False

        print(f"[Engine] Saving index to {self.index_path}...")
        result = self.lib.engine_serialize(self.engine, self.index_path.encode("utf-8"))

        if result == 0:
            print("[Engine] Index saved successfully")
            return True
        else:
            print("[Engine] Failed ot save index")
            return False

    def index_directory(self, directory: str, callback=None) -> bool:
        """
        Index all PDFs in a directory

        Args:
            directory: Path to directory containing PDFs
            callback: Optional Python function to call for each PDF found

        Returns:
            True if indexing succeeded, False otherwise
        """
        if not self.engine:
            self.create_new()

        # Setup callback
        def default_callback(path_bytes):
            path_str = path_bytes.decode("utf-8")
            print(f"[Engine] Found PDF: {path_str}")

        c_callback = self.CALLBACK_TYPE(callback or default_callback)

        # Crawl and collect PDFs
        print(f"[Engine] Crawling directory: {directory}")
        result = self.lib.crawl_directory(
            directory.encode("utf-8"), self.engine, c_callback
        )

        if result != 0:
            print("[Engine] Crawl failed")
            return False

        # Index all found PDFs
        print("[Engine] Starting indexing...")
        self.lib.engine_index_all(self.engine)
        print("[Engine] Indexing complete!")

        self._is_indexed = True
        return True

    def search(self, query: str) -> List[SearchResult]:
        """
        Search for a word in the index.

        Args:
            query: Word to search for
        Returns:
            List of SearchResult objects
        """
        if not self.engine or not self._is_indexed:
            return []

        # Clean and prepare query
        clean_query = query.lower().strip()
        count = ctypes.c_int()

        # Get results from C
        results_ptr = self.lib.get_search_results(
            self.engine, clean_query.encode("utf-8"), ctypes.byref(count)
        )

        # Parse results
        results = []
        if count.value > 0:
            occurrences = ctypes.cast(results_ptr, ctypes.POINTER(RawOccurence))
            for i in range(count.value):
                occ = occurrences[i]
                doc_id = occ.doc_id
                page_num = occ.page_num
                byte_offset = occ.byte_offset

                # Get document path
                doc_path = self.lib.engine_get_document_path(
                    self.engine, doc_id
                ).decode("utf-8")
                result = SearchResult(doc_id, page_num, byte_offset, doc_path)
                results.append(result)

            # Free C memory
            self.lib.free_results(results_ptr)

        return results

    def get_snippet(self, result: SearchResult) -> Optional[str]:
        """
        Get text snippet around a search result.

        Args:
            result: SearchResult object

        Returns:
            Text snippet or None if failed
        """
        raw_snippet_ptr = self.lib.get_snippet(
            result.doc_path.encode("utf-8"), result.page_num, result.byte_offset
        )

        if not raw_snippet_ptr:
            return None

        try:
            raw_bytes = ctypes.string_at(raw_snippet_ptr)
            snippet = raw_bytes.decode("utf-8", errors="ignore")
            return snippet.replace("\n", " ")
        finally:
            # Always free C memory
            self.lib.free_snippet(raw_snippet_ptr)

    def is_indexed(self) -> bool:
        """Check if engine has been indexed"""
        return self._is_indexed

    def __del__(self):
        """Cleanup when object is destroyed"""
        if hasattr(self, "lib") and self.engine:
            self.lib.engine_free(self.engine)

    def __enter__(self):
        """Context manager support"""
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager cleanup"""
        if self.engine:
            self.lib.engine_free(self.engine)
        return False
