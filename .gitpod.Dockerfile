# syntax = edrevo/dockerfile-plus

INCLUDE+ Dockerfile.base hello.Dockerfile

RUN npm install http-server -g