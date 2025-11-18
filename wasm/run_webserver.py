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
    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        super().end_headers()

    def do_GET(self):
        path = self.path.split('?')[0]
        path = path.split('#')[0]
        file_path = self.translate_path(path)
        if os.path.exists(file_path):
            if file_path.endswith('.gz'):
                self.serve_gzip_file(file_path)
            else:
                super().do_GET()
        else:
            gz_path = file_path + '.gz'
            if os.path.exists(gz_path):
                self.serve_gzip_file(gz_path)
            else:
                self.send_error(404, f"File not found: {path}")

    def serve_gzip_file(self, gz_path):
        try:
            with open(gz_path, 'rb') as f:
                gz_content = f.read()
                file_size = len(gz_content)
            original_filename = os.path.basename(gz_path)[:-3]  # remove .gz
            mime_type = self.get_mime_type(original_filename)

            self.send_response(200)
            self.send_header("Content-Type", mime_type)
            self.send_header("Content-Length", str(file_size))
            self.send_header("Content-Encoding", "gzip")
            self.send_header("Cache-Control", "no-cache")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
            self.wfile.write(gz_content)

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
