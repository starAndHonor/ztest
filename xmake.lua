-- add_requires("glfw", "imgui", "glad","implot")
-- add_requires("imgui 1.91.7-docking", {configs = {glfw_opengl3 = true}})
-- target("test_gui")
--     set_kind("binary")
--     add_packages("glfw")
--     add_packages("imgui")
--     add_packages("glad")
--     add_files("*.cpp")
--     add_files(
--         "*.cpp",
--         "./ztest/lib/implot/*.cpp"  
--     )
    
--     add_includedirs(
--         ".",
--         "./ztest/lib/implot" 
--     )
--     set_languages("c++20")

target("test_gui")
    set_kind("binary")
    set_languages("c++17")
    add_files("*.cpp")
    if is_plat("windows") then
        add_defines("WIN32_LEAN_AND_MEAN")
        add_defines("NOMINMAX")  -- 防止 min/max 宏冲突
    end
    add_syslinks("pdh")
    add_files(
        "*.cpp",
        "./ztest/lib/implot/*.cpp",
        "./ztest/lib/glad/src/*.cpp",
        "./ztest/lib/imgui-1.91.7-docking/*.cpp",
        "./ztest/lib/imgui-1.91.7-docking/backends/*.cpp"
    )
    
    add_includedirs(
        ".",
        "./ztest/lib/implot" ,
        "./ztest/lib/imgui-1.91.7-docking/backends",
        "./ztest/lib/imgui-1.91.7-docking",
        "./ztest/lib/glfw/include",
        "./ztest/lib/glad/include"
    )
    
    add_linkdirs("./ztest/lib/glfw/lib-mingw-w64")
    add_links("glfw3","opengl32","gdi32","user32","psapi")
