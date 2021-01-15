import json
import os
import subprocess
import sys
import shutil
import requests
import re
import zipfile

workspaceCfgName = 'workspace.json'
workspaceCfg = {
	'modules': []
}

libraries = [
	{
		'name': 'PhysX',
		'dlls': [
			"PhysXCommon_{architecture}.dll",
			"PhysX_{architecture}.dll",
			"PhysXFoundation_{architecture}.dll",
			"PhysXCooking_{architecture}.dll",
			"PhysXGpu_{architecture}.dll",
		]
	},
	{
		'name': 'GameNetworkingSockets',
		'dlls': [ 'GameNetworkingSockets.dll' ]
	},
	{
		'name': 'OpenSSL',
		'dlls': [
			'libssl-1_1-x{architecture}.dll',
			'libcrypto-1_1-x{architecture}.dll',
			'libprotobufd.dll',
		]
	},
	{
		'name': 'Assimp',
		'dlls': [ 'assimp-vc141-mtd.dll' ]
	}
]

shadercHtml = "https://storage.googleapis.com/shaderc/badges/build_link_windows_vs2017_release.html"

def getGoogleStorageUrl(gUrl):
	htmlPage = requests.get(shadercHtml)
	extracted = re.match('.*content=".*url=(.*)".*', htmlPage.content.decode('utf-8'))
	return extracted.groups()[0]

def bin(config, architecture, package):
	return os.path.join(os.getcwd(), f"Binaries/Build/{config}/x{architecture}/{package}")

def saveWorkspace():
	global workspaceCfgName
	global workspaceCfg
	with open(workspaceCfgName, 'w') as f:
		f.write(json.dumps(workspaceCfg, sort_keys=True, indent=2))

def loadWorkspace():
	global workspaceCfgName
	global workspaceCfg
	with open(workspaceCfgName) as f:
		workspaceCfg = json.load(f)

if not os.path.exists(workspaceCfgName):
	saveWorkspace()	
loadWorkspace()

def scriptPath(name):
	return os.path.join(os.getcwd(), 'scripts', name)

def run(cmd, shell=True, cwd=None, env=None):
	subprocEnv = { **os.environ }
	if not env is None:
		subprocEnv.update(env)

	proc = subprocess.Popen(
		cmd, shell=shell, cwd=cwd, env=subprocEnv,
		stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
		universal_newlines=True,
	)
	while proc.poll() is None:
		line = proc.stdout.readline()
		if line:
			sys.stdout.write(line)
			sys.stdout.flush()

def runScript(name, args=[]):
	run(['bash', scriptPath(name)] + args)

def downloadShaderC(shadercDir):
	shadercZipUrl = getGoogleStorageUrl(shadercHtml)
	shadercZipReq = requests.get(shadercZipUrl)
	shadercZipName = 'shaderc.zip'
	with open(shadercZipName, 'wb') as zipFile:
		zipFile.write(shadercZipReq.content)
	with zipfile.ZipFile(shadercZipName, 'r') as zip_ref:
		zip_ref.extractall(shadercDir)
	os.remove(shadercZipName)

def installShaderC():
	sys.stdout.flush()
	shadercDir = os.path.join(os.getcwd(), 'shaderc')
	if os.path.exists(shadercDir):
		shutil.rmtree(shadercDir)
	downloadShaderC(shadercDir)
	shadercDst = os.path.join(os.getcwd(), 'Core/TemportalEngineEditor/libs/shaderc')
	shadercDstInclude = os.path.join(shadercDst, 'include/shaderc')
	shadercDstLib = os.path.join(shadercDst, 'lib/')
	if os.path.exists(shadercDstInclude):
		shutil.rmtree(shadercDstInclude)
	shutil.copytree(
		os.path.join(shadercDir, 'install/include/shaderc'),
		shadercDstInclude
	)
	os.makedirs(shadercDstLib, exist_ok=True)
	shutil.copyfile(
		os.path.join(shadercDir, 'install/lib/shaderc_combined.lib'),
		os.path.join(shadercDstLib, 'shaderc_combined.lib')
	)

config = 'Debug'
architecture = '64'
args = sys.argv[1:]
if args[0] == 'setup':
	print('Setting up workspace')
	
	print(f"Building PhysX checked x{architecture}")
	#runScript('physx-build.sh', ['checked', architecture])

	print('Setting up GameNetworkingSockets')
	#runScript("gns-setup.sh")
	
	print('Building GameNetworkingSockets')
	#runScript("gns-build.sh")
	
	print('Building assimp')
	runScript("assimp-build.sh")

	print('Downloading ShaderC')
	#installShaderC()

elif args[0] == 'updateLibs':
	moduleNames = [ 'MinecraftGame', 'MinecraftEditor' ]
	for moduleName in workspaceCfg['modules']:
		moduleNames.append(moduleName)
	
	print('Distributing library dlls to:')
	moduleBins = []
	for moduleName in moduleNames:
		moduleBin = bin(config, architecture, moduleName)
		print(f"- {moduleName}")
		os.makedirs(moduleBin, exist_ok=True)
		moduleBins.append(moduleBin)

	print("Libraries:")
	for lib in libraries:
		if 'dlls' in lib:
			libBin = bin(config, architecture, lib['name'])
			print(f"- {lib['name']}")
			for dllName in lib['dlls']:
				dllName = dllName.format(architecture=architecture)
				print(f"  - {dllName}")
				for moduleBin in moduleBins:
					shutil.copyfile(
						os.path.join(libBin, dllName),
						os.path.join(moduleBin, dllName)
					)