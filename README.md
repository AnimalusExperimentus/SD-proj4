# SD2022-2023 - GRUPO 12


Precisa das libs: 

> apt install -y protobuf-compiler protobuf-c-compiler libprotobuf-c-dev

e portanto de ter ficheiro protobuf de linking:

> /usr/libs/x86_64-linux-gnu/libprotobuf-c.a

fazer make compila server e client e mete em /binary os executaveis

Correr:
> ./tree-server 4445

> ./tree-client 127.0.0.1:4445
