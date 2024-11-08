CXXFLAGS_EXTRA = -I$(PWD)/ogldev
all:
	mkdir -p ./assets

	# @See: https://docs.blender.org/api/current/bpy.ops.export_scene.html
	blender -b ../assets/test/texture-test.blend -o assets/texture-test.glb --python-expr \
		"import bpy; bpy.ops.export_scene.gltf(filepath='assets/texture-test.glb', export_yup="True")"
	
	cp ../assets/sprites/heart.png assets/heart.png
	cp ../assets/04b03/04B_03__.TTF assets/04b03.ttf

	cd ../.. && make clean build APP_ROOT=$(PWD) CXXFLAGS_EXTRA="${CXXFLAGS_EXTRA}"

ifeq ($(NATIVE),1)
	../../build/program
else
	bash -c '(cd ../../build && python3 -mhttp.server)'
endif