import ctypes
import os
import argparse

def handle_pdf(path_bytes):
    # C sends bytes, so we can decode back to a string
    path_str = path_bytes.decode('utf-8')
    print(f"[Python] Found PDF: {path_str}")

def main():
    lib_path = os.path.abspath("libengine.so")
    lib = ctypes.CDLL(lib_path)

    CALLBACK_TYPE = ctypes.CFUNCTYPE(None, ctypes.c_char_p)

    lib.crawl_directory.argtypes = [ctypes.c_char_p, ctypes.c_void_p, CALLBACK_TYPE]
    lib.engine_index_all.argtypes = [ctypes.c_void_p]
    lib.engine_create.restype = ctypes.c_void_p
    lib.engine_free.argtypes = [ctypes.c_void_p]

    # Call the function
    parser = argparse.ArgumentParser(description="Prints PDFs in a directory")
    parser.add_argument("dir", type=str, help="Directory")

    args = parser.parse_args()

    c_callback = CALLBACK_TYPE(handle_pdf)
    my_engine = lib.engine_create()

    result = lib.crawl_directory(args.dir.encode('utf-8'), my_engine, c_callback)

    # ... crawling is done
    if result == 0:
        print("[Python] Starting Indexing...")
        lib.engine_index_all(my_engine)
        print("[Python] Indexing Complete!")
    else:
        print("[Python] Crawl failed.")

    # free the engine
    lib.engine_free(my_engine)

if __name__ == "__main__":
    main()

