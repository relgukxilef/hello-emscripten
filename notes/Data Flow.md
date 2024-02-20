# Data Flow

## Current

```mermaid
sequenceDiagram
box hello
participant Client
participant Network
participant Audio Codec
participant Audio
participant Visuals
end
box native/emscripten
participant OpenAL
participant Vulkan
end
loop Update
Input ->> Client: keyboard/mouse/controller input
OpenAL ->> Audio: decoded microphone input
Audio ->> Client: decoded microphone input
Client ->> Audio Codec: decoded microphone input
Audio Codec ->> Client: encoded microphone input
Client ->> Network: positions, audio
Network ->> Client: positions, audio
Client ->> Audio Codec: encoded audio
Audio Codec ->> Client: decoded audio
Client ->> Audio: decoded audio, positions
Audio ->> OpenAL: decoded audio, positions
end
loop Draw
Client ->> Visuals: positions
Visuals ->> Vulkan: positions
end
```

## Questions

Audio Codec currently uses libopus directly. This is very slow in emscripten. Instead, the browser should use the browser's codec implementation. Should this be implemented using compiler-time polymorphism or a fixed number of coding buffers?

Network currently uses compile-time polymorphism to use boost on desktop and the browser's API in the browser. Instead, it could be implemented using a fixed number of connection slots with buffers for reading and writing. This would probably make testing easier.

Audio and Visuals use compile-time polymorphism. The alternative would probably be too complicated to implement, and the underlying implementation is already polymorph. 