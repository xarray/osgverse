"""
Runs a COOE/COEP local webserver for testing emscripten deployment.
Note:
    Browsers that have implemented and enabled SharedArrayBuffer are gating it behind Cross Origin Opener Policy (COOP)
    and Cross Origin Embedder Policy (COEP) headers.
    Pthreads code will not work in deployed environment unless these headers are correctly set.
    see: https://emscripten.org/docs/porting/pthreads.html
"""

from http.server import HTTPServer, SimpleHTTPRequestHandler
import ssl, os, sys, mimetypes

class RequestHandler(SimpleHTTPRequestHandler):
    # Pre-compressed variants of each file, in the order of preference
    COMPRESSED_VARIANTS = (('.br', 'br'), ('.gz', 'gzip'))

    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        super().end_headers()

    def do_GET(self):
        path = self.path.split('?')[0]
        path = path.split('#')[0]
        file_path = self.translate_path(path)
        encoding = self.get_encoding_for_suffix(file_path)
        if encoding and os.path.exists(file_path):
            # The client is explicitly requesting a pre-compressed file (xxx.wasm.gz / .br)
            self.serve_compressed_file(file_path, encoding)
        else:
            variant = None if encoding else self.find_compressed_variant(file_path)
            if variant:
                self.serve_compressed_file(variant[0], variant[1])
            elif os.path.exists(file_path):
                super().do_GET()
            else:
                self.send_error(404, f"File not found: {path}")

    def get_encoding_for_suffix(self, file_path):
        for suffix, encoding in self.COMPRESSED_VARIANTS:
            if file_path.endswith(suffix):
                return encoding
        return None

    def accepts_encoding(self, encoding):
        return encoding in self.headers.get('Accept-Encoding', '').lower()

    def find_compressed_variant(self, file_path):
        """Find a pre-compressed variant accepted by the client. It should be newer than the
           original file, so that stale compressed copies are never sent to the browser"""
        for suffix, encoding in self.COMPRESSED_VARIANTS:
            if not self.accepts_encoding(encoding):
                continue
            variant_path = file_path + suffix
            if not os.path.exists(variant_path):
                continue
            if os.path.exists(file_path) and \
                os.path.getmtime(variant_path) < os.path.getmtime(file_path):
                continue
            return variant_path, encoding
        return None

    def serve_compressed_file(self, compressed_path, encoding):
        try:
            with open(compressed_path, 'rb') as f:
                content = f.read()
            original_filename = os.path.basename(os.path.splitext(compressed_path)[0])
            mime_type = self.get_mime_type(original_filename)

            self.send_response(200)
            self.send_header("Content-Type", mime_type)
            self.send_header("Content-Length", str(len(content)))
            self.send_header("Content-Encoding", encoding)
            self.send_header("Vary", "Accept-Encoding")
            self.send_header("Cache-Control", "no-cache")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(content)

        except Exception as e:
            self.send_error(500, f"Server error: {str(e)}")

    def get_mime_type(self, filename):
        base, ext = os.path.splitext(filename)
        mime_types = {
            '.html': 'text/html; charset=utf-8',
            '.htm': 'text/html; charset=utf-8',
            '.css': 'text/css',
            '.js': 'application/javascript',
            '.json': 'application/json',
            '.png': 'image/png',
            '.jpg': 'image/jpeg',
            '.jpeg': 'image/jpeg',
            '.gif': 'image/gif',
            '.svg': 'image/svg+xml',
            '.wasm': 'application/wasm',
        }
        return mime_types.get(ext.lower(), 'application/octet-stream')

def main():
    mimetypes.add_type('application/wasm', '.wasm')
    mimetypes.add_type('application/javascript', '.js')
    mimetypes.add_type('text/css', '.css')

    if len(sys.argv) > 1:
        """
        Make locally-trusted development certificates at: https://github.com/FiloSottile/mkcert
        $ mkcert -install
        $ mkcert 192.168.1.5 127.0.0.1
        Copy and rename generated .pem files to certPath (argv[1])
        """
        addr = "0.0.0.0"
        port = 4443
        certPath = str(sys.argv[1])
        httpd = HTTPServer((addr, port), RequestHandler)
        httpd.socket = ssl.wrap_socket(httpd.socket,
                                       server_side=True,
                                       certfile=certPath + "/server.pem",
                                       keyfile=certPath + "/key.pem",
                                       ssl_version=ssl.PROTOCOL_TLS)
        print("Serving HTTPS at https://{}:{}".format(addr, port))
    else:
        addr = "127.0.0.1"
        port = 8000
        httpd = HTTPServer((addr, port), RequestHandler)
        print("Serving HTTP at http://{}:{}".format(addr, port))

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nBye.")


if __name__ == "__main__":
    main()
