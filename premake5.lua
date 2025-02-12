workspace "ViBoard"
   configurations { "Debug", "Release" }
   platforms { "x64", "x86" }
   startproject "ViBoard"
   
include "ViBoard"
group "dependencies"
   include "dependencies/imgui"