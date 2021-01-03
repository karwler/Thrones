import os
import psutil
import re
import shutil
import subprocess
import sys
import tarfile
import zipfile

def mkdir(path: str) -> None:
	if not os.path.isdir(path):
		os.makedirs(path)

def prun(*cmd: str) -> None:
	subprocess.check_call(filter(lambda s: s and not s.isspace(), cmd))

def getVersion() -> str:
	with open('src/server/server.h', 'r') as fh:
		return re.search(r'char\s*commonVersion\[.*\]\s*=\s*"(.*)"', fh.read()).group(1)

def getThreads() -> str:
	jobs = None
	for it in sys.argv:
		if it[0] == 'j':
			jobs = it[1:]
	if jobs == None:
		jobs = str(psutil.cpu_count(False))
	return jobs if jobs else '1'

def writeZip(odir: str) -> None:
	with zipfile.ZipFile(odir + '.zip', 'w', zipfile.ZIP_DEFLATED) as zh:
		for root, dirs, files in os.walk(odir):
			for ft in files:
				zh.write(os.path.join(root, ft))

def writeTar(odir: str) -> None:
	with tarfile.open(odir + '.tar.gz', 'w:gz') as th:
		th.add(odir)

def expCommon(odir: str, pdir: str, docPath: str = 'doc', regGL: bool = True, noGlew: bool = False, noCurl: bool = False, message: str = '') -> None:
	mkdir(odir)

	regFlags = re.I | re.M | re.S
	with open('README.md', 'r') as fh:
		txt = fh.read()
	txt = re.sub(r'\(doc/', '(' + docPath + '/', txt, flags = regFlags)
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

def expAndroid(odir: str) -> None:
	expCommon(odir, os.path.join('android', 'app'), regGL = False, noGlew = True, noCurl = True)
	shutil.copy(os.path.join('android', 'app', 'release', 'app-release.apk'), os.path.join(odir, 'Thrones.apk'))
	shutil.copytree('doc', os.path.join(odir, 'doc'))
	writeZip(odir)

def expMac(odir: str, pdir: str) -> None:
	expCommon(odir, pdir, 'Thrones.app/Contents/Resources/doc', noGlew = True, noCurl = True)
	shutil.copytree(os.path.join(pdir, 'Thrones.app'), odir)
	shutil.copy(os.path.join(pdir, 'server'), odir)
	for root, dirs, files in os.walk(odir):
		for ft in files:
			if ft == '.DS_Store':
				os.remove(os.path.join(root, ft))
	writeZip(odir)

def expLinux(odir: str, pdir: str) -> None:
	expCommon(odir, pdir, 'share/doc')
	shutil.copytree(os.path.join(pdir, 'bin'), os.path.join(odir, 'bin'))
	shutil.copytree(os.path.join(pdir, 'share'), os.path.join(odir, 'share'))
	shutil.copy(os.path.join(pdir, 'thrones.desktop'), odir)
	writeTar(odir)

def expAppImage(odir: str, pdir: str) -> None:
	expCommon(odir, pdir)
	appimg = next(img for img in os.listdir(pdir) if re.fullmatch(r'Thrones\S+\.AppImage', img))
	shutil.copy(os.path.join(pdir, appimg), os.path.join(odir, 'Thrones.AppImage'))
	shutil.copy(os.path.join(pdir, 'server'), odir)
	shutil.copytree(os.path.join(pdir, 'Thrones.AppDir', 'share', 'doc'), os.path.join(odir, 'doc'))
	writeTar(odir)

def expWeb(odir: str, pdir: str) -> None:
	expCommon(odir, pdir, regGL = False, noGlew = True, noCurl = True)
	shutil.copy(os.path.join(pdir, 'thrones.html'), odir)
	shutil.copy(os.path.join(pdir, 'thrones.js'), odir)
	shutil.copy(os.path.join(pdir, 'thrones.wasm'), odir)
	shutil.copy(os.path.join(pdir, 'thrones.data'), odir)
	shutil.copy(os.path.join(pdir, 'thrones.png'), odir)
	shutil.copytree(os.path.join(pdir, 'doc'), os.path.join(odir, 'doc'))
	shutil.copytree(os.path.join(pdir, 'licenses'), os.path.join(odir, 'licenses'))
	shutil.copy(os.path.join('tools', 'server.py'), odir)
	writeZip(odir)

def expWin(odir: str, pdir: str, msg: str) -> None:
	expCommon(odir, pdir, message = msg)
	shutil.copy(os.path.join(pdir, 'Thrones.exe'), odir)
	shutil.copy(os.path.join(pdir, 'Server.exe'), odir)
	shutil.copytree(os.path.join(pdir, 'share'), os.path.join(odir, 'share'))
	shutil.copytree(os.path.join(pdir, 'doc'), os.path.join(odir, 'doc'))
	shutil.copytree(os.path.join(pdir, 'licenses'), os.path.join(odir, 'licenses'))
	for ft in ['SDL2', 'SDL2_image', 'SDL2_ttf', 'libcurl', 'libpng16-16', 'libfreetype-6', 'zlib1']:
		shutil.copy(os.path.join(pdir, ft + '.dll'), odir)
	writeZip(odir)

def genProject(pdir: str, pref: str = '', *gopt: str) -> None:
	mkdir(pdir)
	os.chdir(pdir)
	if 'debug' in sys.argv:
		prun(pref, 'cmake', '..', *gopt, '-DCMAKE_BUILD_TYPE=Debug')
	else:
		prun(pref, 'cmake', '..', *gopt)

def genLinux(pdir: str, target: str = '', *gopt: str) -> None:
	genProject(pdir, '', *gopt)
	prun('make', '-j', getThreads(), target)

def genWeb(pdir: str) -> None:
	wglDir = 'build_wgl'
	genLinux(wglDir, 'assets', '-DOPENGLES=1')
	os.chdir('..')
	genProject(pdir, 'emcmake')

	wglRel = os.path.join('..', wglDir, 'Thrones')
	shutil.copy(os.path.join('..', 'tools', 'server.py'), 'server.py')
	shutil.copytree(os.path.join(wglRel, 'share'), 'share')
	os.rename(os.path.join('share', 'thrones.png'), 'thrones.png')
	os.rename(os.path.join('share', 'doc'), 'doc')
	os.rename(os.path.join('share', 'licenses'), 'licenses')
	prun('emmake', 'make', '-j', getThreads())

if __name__ == '__main__':
	if len(sys.argv) < 2:
		print("usage: " + os.path.basename(__file__) + " <action>")
		sys.exit()

	appimgDir = 'build_appimgage'
	linuxDir = 'build_linux'
	glesDir = 'build_gles'
	webDir = 'build_web'
	win32Dir = 'build_win32'
	win64Dir = 'build_win64'
	if sys.argv[1] == 'android':
		expAndroid('Thrones_' + getVersion() + '_android')
	elif sys.argv[1] == 'linux':
		expLinux('Thrones_' + getVersion() + '_linux64', os.path.join(linuxDir, 'Thrones'))
	elif sys.argv[1] == 'appimage':
		expAppImage('Thrones_' + getVersion() + '_linux64', appimgDir)
	elif sys.argv[1] == 'gles':
		expLinux('Thrones_' + getVersion() + '_gles64', os.path.join(glesDir, 'Thrones'))
	elif sys.argv[1] == 'web':
		expWeb('Thrones_' + getVersion() + '_web', webDir)
	elif sys.argv[1] == 'mac':
		expMac('Thrones_' + getVersion() + '_mac64', 'build')
	elif sys.argv[1] == 'win32':
		expWin('Thrones_' + getVersion() + '_win32', os.path.join(win32Dir, 'Thrones'), 'To run the program you need to have the Microsoft Visual C++ Redistributable 2019 32-bit installed.  \n')
	elif sys.argv[1] == 'win64':
		expWin('Thrones_' + getVersion() + '_win64', os.path.join(win64Dir, 'Thrones'), 'To run the program you need to have the Microsoft Visual C++ Redistributable 2019 64-bit installed.  \n')
	elif sys.argv[1] == 'glinux':
		genLinux(linuxDir)
	elif sys.argv[1] == 'gappimage':
		genLinux(appimgDir, '', '-DAPPIMAGE=1')
	elif sys.argv[1] == 'ggles':
		genLinux(glesDir, '', '-DOPENGLES=1')
	elif sys.argv[1] == 'gweb':
		genWeb(webDir)
	elif sys.argv[1] == 'gwin32':
		genProject(win32Dir, '', '-G', 'Visual Studio 16', '-A', 'Win32')
	elif sys.argv[1] == 'gwin64':
		genProject(win64Dir, '', '-G', 'Visual Studio 16', '-A', 'x64')
	else:
		print('unknown action')
