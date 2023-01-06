import os
import re
import sys

def readlin(fpath: str) -> list[str]:
	with open(fpath, 'r') as fh:
		return fh.readlines()

def writelin(fpath: str, lines: list[str]) -> None:
	with open(fpath, 'w') as fh:
		fh.writelines(lines)

def findSubstr(lines: list[str], sub: str, pos: int = 0) -> int:
	pat = re.compile(sub)
	for i in range(pos, len(lines)):
		if pat.search(lines[i]):
			return i
	raise Exception('failed to find ' + sub)

def findReplace(lines: list[str], cond: str, regs: str, repl: str, pos: int = 0) -> None:
	lid = findSubstr(lines, cond, pos)
	lines[lid] = re.sub(regs, repl, lines[lid])

def incCode(lines: list[str], regs: str, form: str, inc: int) -> None:
	lid = findSubstr(lines, 'versionCode')
	code = int(re.search(r'\d+', lines[lid]).group(0)) + inc
	lines[lid] = re.sub(regs, form.format(code), lines[lid])

def setVersion(fpath: str) -> None:
	lines = readlin(fpath)
	findReplace(lines, r'string_view\s+commonVersion\s*=', '".*"', '"' + sys.argv[1] + '"')
	writelin(fpath, lines)

def setGradle(fpath: str, inc: int) -> None:
	lines = readlin(fpath)
	incCode(lines, r'\d', '{}', inc)
	findReplace(lines, 'versionName', '".*"', '"' + sys.argv[1] + '"')
	writelin(fpath, lines)

def setAndroid(fpath: str, inc: int) -> None:
	lines = readlin(fpath)
	incCode(lines, 'versionCode=".*?"', 'versionCode="{}"', inc)
	findReplace(lines, 'versionName', 'versionName=".*?"', 'versionName="' + sys.argv[1] + '"')
	writelin(fpath, lines)

def setInfo(fpath: str) -> None:
	lines = readlin(fpath)
	rstr = '<string>.*</string>'
	lid = findSubstr(lines, '<key>CFBundleVersion</key>')
	findReplace(lines, rstr, rstr, '<string>' + sys.argv[1] + '</string>', lid + 1)
	writelin(fpath, lines)

def setResource(fpath: str) -> None:
	lines = readlin(fpath)
	rver = sys.argv[1].replace('.', ',')
	rnum = re.compile(r'\d+,\d+,\d+,\d+')
	rstr = re.compile(r',\s*".*"')

	findReplace(lines, 'FILEVERSION', rnum, rver + ',0')
	findReplace(lines, 'PRODUCTVERSION', rnum, rver + ',0')
	findReplace(lines, '"FileVersion"', rstr, ', "' + sys.argv[1] + '"')
	findReplace(lines, '"ProductVersion"', rstr, ', "' + sys.argv[1] + '"')
	writelin(fpath, lines)

def setCmake(fpath: str) -> None:
	lines = readlin(fpath)
	findReplace(lines, r'VERSION\s+\s+', r'\d+\.d+\.\d+', sys.argv[1])
	writelin(fpath, lines)

def setDoxyfile(fpath: str) -> None:
	lines = readlin(fpath)
	findReplace(lines, r'PROJECT_NUMBER\s*=', r'=.*', '= ' + sys.argv[1])
	writelin(fpath, lines)

if __name__ == '__main__':
	if len(sys.argv) < 2:
		print('usage:', os.path.basename(__file__), '<new version> <code increment = 1>')
		quit()
	inc = 1 if len(sys.argv) < 3 else int(sys.argv[2])

	try:
		setVersion('src/utils/alias.h')
		setGradle('android/app/build.gradle', inc)
		setAndroid('android/app/src/main/AndroidManifest.xml', inc)
		setInfo('rsc/Info.plist')
		setResource('rsc/server.rc')
		setResource('rsc/thrones.rc')
		setCmake('CMakeLists.txt')
		setDoxyfile('Doxyfile')
	except Exception as e:
		print(e)
