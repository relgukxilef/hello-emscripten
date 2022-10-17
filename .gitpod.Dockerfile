FROM emscripten/emsdk

RUN apt-get update && apt-get install -y libglfw3-dev libvulkan1 libgl-dev libboost-dev libboost-system-dev
RUN wget -q -c https://storage.googleapis.com/shaderc/artifacts/prod/graphics_shader_compiler/shaderc/linux/continuous_clang_release/386/20220812-081917/install.tgz -O - | tar -xzv --strip-components=2 -C /usr/bin install/bin
RUN npm install http-server -g