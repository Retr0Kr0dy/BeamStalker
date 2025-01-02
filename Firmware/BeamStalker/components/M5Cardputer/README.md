Fork edit : ported lib for esp-idf getting rid of arduino.h so no need for arduino as an esp components

### Usage :

Go to your porjects `components` folder, slap the lib here :

```sh
cd main
git clone https://github.com/Retr0Kr0dy/M5Cardputer
```

Go to the root of your project and add m5unified as deps :

```cpp
idf.py add-dependency "m5stack/m5unified^0.2.0" 
```

You also need to modify your main C file to C++ :
```sh
mv main/<your-porject>.c main/<your-project>.cpp
```

Modify `main/CMakeLists.txt` to :
```txt
idf_component_register(SRCS "<your-project>.cpp"
```

Modify the `app_main()` function as it's C in `<your-project>.cpp` : 
```cpp
extern "C" void app_main(void) {

}
```

Also enable C++ for all file in the current project by adding the following lines to `CMakeLists.txt` :
```txt
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

Then you can simply include headers :

```cpp
#include "M5Cardputer.h"
```

# M5Cardputer

## Basic library for M5Stack M5Cardputer Board 

License
----------------
M5GFX : [MIT](https://github.com/m5stack/M5GFX/blob/master/LICENSE)  
M5Unified : [MIT](https://github.com/m5stack/M5Unified/blob/master/LICENSE)  
 
