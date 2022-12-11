# SD2022-2023 - GRUPO 12


Precisa das libs: 

> sudo apt-get install -y protobuf-compiler protobuf-c-compiler libprotobuf-c-dev
> sudo apt-get install -y libzookeeper-mt-dev

e portanto de ter ficheiro protobuf de linking:

> /usr/libs/x86_64-linux-gnu/libprotobuf-c.a

e

> /usr/include/zookeeper/zookeeper.h

fazer make compila server e client e mete em /binary os executaveis

Correr:
> ./tree-server host:2181

> ./tree-client host:2181

onde host -> hostname da maquina onde esta a correr o zookeeper
