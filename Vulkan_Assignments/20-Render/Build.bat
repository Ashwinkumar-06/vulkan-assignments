cls

del *.exe *.res *.obj

cl.exe /TC /c /EHsc /I "C:\\VulkanSDK\\Vulkan\\Include" VK.c

rc.exe VK.rc

link.exe VK.obj VK.res /LIBPATH:"C:\\VulkanSDK\\Vulkan\\Lib" User32.lib GDI32.lib /SUBSYSTEM:WINDOWS
