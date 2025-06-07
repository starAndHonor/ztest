add_requires("glfw", "imgui", "glad","implot","curl","nlohmann_json","libcurl")
add_requires("imgui 1.91.7-docking", {configs = {glfw_opengl3 = true}})
target("test_gui")
    set_kind("binary")
    set_targetdir("build")
    add_runenvs("DASHSCOPE_API_KEY","sk-7d6389df8ba540feb5578eb9103c9f0d")
    add_packages("glfw")
    add_packages("imgui")
    add_packages("glad")
    add_packages("implot","curl","nlohmann_json","libcurl")
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
    set_toolchains("gcc")
    set_languages("c++20")
