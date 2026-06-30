# Как собрать
```
mkdir .build
cd .build
cmake -S ..
make
```

# Как собрать дебиан пакет
``` 
dpkg-buildpackage -us -uc -b
```
