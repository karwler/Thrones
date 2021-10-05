from http import server
import sys

if __name__ == '__main__':
	port = 8080
	for ag in sys.argv:
		if ag.isdigit():
			n = int(ag)
			if n < 65536:
				port = n
				break

	handler = server.SimpleHTTPRequestHandler
	handler.extensions_map['.wasm'] = 'application/wasm'
	with server.HTTPServer(("", port), handler) as httpd:
		print(f'started localhost:{port}')
		try:
			httpd.serve_forever()
		except KeyboardInterrupt:
			httpd.shutdown()
			print('')
