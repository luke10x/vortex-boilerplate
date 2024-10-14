build:
	mkdir -p ./assets
	blender -b red-monkey.blend -o assets/red-monkey.gltf --python-expr \
		"import bpy; bpy.ops.export_scene.gltf(filepath='assets/red-monkey.gltf')"
	
	cd ../.. && make clean build

	../../build/program