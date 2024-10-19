all:
	mkdir -p ./assets

	# @See: https://docs.blender.org/api/current/bpy.ops.export_scene.html
	blender -b ../assets/animated-human/Blend/Animated\ Human.blend -o assets/human.glb --python-expr \
		"import bpy; bpy.ops.export_scene.gltf(filepath='assets/human.glb')"
	
	cd ../.. && make clean build APP_ROOT=$(PWD)

ifeq ($(NATIVE),1)
	../../build/program
else
	bash -c '(cd ../../build && python3 -mhttp.server)'
endif