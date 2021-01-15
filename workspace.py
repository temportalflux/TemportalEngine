import json
import os
import subprocess
import sys
import shutil

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
	}
]

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

def copyLibrariesTo(config, arch, package):
	dst = bin(config, arch, package)
	os.makedirs(dst, exist_ok=True)
	for lib in libraries:
		libBin = bin(config, arch, lib['name'])
		if 'dlls' in lib:
			for dllName in lib['dlls']:
				dllName = dllName.format(architecture = arch)
				print(f"Copying {dllName} to {package}")
				shutil.copyfile(
					os.path.join(libBin, dllName),
					os.path.join(dst, dllName)
				)

config = 'Debug'
architecture = '64'
args = sys.argv[1:]
if args[0] == 'setup':
	print('Setting up workspace')
	
	print(f"Building PhysX checked x{architecture}")
	runScript('physx-build.sh', ['checked', architecture])

	print('Setting up GameNetworkingSockets')
	runScript("gns-setup.sh")
	
	print('Building GameNetworkingSockets')
	runScript("gns-build.sh")

elif args[0] == 'updateLibs':
	copyLibrariesTo(config, architecture, 'MinecraftGame')
	copyLibrariesTo(config, architecture, 'MinecraftEditor')
	for moduleName in workspaceCfg['modules']:
		copyLibrariesTo(config, architecture, moduleName)