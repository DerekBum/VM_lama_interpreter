## Lama interpreter

To use my `Lama` interpreter on `C++` lang, you need to follow theese steps:

0) Be sure to install the `lamac` compiler from the [Lama repository](https://github.com/PLTools/Lama). 
1) Create an executable using `make` commang from the root of the project:
```bash
make
```
2) Create a `Lama` binary using `lamac`:
```
lamac -b <path to .lama file>
```
3) Run the `lama_c_interpreter` executable, located in the `build` directory. As example:
```bash
./build/lama_c_interpreter ./examples/a.bc
```

## Regression tests

To run regression tests, simply call (after building the interpreter)
```bash
cd ./regression
make
cd ../
```

## Performance

There is currently only one performance test. Here are results for this test:
```bash
cd ./performance
make
cd ../
```

```
Lamac interpreter:
Sort	2.12
Lama_C interpreter:
Sort	1.08
```
