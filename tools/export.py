import os
import psutil
import re
import shutil
import subprocess
import sys
import tarfile
import zipfile

def mkdir(path):
	if not os.path.isdir(path):
		os.makedirs(path)

def prun(*cmd):
	subprocess.check_call(filter(lambda s: s and not s.isspace(), cmd))

def getVersion():
	with open('src/server/server.h', 'r') as fh:
		return re.search(r'char\s*commonVersion\[.*\]\s*=\s*"(.*)"', fh.read()).group(1)

def getThreads():
	jobs = None
	for it in sys.argv:
		if it[0] == 'j':
			jobs = it[1:]
	if jobs == None:
		jobs = str(psutil.cpu_count(False))
	return jobs if jobs else '1'

def writeZip(odir):
	with zipfile.ZipFile(odir + '.zip', 'w', zipfile.ZIP_DEFLATED) as zh:
		for root, dirs, files in os.walk(odir):
			for ft in files:
				zh.write(os.path.join(root, ft))

def writeTar(odir):
	with tarfile.open(odir + '.tar.gz', 'w:gz') as th:
		th.add(odir)

def expCommon(odir, pdir, regGL = True, noGlew = False, noCurl = False, copyLicn = True, message = ''):
	mkdir(odir)
	if regGL:
		shutil.copytree(os.path.join(pdir, 'data'), os.path.join(odir, 'data'))
	if copyLicn:
		shutil.copytree(os.path.join(pdir, 'licenses'), os.path.join(odir, 'licenses'))
	shutil.copytree('doc', os.path.join(odir, 'doc'))

	regFlags = re.I | re.M | re.S
	with open('README.md', 'r') as fh:
		txt = fh.read()
	txt = re.sub(r'##\sBuild\s*', message, txt, flags = regFlags)
	txt = re.sub(r'The\sCMakeLists\.txt.*', '', txt, flags = regFlags)
	if noCurl:
		txt = re.sub(r'libcurl,\s*', '', txt, flags = regFlags)
	if noGlew:
		txt = re.sub(r'GLEW,\s*', '', txt, flags = regFlags)
	if regGL:
		txt = re.sub(r',\s*libjpeg', '', txt, flags = regFlags)
	with open(os.path.join(odir, 'README.md'), 'w') as fh:
		fh.write(txt)

def expAndroid(odir):
	expCommon(odir, os.path.join('android', 'app'), regGL = False, noGlew = True, noCurl = True)
	shutil.copy(os.path.join('android', 'app', 'release', 'app-release.apk'), os.path.join(odir, 'Thrones.apk'))
	writeZip(odir)

def expMac(odir, pdir):
	expCommon(odir, pdir, noGlew = True, noCurl = True, copyLicn = False)
	shutil.copy(os.path.join(pdir, 'Thrones.app'), odir)
	shutil.copy(os.path.join(pdir, 'server'), odir)
	for root, dirs, files in os.walk(odir):
		for ft in files:
			if ft == '.DS_Store':
				os.remove(os.path.join(root, ft))
	writeZip(odir)

def expLinux(odir, pdir):
	expCommon(odir, pdir)
	shutil.copy(os.path.join(pdir, 'thrones'), odir)
	shutil.copy(os.path.join(pdir, 'server'), odir)
	shutil.copy(os.path.join(pdir, 'thrones.desktop'), odir)
	writeTar(odir)

def expWeb(odir, pdir):
	expCommon(odir, pdir, regGL = False, noGlew = True, noCurl = True)
	shutil.copy(os.path.join(pdir, 'thrones.html'), odir)
	shutil.copy(os.path.join(pdir, 'thrones.js'), odir)
	shutil.copy(os.path.join(pdir, 'thrones.wasm'), odir)
	shutil.copy(os.path.join(pdir, 'thrones.data'), odir)
	shutil.copy(os.path.join(pdir, 'thrones.png'), odir)
	shutil.copy(os.path.join('tools', 'server.py'), odir)
	writeZip(odir)

def expWin(odir, pdir, msg):
	expCommon(odir, pdir, message = msg)
	shutil.copy(os.path.join(pdir, 'Thrones.exe'), odir)
	shutil.copy(os.path.join(pdir, 'Server.exe'), odir)
	for ft in ['SDL2', 'SDL2_image', 'SDL2_ttf', 'libcurl.dll', 'libpng16-16', 'libfreetype-6', 'zlib1']:
		shutil.copy(os.path.join(pdir, ft + '.dll'), odir)
	writeZip(odir)

def genProject(pdir, pref = '', *gopt):
	mkdir(pdir)
	os.chdir(pdir)
	if 'debug' in sys.argv:
		prun(pref, 'cmake', '..', *gopt, '-DCMAKE_BUILD_TYPE=Debug')
	else:
		prun(pref, 'cmake', '..', *gopt)

def genLinux(pdir, target = '', *gopt):
	genProject(pdir, '', *gopt)
	prun('make', '-j', getThreads(), target)

def genWeb(pdir):
	wglDir = 'build_wgl'
	genLinux(wglDir, 'assets', '-DOPENGLES=1')
	os.chdir('..')
	genProject(pdir, 'emcmake')

	wglRel = os.path.join('..', wglDir, 'bin')
	os.rename(os.path.join(wglRel, 'data'), 'data')
	os.rename(os.path.join(wglRel, 'licenses'), 'licenses')
	os.rename(os.path.join('data', 'thrones.png'), 'thrones.png')
	shutil.copy(os.path.join('..', 'tools', 'server.py'), 'server.py')
	prun('emmake', 'make', '-j', getThreads())

if __name__ == '__main__':
	if len(sys.argv) < 2:
		print("usage: " + os.path.basename(__file__) + " <action>")
		quit()

	lnxDir = 'build_lnx'
	glesDir = 'build_gles'
	webDir = 'build_web'
	win32Dir = 'build_win32'
	win64Dir = 'build_win64'
	if sys.argv[1] == 'android':
		expAndroid('Thrones_' + getVersion() + '_android')
	elif sys.argv[1] == 'linux':
		expLinux('Thrones_' + getVersion() + '_linux64', os.path.join(lnxDir, 'bin'))
	elif sys.argv[1] == 'gles':
		expLinux('Thrones_' + getVersion() + '_gles64', os.path.join(glesDir, 'bin'))
	elif sys.argv[1] == 'web':
		expWeb('Thrones_' + getVersion() + '_web', webDir)
	elif sys.argv[1] == 'mac':
		exprMac('Thrones_' + getVersion() + '_mac64', 'build')
	elif sys.argv[1] == 'win':
		expWin('Thrones_' + getVersion() + '_win32', os.path.join(win32Dir, 'bin'), 'To run the program you need to have the Microsoft Visual C++ Redistributable 2019 32-bit installed.  \n')
		expWin('Thrones_' + getVersion() + '_win64', os.path.join(win64Dir, 'bin'), 'To run the program you need to have the Microsoft Visual C++ Redistributable 2019 64-bit installed.  \n')
	elif sys.argv[1] == 'glinux':
		genLinux(lnxDir)
	elif sys.argv[1] == 'ggles':
		genLinux(glesDir, '', '-DOPENGLES=1')
	elif sys.argv[1] == 'gweb':
		genWeb(webDir)
	elif sys.argv[1] == 'gwin':
		genProject(win32Dir, '', '-G', '"Visual Studio 16"', '-A', '"Win32"')
		genProject(win64Dir, '', '-G', '"Visual Studio 16"', '-A', '"x64"')
	else:
		print('unknown action')
