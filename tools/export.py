import os;
import re;
import shutil;
import sys;
import tarfile;
import zipfile;

lnxDir = 'build_lnx'
glesDir = 'build_gles'
webDir = 'build_web'
win32Dir = 'build_win32'
win64Dir = 'bulid_win64'

def getVersion():
	with open('src/utils/text.h', 'r') as fh:
		return re.search(r'char\s*commonVersion\[.*\]\s*=\s*"(.*)"', fh.read()).group(1)

def writeZip(odir):
	with zipfile.ZipFile(odir + '.zip', 'w', zipfile.ZIP_DEFLATED) as zh:
		for root, dirs, files in os.walk(odir):
			for ft in files:
				zh.write(os.path.join(root, ft))

def writeTar(odir):
	with tarfile.open(odir + '.tar.gz', 'w:gz') as th:
		th.add(odir)

def expCommon(odir, pdir, copyData = True, message = ''):
	if not os.path.isdir(odir):
		os.mkdir(odir)
	os.chdir(odir)
	if copyData:
		shutil.copytree(os.path.join('..', pdir, 'data'), 'data')
	shutil.copytree(os.path.join('..', pdir, 'licenses'), 'licenses')
	shutil.copytree(os.path.join('..', 'doc'), 'doc')
	os.chdir('..')

	with open('README.md', 'r') as fh:
		txt = fh.read()
	txt = re.sub(r'##\sBuild\s*', message, txt, flags = re.M | re.S)
	txt = re.sub(r'The\sCMakeLists\.txt.*', '', txt, flags = re.M | re.S)
	with open(os.path.join(odir, 'README.md'), 'w') as fh:
		fh.write(txt)

def expAndroid(odir):
	expCommon(odir, os.path.join('android', 'app'), False)
	shutil.copy(os.path.join('android', 'app', 'release', 'app-release.apk'), os.path.join(odir, 'Thrones.apk'))
	writeZip(odir)

def expLinux(odir, pdir):
	expCommon(odir, pdir)
	shutil.copy(os.path.join(pdir, 'thrones'), odir)
	shutil.copy(os.path.join(pdir, 'server'), odir)
	shutil.copy(os.path.join(pdir, 'thrones.desktop'), odir)
	writeTar(odir)

def expWeb(odir, pdir):
	expCommon(odir, pdir, False)
	shutil.copy(os.path.join(pdir, 'thrones.html'), odir)
	shutil.copy(os.path.join(pdir, 'thrones.js'), odir)
	shutil.copy(os.path.join(pdir, 'thrones.wasm'), odir)
	shutil.copy(os.path.join(pdir, 'thrones.data'), odir)
	shutil.copy(os.path.join(pdir, 'thrones.png'), odir)
	writeZip(odir)

def expWin(odir, pdir, msg):
	expCommon(odir, pdir, message = msg)
	shutil.copy(os.path.join(pdir, 'Thrones.exe'), odir)
	shutil.copy(os.path.join(pdir, 'Server.exe'), odir)
	for ft in ['SDL2', 'SDL2_image', 'SDL2_net', 'SDL2_ttf', 'libpng16-16', 'libfreetype-6', 'zlib1']:
		shutil.copy(os.path.join(pdir, ft + '.dll'), odir)
	writeZip(odir)

def genProject(pdir, gopt = '', pref = ''):
	if not os.path.isdir(pdir):
		os.mkdir(pdir)
	os.chdir(pdir)
	os.system(pref + 'cmake .. ' + gopt)
	os.chdir('..')

def genWeb():
	genProject(webDir, '', 'emcmake ')
	esRel = os.path.join('..', glesDir, 'bin')
	os.chdir(webDir)
	shutil.copytree(os.path.join(esRel, 'data'), 'data')
	shutil.copytree(os.path.join(esRel, 'licenses'), 'licenses')
	os.rename(os.path.join('data', 'thrones.png'), 'thrones.png')
	os.chdir('..')

if __name__ == '__main__':
	if len(sys.argv) < 2:
		print("usage: " + os.path.basename(__file__) + " <action>")
		quit()

	if sys.argv[1] == 'android':
		expAndroid('Thrones_' + getVersion() + '_android')
	elif sys.argv[1] == 'linux':
		expLinux('Thrones_' + getVersion() + '_linux64', os.path.join(lnxDir, 'bin'))
	elif sys.argv[1] == 'gles':
		expLinux('Thrones_' + getVersion() + '_gles64', os.path.join(glesDir, 'bin'))
	elif sys.argv[1] == 'web':
		expWeb('Thrones_' + getVersion() + '_web', webDir)
	elif sys.argv[1] == 'win':
		expWin('Thrones_' + getVersion() + '_win32', os.path.join(win32Dir, 'bin'), 'To run the program you need to have the vc_redist 2017 32-bit installed.  \n')
		expWin('Thrones_' + getVersion() + '_win64', os.path.join(win64Dir, 'bin'), 'To run the program you need to have the vc_redist 2019 64-bit installed.  \n')
	elif sys.argv[1] == 'glinux':
		genProject(lnxDir)
	elif sys.argv[1] == 'ggles':
		genProject(glesDir, '-DOPENGLES=1')
	elif sys.argv[1] == 'gweb':
		genWeb()
	elif sys.argv[1] == 'gwin':
		genProject(win32Dir, '-G "Visual Studio 15" -A "Win32"')
		genProject(win64Dir, '-G "Visual Studio 15" -A "x64"')
	else:
		print('unknown action')
