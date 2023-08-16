"""
Runs a COOE/COEP local webserver for testing emscripten deployment.
Note:
    Browsers that have implemented and enabled SharedArrayBuffer are gating it behind Cross Origin Opener Policy (COOP)
    and Cross Origin Embedder Policy (COEP) headers.
    Pthreads code will not work in deployed environment unless these headers are correctly set.
    see: https://emscripten.org/docs/porting/pthreads.html
"""

from http.server import HTTPServer, SimpleHTTPRequestHandler
import ssl, sys

class RequestHandler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Cross-Origin-Embedder-Policy", "require-corp")
        self.send_header("Cross-Origin-Opener-Policy", "same-origin")
        super().end_headers()


def main():

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
