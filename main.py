import ctypes
import argparse

def handle_pdf(path_bytes):
    # C sends bytes, so we can decode back to a string
    path_str = path_bytes.decode('utf-8')
    print(f"[Python] Found PDF: {path_str}")

def main():
    lib_path = "./crawler.so" # We use .so extension for linux/mac and .dll for Win
    crawler = ctypes.CDLL(lib_path) # load the function

    CALLBACK_TYPE = ctypes.CFUNCTYPE(None, ctypes.c_char_p)

    crawler.crawl_directory.argtypes = [ctypes.c_char_p, CALLBACK_TYPE]
    crawler.crawl_directory.restype = None

    # Call the function
    parser = argparse.ArgumentParser(description="Prints PDFs in a directory")
    parser.add_argument("dir", type=str, help="Directory")

    args = parser.parse_args()

    c_callback = CALLBACK_TYPE(handle_pdf)

    result = crawler.crawl_directory(args.dir.encode('utf-8'), c_callback)

    # print(f"Result is: {result}")


if __name__ == "__main__":
    main()

