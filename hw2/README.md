## Сборка проекта
### Debug
```shell
cmake -S . -B build_debug -DCMAKE_BUILD_TYPE=Debug && cd build_debug && make && cd ..
```
### Release
```shell
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cd build && make && cd ..
```