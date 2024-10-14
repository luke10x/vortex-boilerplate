build:
	mkdir -p ./assets
	blender -b red-monkey.blend -o assets/red-monkey.gltf --python-expr \
		"import bpy; bpy.ops.export_scene.gltf(filepath='assets/red-monkey.gltf')"
	
	cd ../.. && make clean build ASSET_ROOT=$(PWD)
ifeq ($(NATIVE),1)
	../../build/program
else
	bash -c '(cd ../../build && python3 -mhttp.server)'
endif