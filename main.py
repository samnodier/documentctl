import ctypes
import os
import argparse


def handle_pdf(path_bytes):
    # C sends bytes, so we can decode back to a string
    path_str = path_bytes.decode("utf-8")
    print(f"[Python] Found PDF: {path_str}")


def main():
    lib_path = os.path.abspath("libengine.so")
    lib = ctypes.CDLL(lib_path)

    CALLBACK_TYPE = ctypes.CFUNCTYPE(None, ctypes.c_char_p)

    lib.crawl_directory.argtypes = [ctypes.c_char_p, ctypes.c_void_p, CALLBACK_TYPE]
    lib.engine_index_all.argtypes = [ctypes.c_void_p]
    lib.engine_create.restype = ctypes.c_void_p
    lib.engine_free.argtypes = [ctypes.c_void_p]
    lib.get_search_results.restype = ctypes.POINTER(ctypes.c_int)
    lib.get_search_results.argtypes = [
        ctypes.c_void_p,
        ctypes.c_char_p,
        ctypes.POINTER(ctypes.c_int),
    ]
    lib.free_results.argtypes = [ctypes.POINTER(ctypes.c_int)]
    lib.engine_get_document_path.restype = ctypes.c_char_p
    lib.engine_get_document_path.argtypes = [ctypes.c_void_p, ctypes.c_int]

    # Call the function
    parser = argparse.ArgumentParser(description="Prints PDFs in a directory")
    parser.add_argument("dir", type=str, help="Directory")

    args = parser.parse_args()

    c_callback = CALLBACK_TYPE(handle_pdf)
    my_engine = lib.engine_create()

    result = lib.crawl_directory(args.dir.encode("utf-8"), my_engine, c_callback)

    # ... crawling is done
    if result == 0:
        print("[Python] Starting Indexing...")
        lib.engine_index_all(my_engine)
        print("[Python] Indexing Complete!")
    else:
        print("[Python] Crawl failed.")

    # Loop to test the search results
    while True:
        word = input("\nSearch (or 'exit'): ")
        if word.lower() == "exit":
            break

        count = ctypes.c_int()
        # Get the pointer to the results array
        clean_word = (
            word.lower().strip()
        )  # clean the word before giving it to C, lowercase it
        results = lib.get_search_results(
            my_engine, clean_word.encode("utf-8"), ctypes.byref(count)
        )

        if count.value > 0:
            print(f"Found {count.value} occurrences:")
            for i in range(count.value):
                # Get the actual doc_id from the results array
                actual_doc_id = results[i]

                # Use that id to get the filename
                path_ptr = lib.engine_get_document_path(my_engine, actual_doc_id)

                print(f" - {path_ptr.decode('utf-8')}")
            lib.free_results(results)
        else:
            print("No results found.")

    # free the engine
    lib.engine_free(my_engine)


if __name__ == "__main__":
    main()
