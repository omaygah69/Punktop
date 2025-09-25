{
  pkgs ? import <nixpkgs> { },
}:

pkgs.mkShell {
  buildInputs = with pkgs; [
    cmake
    gcc
    clang
    pkg-config

    # Graphics + Windowing
    glfw
    glew
    libGL
    libGLU
    wayland
    libxkbcommon
    xorg.libX11
    xorg.libXrandr
    xorg.libXinerama
    xorg.libXcursor
    xorg.libXi

    # SDL2 for ImGui backends
    SDL2

    # Fonts
    freetype
    fontconfig
    #json derulo
    jansson

  ];

  shellHook = ''
  ...
  '';
}

