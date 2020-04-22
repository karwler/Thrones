import slimmer;
import sys;

if __name__ == '__main__':
	for fpath in sys.argv[1:]:
		with open(fpath, 'r') as fh:
			txt = fh.read()

		if fpath.endswith('.html'):
			txt = slimmer.html_slimmer(txt)
		elif fpath.endswith('.css'):
			txt = slimmer.css_slimmer(txt)
		elif fpath.endswith('.js'):
			txt = slimmer.js_slimmer(txt)
		else:
			print('unknown format of', fpath)

		with open(fpath, 'w') as fh:
			fh.write(txt)
