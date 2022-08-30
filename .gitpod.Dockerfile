FROM emscripten/emsdk

RUN sudo apt-get update && apt-get install -y libglfw3-dev libvulkan1 libgl-dev libglm-dev
RUN wget -c https://storage.googleapis.com/shaderc/artifacts/prod/graphics_shader_compiler/shaderc/linux/continuous_clang_release/386/20220812-081917/install.tgz -O - | sudo tar -xzv --strip-components=2 -C /usr/bin install/bin
RUN npm install http-server -g