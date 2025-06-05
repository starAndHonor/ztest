add_requires("glfw", "imgui", "glad","implot","curl","nlohmann_json")
add_requires("imgui 1.91.7-docking", {configs = {glfw_opengl3 = true}})
target("test_gui")
    set_kind("binary")
    add_packages("glfw")
    add_packages("imgui")
    add_packages("glad")
    add_packages("implot","curl","nlohmann_json")
    add_files("*.cpp")
    add_files(
        "*.cpp",
        "./ztest/lib/implot/*.cpp"  
    )
    
    add_includedirs(
        ".",
        "./ztest/lib/implot" 
    )
    add_links("curl")
    set_toolchains("clang")
    set_languages("c++20")
