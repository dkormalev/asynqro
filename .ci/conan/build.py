from cpt.packager import ConanMultiPackager

def test_build():
    builder = ConanMultiPackager(channel="build-testing", upload_only_when_stable=True, upload_only_recipe=True)
    builder.add({"arch": "x86_64", "build_type": "Debug"}, {"asynqro:build_tests": True})
    builder.run()

if __name__ == "__main__":
    test_build()
    builder = ConanMultiPackager(channel="stable", docker_32_images=True)
    builder.add_common_builds(shared_option_name="asynqro:shared", pure_c=False)
    builder.run()
