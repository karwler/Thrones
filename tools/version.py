import os
import re
import sys

def readlin(fpath):
	with open(fpath, 'r') as fh:
		return fh.readlines()

def writelin(fpath, lines):
	with open(fpath, 'w') as fh:
		fh.writelines(lines)

def findSubstr(lines, sub, pos = 0):
	pat = re.compile(sub)
	for i in range(pos, len(lines)):
		if pat.search(lines[i]):
			return i
	raise Exception('failed to find ' + sub)

def findReplace(lines, cond, regs, repl, pos = 0):
	lid = findSubstr(lines, cond, pos)
	lines[lid] = re.sub(regs, repl, lines[lid])

def incCode(lines, regs, form, inc):
	lid = findSubstr(lines, 'versionCode')
	code = int(re.search(r'\d+', lines[lid]).group(0)) + inc
	lines[lid] = re.sub(regs, form.format(code), lines[lid])

def setVersion(fpath):
	lines = readlin(fpath)
	findReplace(lines, r'char\s*commonVersion\[.*\]\s*=', '".*"', '"' + sys.argv[1] + '"')
	writelin(fpath, lines)

def setGradle(fpath, inc):
	lines = readlin(fpath)
	incCode(lines, r'\d', '{}', inc)
	findReplace(lines, 'versionName', '".*"', '"' + sys.argv[1] + '"')
	writelin(fpath, lines)

def setAndroid(fpath, inc):
	lines = readlin(fpath)
	incCode(lines, 'versionCode=".*?"', 'versionCode="{}"', inc)
	findReplace(lines, 'versionName', 'versionName=".*?"', 'versionName="' + sys.argv[1] + '"')
	writelin(fpath, lines)

def setInfo(fpath):
	lines = readlin(fpath)
	rstr = '<string>.*</string>'
	lid = findSubstr(lines, '<key>CFBundleVersion</key>')
	findReplace(lines, rstr, rstr, '<string>' + sys.argv[1] + '</string>', lid + 1)
	writelin(fpath, lines)

def setResource(fpath):
	lines = readlin(fpath)
	rver = sys.argv[1].replace('.', ',')
	rnum = re.compile(r'\d+,\d+,\d+,\d+')
	rstr = re.compile(r',\s*".*"')

	findReplace(lines, 'FILEVERSION', rnum, rver + ',0')
	findReplace(lines, 'PRODUCTVERSION', rnum, rver + ',0')
	findReplace(lines, '"FileVersion"', rstr, ', "' + sys.argv[1] + '"')
	findReplace(lines, '"ProductVersion"', rstr, ', "' + sys.argv[1] + '"')
	writelin(fpath, lines)

def setDesktop(fpath):
	lines = readlin(fpath)
	findReplace(lines, 'Version\s*=', '=.*', '=' + sys.argv[1])
	writelin(fpath, lines)

if __name__ == '__main__':
	if len(sys.argv) < 2:
		print("usage: " + os.path.basename(__file__) + " <new version> <code increment = 1>")
		quit()
	inc = 1 if len(sys.argv) < 3 else int(sys.argv[2])

	try:
		setVersion('src/server/server.h')
		setGradle('android/app/build.gradle', inc)
		setAndroid('android/app/src/main/AndroidManifest.xml', inc)
		setInfo('rsc/Info.plist')
		setResource('rsc/server.rc')
		setResource('rsc/thrones.rc')
		setDesktop('rsc/thrones.desktop')
	except Exception as e:
		print(e)
