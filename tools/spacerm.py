import os
import re

def stripFile(fpath):
	with open(fpath, 'r') as fh:
		orit = fh.read()
	
	txt = orit.strip() + os.linesep
	txt = re.sub(r'[ \t]+\r?\n', os.linesep, txt, flags = re.M)
	txt = re.sub(r' {2,}', ' ', txt, flags = re.M)
	txt = re.sub(r'([ \t]*\r?\n){3,}', os.linesep + os.linesep, txt, flags = re.M)
	
	if txt != orit:
		print(fpath)
		with open(fpath, 'w') as fh:
			fh.write(txt)

def stripDir(dpath, *exclude):
	for root, dirs, files in os.walk(dpath):
		for ft in files:
			fpath = os.path.join(root, ft)
			if fpath not in exclude:
				stripFile(fpath)

if __name__ == '__main__':
	stripDir('doc')
	stripDir(os.path.join('rsc', 'materials'))
	stripDir(os.path.join('rsc', 'objects'))
	stripDir(os.path.join('rsc', 'shaders'))
	stripDir('src', os.path.join('src', 'test', 'text.cpp'))
	stripDir('tools', os.path.join('tools', 'export.py'), os.path.join('tools', 'spacerm.py'))

	files = (
		'CMakeLists.txt',
		os.path.join('rsc', 'Info.plist'),
		os.path.join('rsc', 'server.rc'),
		os.path.join('rsc', 'thrones.desktop'),
		os.path.join('rsc', 'thrones.html'),
		os.path.join('rsc', 'thrones.rc')
	)
	for ft in files:
		stripFile(ft)
