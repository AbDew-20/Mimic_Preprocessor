.\thirdParty\dxc\bin\dxc.exe -E main -T vs_6_0 -Fo .\shaders\vs.cso -Zi -Fd .\shaders\vs.pdb .\shaders\vs.hlsl
.\thirdParty\dxc\bin\dxc.exe -E main -T ps_6_0 -Fo .\shaders\ps.cso -Zi -Fd .\shaders\ps.pdb .\shaders\ps.hlsl
.\thirdParty\dxc\bin\dxc.exe -E main -T vs_6_0 -Fo .\shaders\vs_debug.cso -Zi -Fd .\shaders\vs_debug.pdb .\shaders\vs_debug.hlsl
.\thirdParty\dxc\bin\dxc.exe -E main -T ps_6_0 -Fo .\shaders\ps_debug.cso -Zi -Fd .\shaders\ps_debug.pdb .\shaders\ps_debug.hlsl
pause