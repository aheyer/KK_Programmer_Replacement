import shutil
Import("env")

def copy_action(target, source, env):
	dest = env.File(env.GetProjectOption("custom_device_name") + ".bin", env.Dir("firmware", env.Dir("$PROJECT_DIR"))).get_abspath()
	print(f"Copying {target[0]} to \033[92m{dest}\033[0m")
	shutil.copyfile(target[0].get_abspath(), dest)
	return 0

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_action)
